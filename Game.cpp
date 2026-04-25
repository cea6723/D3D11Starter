#include "Game.h"
#include "Graphics.h"
#include "Vertex.h"
#include "Input.h"
#include "PathHelpers.h"
#include "Window.h"
#include "Mesh.h"
#include "BufferStructs.h"
#include "Material.h"
#include "WICTextureLoader.h"

#include <DirectXMath.h>

// Needed for a helper function to load pre-compiled shader files
#pragma comment(lib, "d3dcompiler.lib")
#include <d3dcompiler.h>

// This code assumes files are in "ImGui" subfolder!
// Adjust as necessary for your own folder structure and project setup
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"

// For the DirectX Math library
using namespace DirectX;

// --------------------------------------------------------
// The constructor is called after the window and graphics API
// are initialized but before the game loop begins
// --------------------------------------------------------
Game::Game()
{
	LoadContent();
	PostProcessSetUp();

	// Initialize Lights
	Light directionalLight1 = {};
	directionalLight1.type = 0;
	directionalLight1.direction = XMFLOAT3(1.0f, -1.0f, 1.0f);
	directionalLight1.color = XMFLOAT3(1.0f, 1.0f, 1.0f);
	directionalLight1.intensity = 1.0f;
	lights.push_back(directionalLight1);

	Light directionalLight2 = {};
	directionalLight2.type = 0;
	directionalLight2.direction = XMFLOAT3(0.0f, -1.0f, 0.0f);
	directionalLight2.color = XMFLOAT3(1.0f, 1.0f, 1.0f);
	directionalLight2.intensity = 1.0f;
	lights.push_back(directionalLight2);

	// Ring Constant Buffer
	{
		// get Direct3D 11.1 version of the context
		Graphics::Context->QueryInterface<ID3D11DeviceContext1>(context1.GetAddressOf());

		cbHeapOffsetInBytes = 0;

		cbHeapSizeInBytes = 1000 * 256;
		cbHeapSizeInBytes = (cbHeapSizeInBytes + 255) / 256 * 256;

		D3D11_BUFFER_DESC cbDesc = {};
		cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbDesc.ByteWidth = cbHeapSizeInBytes;
		cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		cbDesc.Usage = D3D11_USAGE_DYNAMIC;

		Graphics::Device->CreateBuffer(&cbDesc, 0, constantBufferHeap.GetAddressOf());
	}

	// Input Layout
	{
		//Microsoft::WRL::ComPtr<ID3DBlob> vertexShaderBlob;

		ID3DBlob* vertexShaderBlob;
		D3DReadFileToBlob(FixPath(L"VertexShader.cso").c_str(), &vertexShaderBlob);

		D3D11_INPUT_ELEMENT_DESC inputElements[4] = {};

		inputElements[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		inputElements[0].SemanticName = "POSITION";
		inputElements[0].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;

		inputElements[1].Format = DXGI_FORMAT_R32G32_FLOAT;
		inputElements[1].SemanticName = "TEXCOORD";
		inputElements[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;

		inputElements[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		inputElements[2].SemanticName = "NORMAL";
		inputElements[2].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;

		inputElements[3].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		inputElements[3].SemanticName = "TANGENT";
		inputElements[3].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;

		// Create the input layout, verifying our description against actual shader code
		Graphics::Device->CreateInputLayout(
			inputElements,							// An array of descriptions
			4,										// How many elements in that array?
			vertexShaderBlob->GetBufferPointer(),	// Pointer to the code of a shader that uses this layout
			vertexShaderBlob->GetBufferSize(),		// Size of the shader code that uses this layout
			inputLayout.GetAddressOf());			// Address of the resulting ID3D11InputLayout pointer

		Graphics::Context->IASetInputLayout(inputLayout.Get());
	}

	// Initialize ImGui itself & platform/renderer backends
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplWin32_Init(Window::Handle());
	ImGui_ImplDX11_Init(Graphics::Device.Get(), Graphics::Context.Get());
	// Pick a style (uncomment one of these 3)
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();
	//ImGui::StyleColorsClassic();

	cameras.push_back(std::make_shared<Camera>(Window::AspectRatio(), XMFLOAT3(0.0f, 3.0f, -18.0f), XM_PIDIV4));
	cameras.push_back(std::make_shared<Camera>(Window::AspectRatio(), XMFLOAT3(0.0f, -5.0f, -5.0f), XM_PIDIV2));

	activeCamera = cameras[0];
	activeIndex = 0;

	ShadowMapSetUp();
}


// --------------------------------------------------------
// Clean up memory or objects created by this class
// 
// Note: Using smart pointers means there probably won't
//       be much to manually clean up here!
// --------------------------------------------------------
Game::~Game()
{
	// ImGui clean up
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void Game::FillAndBindNextConstantBuffer(void* data, unsigned int dataSizeInBytes, D3D11_SHADER_TYPE shaderType, unsigned int registerSlot)
{
	// get next multiple of 256
	unsigned int reservationSize = (dataSizeInBytes + 255) / 256 * 256;

	// determine if enough space for new reservation, if not loop back to beginning
	if (cbHeapOffsetInBytes + reservationSize >= cbHeapSizeInBytes) cbHeapOffsetInBytes = 0;

	// map, memcpy, unmap
	D3D11_MAPPED_SUBRESOURCE map{};
	Graphics::Context->Map(
		constantBufferHeap.Get(),
		0,
		D3D11_MAP_WRITE_NO_OVERWRITE,
		0,
		&map);
	void* uploadAddress = reinterpret_cast<void*>((UINT64)map.pData + cbHeapOffsetInBytes);
	memcpy(uploadAddress, data, dataSizeInBytes);
	Graphics::Context->Unmap(constantBufferHeap.Get(), 0);

	// calculate binding offset and size in shader constants (16-byte constants)
	unsigned int firstConstant = cbHeapOffsetInBytes / 16;
	unsigned int numConstants = reservationSize / 16;

	// bind the buffer to proper pipeline stage
	switch (shaderType)
	{
	case D3D11_VERTEX_SHADER:
		context1->VSSetConstantBuffers1(
			registerSlot,
			1,
			constantBufferHeap.GetAddressOf(),
			&firstConstant,
			&numConstants
		);
		break;
	case D3D11_PIXEL_SHADER:
		context1->PSSetConstantBuffers1(
			registerSlot,
			1,
			constantBufferHeap.GetAddressOf(),
			&firstConstant,
			&numConstants
		);
		break;
	}

	// offset for the next call
	cbHeapOffsetInBytes += reservationSize;
}

void Game::LoadVertexShader(Microsoft::WRL::ComPtr<ID3D11VertexShader>& vertexShader, const std::wstring& filePath)
{
	ID3DBlob* vertexShaderBlob;
	D3DReadFileToBlob(FixPath(filePath).c_str(), &vertexShaderBlob);
	Graphics::Device->CreateVertexShader(
		vertexShaderBlob->GetBufferPointer(),
		vertexShaderBlob->GetBufferSize(),
		0,
		vertexShader.GetAddressOf());
}

void Game::LoadPixelShader(Microsoft::WRL::ComPtr<ID3D11PixelShader>& pixelShader, const std::wstring& filePath)
{
	ID3DBlob* pixelShaderBlob;
	D3DReadFileToBlob(FixPath(filePath).c_str(), &pixelShaderBlob);
	Graphics::Device->CreatePixelShader(
		pixelShaderBlob->GetBufferPointer(),
		pixelShaderBlob->GetBufferSize(),
		0,
		pixelShader.GetAddressOf());
}


// --------------------------------------------------------
// Loads shaders from compiled shader object (.cso) files
// and also created the Input Layout that describes our 
// vertex data to the rendering pipeline. 
// - Input Layout creation is done here because it must 
//    be verified against vertex shader byte code
// - We'll have that byte code already loaded below
// --------------------------------------------------------
void Game::LoadContent()
{
	// create sampler
	Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler;
	D3D11_SAMPLER_DESC sampDesc = {};
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	sampDesc.MaxAnisotropy = 16;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	Graphics::Device->CreateSamplerState(&sampDesc, sampler.GetAddressOf());

	// load shaders
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
	LoadVertexShader(vertexShader, L"VertexShader.cso");
	Microsoft::WRL::ComPtr<ID3D11VertexShader> skyVertexShader;
	LoadVertexShader(skyVertexShader, L"SkyVertexShader.cso");
	LoadVertexShader(shadowVS, L"ShadowMapVS.cso");
	LoadVertexShader(ppVS, L"FullscreenVS.cso");

	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader;
	LoadPixelShader(pixelShader, L"PixelShader.cso");
	Microsoft::WRL::ComPtr<ID3D11PixelShader> uvPixelShader;
	LoadPixelShader(uvPixelShader, L"DebugUVsPS.cso");
	Microsoft::WRL::ComPtr<ID3D11PixelShader> normalPixelShader;
	LoadPixelShader(normalPixelShader, L"DebugNormalsPS.cso");
	Microsoft::WRL::ComPtr<ID3D11PixelShader> customPixelShader;
	LoadPixelShader(customPixelShader, L"CustomPS.cso");
	Microsoft::WRL::ComPtr<ID3D11PixelShader> comboPixelShader;
	LoadPixelShader(comboPixelShader, L"ComboPS.cso");
	Microsoft::WRL::ComPtr<ID3D11PixelShader> skyPixelShader;
	LoadPixelShader(skyPixelShader, L"SkyPixelShader.cso");
	LoadPixelShader(ppBlurPS, L"PPBlurPS.cso");
	LoadPixelShader(ppChromaticPS, L"PPChromaticPS.cso");


	// load textures
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> rockSRV;
	CreateWICTextureFromFile(Graphics::Device.Get(), Graphics::Context.Get(), FixPath(L"../../Assets/Textures/rock_wall_15_diff_4k.png").c_str(), 0, rockSRV.GetAddressOf());
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> rockNormalSRV;
	CreateWICTextureFromFile(Graphics::Device.Get(), Graphics::Context.Get(), FixPath(L"../../Assets/Textures/rock_wall_15_nor_dx_4k.png").c_str(), 0, rockNormalSRV.GetAddressOf());
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> rockRoughSRV;
	CreateWICTextureFromFile(Graphics::Device.Get(), Graphics::Context.Get(), FixPath(L"../../Assets/Textures/rock_wall_15_rough_4k.png").c_str(), 0, rockRoughSRV.GetAddressOf());

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> bronzeSRV;
	CreateWICTextureFromFile(Graphics::Device.Get(), Graphics::Context.Get(), FixPath(L"../../Assets/Textures/bronze_albedo.png").c_str(), 0, bronzeSRV.GetAddressOf());
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> bronzeNormalSRV;
	CreateWICTextureFromFile(Graphics::Device.Get(), Graphics::Context.Get(), FixPath(L"../../Assets/Textures/bronze_normals.png").c_str(), 0, bronzeNormalSRV.GetAddressOf());
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> bronzeRoughSRV;
	CreateWICTextureFromFile(Graphics::Device.Get(), Graphics::Context.Get(), FixPath(L"../../Assets/Textures/bronze_roughness.png").c_str(), 0, bronzeRoughSRV.GetAddressOf());

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> floorSRV;
	CreateWICTextureFromFile(Graphics::Device.Get(), Graphics::Context.Get(), FixPath(L"../../Assets/Textures/floor_albedo.png").c_str(), 0, floorSRV.GetAddressOf());
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> floorNormalSRV;
	CreateWICTextureFromFile(Graphics::Device.Get(), Graphics::Context.Get(), FixPath(L"../../Assets/Textures/floor_normals.png").c_str(), 0, floorNormalSRV.GetAddressOf());
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> floorRoughSRV;
	CreateWICTextureFromFile(Graphics::Device.Get(), Graphics::Context.Get(), FixPath(L"../../Assets/Textures/floor_roughness.png").c_str(), 0, floorRoughSRV.GetAddressOf());
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> floorMetalSRV;
	CreateWICTextureFromFile(Graphics::Device.Get(), Graphics::Context.Get(), FixPath(L"../../Assets/Textures/floor_metal.png").c_str(), 0, floorMetalSRV.GetAddressOf());

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> woodSRV;
	CreateWICTextureFromFile(Graphics::Device.Get(), Graphics::Context.Get(), FixPath(L"../../Assets/Textures/wood_albedo.png").c_str(), 0, woodSRV.GetAddressOf());
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> woodNormalSRV;
	CreateWICTextureFromFile(Graphics::Device.Get(), Graphics::Context.Get(), FixPath(L"../../Assets/Textures/wood_normals.png").c_str(), 0, woodNormalSRV.GetAddressOf());
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> woodRoughSRV;
	CreateWICTextureFromFile(Graphics::Device.Get(), Graphics::Context.Get(), FixPath(L"../../Assets/Textures/wood_roughness.png").c_str(), 0, woodRoughSRV.GetAddressOf());

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> noMetalSRV;
	CreateWICTextureFromFile(Graphics::Device.Get(), Graphics::Context.Get(), FixPath(L"../../Assets/Textures/no_metal.png").c_str(), 0, noMetalSRV.GetAddressOf());
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> allMetalSRV;
	CreateWICTextureFromFile(Graphics::Device.Get(), Graphics::Context.Get(), FixPath(L"../../Assets/Textures/all_metal.png").c_str(), 0, allMetalSRV.GetAddressOf());




	// create materials
	std::shared_ptr<Material> rockMaterial = std::make_shared<Material>("Rock", XMFLOAT4(1.0f, 1.0f, 1.0f, 0.0f), vertexShader, pixelShader);
	materials.push_back(rockMaterial);
	std::shared_ptr<Material> bronzeMaterial = std::make_shared<Material>("Bronze", XMFLOAT4(1.0f, 1.0f, 1.0f, 0.0f), vertexShader, pixelShader);
	materials.push_back(bronzeMaterial);
	std::shared_ptr<Material> floorMaterial = std::make_shared<Material>("Floor", XMFLOAT4(1.0f, 1.0f, 1.0f, 0.0f), vertexShader, pixelShader);
	materials.push_back(floorMaterial);
	std::shared_ptr<Material> woodMaterial = std::make_shared<Material>("Wood", XMFLOAT4(1.0f, 1.0f, 1.0f, 0.0f), vertexShader, pixelShader);
	materials.push_back(woodMaterial);

	std::shared_ptr<Material> uvMaterial = std::make_shared<Material>("UVs", XMFLOAT4(0.25f, 1.0f, 0.25f, 0.0f), vertexShader, uvPixelShader);
	materials.push_back(uvMaterial);
	std::shared_ptr<Material> normalMaterial = std::make_shared<Material>("Normals", XMFLOAT4(0.25f, 0.25f, 1.0f, 0.0f), vertexShader, normalPixelShader);
	materials.push_back(normalMaterial);
	std::shared_ptr<Material> customMaterial = std::make_shared<Material>("Timey Wimey", XMFLOAT4(0.25f, 0.25f, 1.0f, 0.0f), vertexShader, customPixelShader);
	materials.push_back(customMaterial);

	// add textures and samplers to materials
	rockMaterial->AddTextureSRV(0, rockSRV);
	rockMaterial->AddTextureSRV(1, rockNormalSRV);
	rockMaterial->AddTextureSRV(2, rockRoughSRV);
	rockMaterial->AddTextureSRV(3, noMetalSRV);
	rockMaterial->AddSampler(0, sampler);

	bronzeMaterial->AddTextureSRV(0, bronzeSRV);
	bronzeMaterial->AddTextureSRV(1, bronzeNormalSRV);
	bronzeMaterial->AddTextureSRV(2, bronzeRoughSRV);
	bronzeMaterial->AddTextureSRV(3, allMetalSRV);
	bronzeMaterial->AddSampler(0, sampler);

	floorMaterial->AddTextureSRV(0, floorSRV);
	floorMaterial->AddTextureSRV(1, floorNormalSRV);
	floorMaterial->AddTextureSRV(2, floorRoughSRV);
	floorMaterial->AddTextureSRV(3, floorMetalSRV);
	floorMaterial->AddSampler(0, sampler);

	woodMaterial->AddTextureSRV(0, woodSRV);
	woodMaterial->AddTextureSRV(1, woodNormalSRV);
	woodMaterial->AddTextureSRV(2, woodRoughSRV);
	woodMaterial->AddTextureSRV(3, noMetalSRV);
	woodMaterial->AddSampler(0, sampler);

	// Create Meshes
	meshes.push_back(std::make_shared<Mesh>(FixPath("../../Assets/Meshes/sphere.obj").c_str()));			// 0 - sphere
	meshes.push_back(std::make_shared<Mesh>(FixPath("../../Assets/Meshes/torus.obj").c_str()));				// 1 - torus
	meshes.push_back(std::make_shared<Mesh>(FixPath("../../Assets/Meshes/helix.obj").c_str()));				// 2 - helix
	meshes.push_back(std::make_shared<Mesh>(FixPath("../../Assets/Meshes/cube.obj").c_str()));				// 3 - cube
	meshes.push_back(std::make_shared<Mesh>(FixPath("../../Assets/Meshes/cylinder.obj").c_str()));			// 4 - cylinder
	meshes.push_back(std::make_shared<Mesh>(FixPath("../../Assets/Meshes/quad.obj").c_str()));				// 5 - quad
	meshes.push_back(std::make_shared<Mesh>(FixPath("../../Assets/Meshes/quad_double_sided.obj").c_str())); // 6 - double-sided quad

	// create GameEntities
	entities.push_back(GameEntity(meshes[5], woodMaterial));
	entities[0].GetTransform()->SetScale(XMFLOAT3(10.0f, 1.0f, 10.0f));

	entities.push_back(GameEntity(meshes[0], floorMaterial));
	entities[1].GetTransform()->MoveAbsolute(0.0f, 0.5f, 0.0f);

	entities.push_back(GameEntity(meshes[4], rockMaterial));
	entities[2].GetTransform()->MoveAbsolute(-5.0f, 1.5f, 0.0f);

	entities.push_back(GameEntity(meshes[2], bronzeMaterial));
	entities[3].GetTransform()->MoveAbsolute(5.0f, 1.5f, 0.0f);

	// create sky
	// meshes[3] is cube
	sky = std::make_shared<Sky>(meshes[3], sampler, skyPixelShader, skyVertexShader,
		FixPath(L"../../Assets/Textures/right.png").c_str(),
		FixPath(L"../../Assets/Textures/left.png").c_str(),
		FixPath(L"../../Assets/Textures/up.png").c_str(),
		FixPath(L"../../Assets/Textures/down.png").c_str(),
		FixPath(L"../../Assets/Textures/front.png").c_str(),
		FixPath(L"../../Assets/Textures/back.png").c_str());

	// Set initial graphics API state
	//  - These settings persist until we change them
	//  - Some of these, like the primitive topology & input layout, probably won't change
	//  - Others, like setting shaders, will need to be moved elsewhere later
	{
		// Tell the input assembler (IA) stage of the pipeline what kind of
		// geometric primitives (points, lines or triangles) we want to draw.  
		// Essentially: "What kind of shape should the GPU draw with our vertices?"
		Graphics::Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// Ensure the pipeline knows how to interpret all the numbers stored in
		// the vertex buffer. For this course, all of your vertices will probably
		// have the same layout, so we can just set this once at startup.
		Graphics::Context->IASetInputLayout(inputLayout.Get());
	}
}

void Game::ShadowMapSetUp()
{
	// Reset existing API objects
	shadowDSV.Reset();
	shadowSRV.Reset();
	shadowRasterizer.Reset();
	shadowSampler.Reset();

	// Shadow Map Texutre
	D3D11_TEXTURE2D_DESC shadowDesc = {};
	shadowDesc.Width = 1024;
	shadowDesc.Height = 1024;
	shadowDesc.ArraySize = 1;
	shadowDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	shadowDesc.CPUAccessFlags = 0;
	shadowDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	shadowDesc.MipLevels = 1;
	shadowDesc.MiscFlags = 0;
	shadowDesc.SampleDesc.Count = 1;
	shadowDesc.SampleDesc.Quality = 0;
	shadowDesc.Usage = D3D11_USAGE_DEFAULT;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> shadowTexture;
	Graphics::Device->CreateTexture2D(&shadowDesc, 0, shadowTexture.GetAddressOf());

	// Create the depth/stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC shadowDSDesc = {};
	shadowDSDesc.Format = DXGI_FORMAT_D32_FLOAT;
	shadowDSDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	shadowDSDesc.Texture2D.MipSlice = 0;
	Graphics::Device->CreateDepthStencilView(
		shadowTexture.Get(),
		&shadowDSDesc,
		shadowDSV.GetAddressOf());

	// Create the SRV for the shadow map
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	Graphics::Device->CreateShaderResourceView(
		shadowTexture.Get(),
		&srvDesc,
		shadowSRV.GetAddressOf());

	// Create a rasterizer state
	D3D11_RASTERIZER_DESC shadowRastDesc = {};
	shadowRastDesc.FillMode = D3D11_FILL_SOLID;
	shadowRastDesc.CullMode = D3D11_CULL_BACK;
	shadowRastDesc.DepthClipEnable = false;
	shadowRastDesc.DepthBias = 1000;
	shadowRastDesc.SlopeScaledDepthBias = 1.0f;
	Graphics::Device->CreateRasterizerState(&shadowRastDesc, &shadowRasterizer);

	D3D11_SAMPLER_DESC shadowSampDesc = {};
	shadowSampDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
	shadowSampDesc.ComparisonFunc = D3D11_COMPARISON_LESS;
	shadowSampDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	shadowSampDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	shadowSampDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	shadowSampDesc.BorderColor[0] = 1.0f; // Only need the first component
	Graphics::Device->CreateSamplerState(&shadowSampDesc, &shadowSampler);

	// light view matrix
	XMMATRIX lightView = XMMatrixLookAtLH(XMLoadFloat3(
		&lights[0].direction) * XMVectorReplicate(-20.0f),
		XMLoadFloat3(&lights[0].direction),
		XMVectorSet(0, 1, 0, 0));
	XMStoreFloat4x4(&lightViewMatrix, lightView);

	// light proj matrix
	float lightProjSize = 10.0f;
	XMMATRIX lightProj = XMMatrixOrthographicLH(
		lightProjSize,
		lightProjSize,
		1.0f,
		100.0f);
	XMStoreFloat4x4(&lightProjectionMatrix, lightProj);
}

void Game::PostProcessSetUp()
{
	// sampler
	D3D11_SAMPLER_DESC ppSampDesc = {};
	ppSampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	ppSampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	ppSampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	ppSampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	ppSampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	Graphics::Device->CreateSamplerState(&ppSampDesc, ppSampler.GetAddressOf());

	ResizeRenderTarget();
}

void Game::ResizeRenderTarget()
{
	ppRTV1.Reset();
	ppSRV1.Reset();
	ppRTV2.Reset();
	ppSRV2.Reset();

	// Describe the texture we're creating
	D3D11_TEXTURE2D_DESC textureDesc1 = {};
	textureDesc1.Width = Window::Width();
	textureDesc1.Height = Window::Height();
	textureDesc1.ArraySize = 1;
	textureDesc1.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	textureDesc1.CPUAccessFlags = 0;
	textureDesc1.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc1.MipLevels = 1;
	textureDesc1.MiscFlags = 0;
	textureDesc1.SampleDesc.Count = 1;
	textureDesc1.SampleDesc.Quality = 0;
	textureDesc1.Usage = D3D11_USAGE_DEFAULT;

	// Create the resource (no need to track it after the views are created below)
	Microsoft::WRL::ComPtr<ID3D11Texture2D> ppTexture1;
	Graphics::Device->CreateTexture2D(&textureDesc1, 0, ppTexture1.GetAddressOf());

	Microsoft::WRL::ComPtr<ID3D11Texture2D> ppTexture2;
	Graphics::Device->CreateTexture2D(&textureDesc1, 0, ppTexture2.GetAddressOf());

	// Create Render Target View
	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = textureDesc1.Format;
	rtvDesc.Texture2D.MipSlice = 0;
	rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	Graphics::Device->CreateRenderTargetView(
		ppTexture1.Get(),
		&rtvDesc,
		ppRTV1.ReleaseAndGetAddressOf());

	Graphics::Device->CreateRenderTargetView(
		ppTexture2.Get(),
		&rtvDesc,
		ppRTV2.ReleaseAndGetAddressOf());

	// Create Shader Resource View
	Graphics::Device->CreateShaderResourceView(
		ppTexture1.Get(),
		0,
		ppSRV1.ReleaseAndGetAddressOf());

	Graphics::Device->CreateShaderResourceView(
		ppTexture2.Get(),
		0,
		ppSRV2.ReleaseAndGetAddressOf());
}

void Game::RenderShadowMap()
{
	// render fresh info to shadow map
	Graphics::Context->ClearDepthStencilView(shadowDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
	ID3D11RenderTargetView* nullRTV{};
	Graphics::Context->OMSetRenderTargets(1, &nullRTV, shadowDSV.Get());
	Graphics::Context->RSSetState(shadowRasterizer.Get());

	Graphics::Context->PSSetShader(0, 0, 0);

	// change viewport
	D3D11_VIEWPORT viewport = {};
	viewport.Width = 1024.0f;
	viewport.Height = 1024.0f;
	viewport.MaxDepth = 1.0f;
	Graphics::Context->RSSetViewports(1, &viewport);

	// entity render loop
	Graphics::Context->VSSetShader(shadowVS.Get(), 0, 0);

	struct ShadowVSData
	{
		XMFLOAT4X4 world;
		XMFLOAT4X4 view;
		XMFLOAT4X4 proj;

	};

	ShadowVSData vsData = {};
	vsData.view = lightViewMatrix;
	vsData.proj = lightProjectionMatrix;

	// Loop and draw all entities
	for (int i = 0; i < entities.size(); i++) {
		vsData.world = entities[i].GetTransform()->GetWorldMatrix();
		FillAndBindNextConstantBuffer(&vsData, sizeof(ShadowVSData),
			D3D11_VERTEX_SHADER,
			0);
		entities[i].GetMesh()->Draw();
	}

	// reset pipeline
	viewport.Width = (float)Window::Width();
	viewport.Height = (float)Window::Height();
	Graphics::Context->RSSetViewports(1, &viewport);
	Graphics::Context->OMSetRenderTargets(
		1,
		Graphics::BackBufferRTV.GetAddressOf(),
		Graphics::DepthBufferDSV.Get());
	Graphics::Context->RSSetState(0);
}

// --------------------------------------------------------
// Handle resizing to match the new window size
//  - Eventually, we'll want to update our 3D camera
// --------------------------------------------------------
void Game::OnResize()
{
	for (int i = 0; i < cameras.size(); i++)
	{
		if (cameras[i] != nullptr) cameras[i]->UpdateProjMatrix(Window::AspectRatio());
	}

	ResizeRenderTarget();
}

void Game::ImGuiHelper(float deltaTime)
{
	// Put this all in a helper method that is called from Game::Update()
	// Feed fresh data to ImGui
	ImGuiIO& io = ImGui::GetIO();
	io.DeltaTime = deltaTime;
	io.DisplaySize.x = (float)Window::Width();
	io.DisplaySize.y = (float)Window::Height();
	// Reset the frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	// Determine new input capture
	Input::SetKeyboardCapture(io.WantCaptureKeyboard);
	Input::SetMouseCapture(io.WantCaptureMouse);
	// Show the demo window
	if (Game::isDemoVisible)
	{
		ImGui::ShowDemoWindow();
	}
}

void Game::BuildUI()
{
	ImGui::Begin("Info");
	{
		if (ImGui::TreeNode("App Details"))
		{
			ImGui::Text("Frame rate: %f fps", ImGui::GetIO().Framerate);
			ImGui::Text("Window Client Size: %ix%i", Window::Width(), Window::Height());
			ImGui::ColorEdit4("Background Color", bgColor);
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Cameras"))
		{
			for (int i = 0; i < cameras.size(); i++)
			{
				char name[20];
				sprintf_s(name, "Camera %i", i);
				ImGui::RadioButton(name, &activeIndex, i);
			}
			activeCamera = cameras[activeIndex];

			ImGui::Text("Active Camera:");
			XMFLOAT3 position = activeCamera->GetPosition();
			float fov = activeCamera->GetFOV();
			ImGui::DragFloat3("Position", &position.x);
			ImGui::DragFloat("FOV", &fov);

			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Lighting"))
		{
			ImGui::ColorEdit4("Ambient Color", (float*)&ambientColor);

			for (int i = 0; i < lights.size(); i++)
			{
				ImGui::PushID(i);
				char name[20];
				sprintf_s(name, "Light %i", i);
				if (ImGui::TreeNode(name))
				{
					lights[i].color;
					ImGui::ColorEdit4("Color", (float*)&lights[i].color);
					ImGui::DragFloat("Intensity", &lights[i].intensity, 0.01f, 0.0f, 1.0f);
					ImGui::TreePop();
				}
				ImGui::PopID();
			}

			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Shadows"))
		{
			ImGui::Image(shadowSRV.Get(), ImVec2(512, 512));
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Post Processes"))
		{
			if (ImGui::TreeNode("Blur"))
			{
				ImGui::SliderInt("Blur Radius", &blurRadius, 0.0f, 10.0f);
				ImGui::TreePop();
			}
			if (ImGui::TreeNode("Chromatic Abberation"))
			{
				ImGui::SliderFloat3("RGB offset", &chromaticOffset[0], -0.01f, 0.01f);
				ImGui::TreePop();
			}
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Meshes"))
		{
			ImGui::ColorEdit4("Color Tint", colorTint);
			ImGui::SliderFloat3("Offset", offset, -1.0f, 1.0f);
			for (int i = 0; i < meshes.size(); i++)
			{
				ImGui::PushID(i);
				if (ImGui::TreeNode(meshes[i]->GetName()))
				{
					ImGui::Text("Triangles: %i", meshes[i]->GetIndexCount() / 3);
					ImGui::Text("Vertices: %i", meshes[i]->GetVertexCount());
					ImGui::Text("Indices: %i", meshes[i]->GetIndexCount());
					ImGui::TreePop();
				}
				ImGui::PopID();
			}
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Materials"))
		{
			for (int i = 0; i < materials.size(); i++)
			{
				ImGui::PushID(i);
				if (ImGui::TreeNode(materials[i]->GetName()))
				{
					XMFLOAT4 colorTint = materials[i]->GetColorTint();
					XMFLOAT2 uvScale = materials[i]->GetUVScale();
					XMFLOAT2 uvOffset = materials[i]->GetUVOffset();

					ImGui::ColorEdit4("Color Tint", (float*)&colorTint);
					materials[i]->SetColorTint(colorTint);

					ImGui::DragFloat2("UV Scale", (float*)&uvScale, 0.01f);
					materials[i]->SetUVScale(uvScale);

					ImGui::DragFloat2("UV Offset", (float*)&uvOffset, 0.01f);
					materials[i]->SetUVOffset(uvOffset);

					ImGui::Text("Textures");
					for (int j = 0; j < materials[i]->GetMaxSRVIndex() + 1; j++) {
						ImGui::Image((void*)materials[i]->GetSRV(j).Get(), ImVec2(128.0f, 128.0f));
						if (j != materials[i]->GetMaxSRVIndex()) ImGui::SameLine();
					}
					ImGui::TreePop();
				}
				ImGui::PopID();
			}
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Game Entities"))
		{
			for (int i = 0; i < entities.size(); i++)
			{
				ImGui::PushID(i);
				char name[20];
				sprintf_s(name, "Entity %i", i);
				if (ImGui::TreeNode(name))
				{
					ImGui::Text("Mesh: %s", entities[i].GetMesh()->GetName());

					XMFLOAT3 position = entities[i].GetTransform()->GetPosition();
					XMFLOAT3 rotation = entities[i].GetTransform()->GetPitchYawRoll();
					XMFLOAT3 scale = entities[i].GetTransform()->GetScale();

					ImGui::DragFloat3("Position", &position.x);
					ImGui::DragFloat3("Rotation (Radians)", &rotation.x);
					ImGui::DragFloat3("Scale", &scale.x);

					ImGui::TreePop();
				}
				ImGui::PopID();
			}
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Other Stuff"))
		{
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Colorful Text!");

			ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Stuff on"); ImGui::SameLine();
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 1.0f, 1.0f), "the same line??");

			ImGui::RadioButton("this one", &activeIndex, 0); ImGui::SameLine();
			ImGui::RadioButton("that one", &activeIndex, 1); ImGui::SameLine();
			ImGui::RadioButton("another one", &activeIndex, 2);

			ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "How many bananas could you eat rn?");
			ImGui::DragInt("##drag", &dragNum, 1);

			ImGui::TreePop();
		}
		if (ImGui::Button("Show ImGui Demo"))
		{
			Game::isDemoVisible = !Game::isDemoVisible;
		}
	}
	ImGui::End();
}

// --------------------------------------------------------
// Update your game here - user input, move objects, AI, etc.
// --------------------------------------------------------
void Game::Update(float deltaTime, float totalTime)
{
	ImGuiHelper(deltaTime);
	BuildUI();
	activeCamera->Update(deltaTime);

	// move entities
	entities[1].GetTransform()->MoveAbsolute(XMFLOAT3(0.0f, sin(totalTime) * deltaTime, 0.0f));

	entities[2].GetTransform()->MoveAbsolute(XMFLOAT3(sin(totalTime) * deltaTime, 0.0f, 0.0f));
	entities[2].GetTransform()->Rotate(deltaTime * 1.0f, 0.0f, deltaTime * 1.0f);

	entities[3].GetTransform()->MoveAbsolute(XMFLOAT3(sin(totalTime) * deltaTime * -1, 0.0f, 0.0f));
	entities[3].GetTransform()->Rotate(0.0f, deltaTime * 1.0f, 0.0f);

	// Ensure the light view matrix matches the first directional light
	XMMATRIX lightView = XMMatrixLookAtLH(
		XMLoadFloat3(&lights[0].direction) * XMVectorReplicate(-10.0f), // Back up ~30 units
		XMLoadFloat3(&lights[0].direction),
		XMVectorSet(0, 1, 0, 0));
	XMStoreFloat4x4(&lightViewMatrix, lightView);

	// Example input checking: Quit if the escape key is pressed
	if (Input::KeyDown(VK_ESCAPE))
		Window::Quit();
}


// --------------------------------------------------------
// Clear the screen, redraw everything, present to the user
// --------------------------------------------------------
void Game::Draw(float deltaTime, float totalTime)
{
	// Frame START
	// - These things should happen ONCE PER FRAME
	// - At the beginning of Game::Draw() before drawing *anything*
	{

		// Clear the back buffer (erase what's on screen) and depth buffer
		Graphics::Context->ClearRenderTargetView(Graphics::BackBufferRTV.Get(), Game::bgColor);
		Graphics::Context->ClearRenderTargetView(ppRTV1.Get(), Game::bgColor);
		Graphics::Context->ClearRenderTargetView(ppRTV2.Get(), Game::bgColor);

		Graphics::Context->ClearDepthStencilView(Graphics::DepthBufferDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

		RenderShadowMap();

		// swap active render target
		Graphics::Context->OMSetRenderTargets(1, ppRTV1.GetAddressOf(), Graphics::DepthBufferDSV.Get());
	}

	// DRAW geometry
	// - These steps are generally repeated for EACH object you draw
	// - Other Direct3D calls will also be necessary to do more complex things
	{
		for (int i = 0; i < entities.size(); i++)
		{
			// Vertex Shader Constant Buffer
			VertexShaderData vsData = {};
			vsData.worldMat = entities[i].GetTransform()->GetWorldMatrix();
			vsData.worldInvTransMat = entities[i].GetTransform()->GetWorldInverseTransposeMatrix();
			vsData.viewMat = activeCamera->GetViewMatrix();
			vsData.projMat = activeCamera->GetProjMatrix();
			vsData.lightView = lightViewMatrix;
			vsData.lightProj = lightProjectionMatrix;

			FillAndBindNextConstantBuffer(
				&vsData,
				sizeof(VertexShaderData),
				D3D11_VERTEX_SHADER,
				0);

			// Pixel Shader Constant Buffer
			PixelShaderData psData = {};
			psData.colorTint = entities[i].GetMaterial()->GetColorTint();
			psData.ambientColor = (XMFLOAT3)ambientColor;
			psData.uvScale = entities[i].GetMaterial()->GetUVScale();
			psData.uvOffset = entities[i].GetMaterial()->GetUVOffset();
			psData.cameraPos = activeCamera->GetPosition();
			psData.time = totalTime;
			memcpy(&psData.lights, &lights[0], sizeof(Light) * (int)lights.size());

			FillAndBindNextConstantBuffer(
				&psData,
				sizeof(PixelShaderData),
				D3D11_PIXEL_SHADER,
				0);

			entities[i].GetMaterial()->BindTexturesAndSamplers();

			// Set the shadow map and shadow sampler for upcoming draws
			Graphics::Context->PSSetShaderResources(4, 1, shadowSRV.GetAddressOf());
			Graphics::Context->PSSetSamplers(1, 1, shadowSampler.GetAddressOf());

			// DRAW
			entities[i].Draw();
		}

		// Sky constant buffer
		SkyVertexShaderData svsData;
		svsData.projMat = activeCamera->GetProjMatrix();
		svsData.viewMat = activeCamera->GetViewMatrix();
		FillAndBindNextConstantBuffer(
			&svsData,
			sizeof(SkyVertexShaderData),
			D3D11_VERTEX_SHADER,
			0);

		sky->Draw();

		// *** POST PROCESSING ***
		Graphics::Context->VSSetShader(ppVS.Get(), 0, 0);
		Graphics::Context->PSSetSamplers(0, 1, ppSampler.GetAddressOf());

		// Chromatic Abberation
		{
			Graphics::Context->OMSetRenderTargets(1, ppRTV2.GetAddressOf(), 0); // write
			Graphics::Context->PSSetShaderResources(0, 1, ppSRV1.GetAddressOf()); // read
			Graphics::Context->PSSetShader(ppChromaticPS.Get(), 0, 0);

			struct ChromaticData
			{
				float redOffset;
				float greenOffset;
				float blueOffset;
			};

			ChromaticData chrData = {};
			chrData.redOffset = chromaticOffset[0];
			chrData.greenOffset = chromaticOffset[1];
			chrData.blueOffset = chromaticOffset[2];
			FillAndBindNextConstantBuffer(&chrData, sizeof(ChromaticData), D3D11_PIXEL_SHADER, 0);

			Graphics::Context->Draw(3, 0);
		}

		// Blur
		{
			Graphics::Context->OMSetRenderTargets(1, Graphics::BackBufferRTV.GetAddressOf(), 0); // write
			Graphics::Context->PSSetShaderResources(0, 1, ppSRV2.GetAddressOf()); // read
			Graphics::Context->PSSetShader(ppBlurPS.Get(), 0, 0);

			struct BlurData
			{
				int blurRadius;
				float pixelWidth;
				float pixelHeight;
			};

			BlurData blurData = {};
			blurData.blurRadius = blurRadius;
			blurData.pixelWidth = 1.0f / Window::Width();
			blurData.pixelHeight = 1.0f / Window::Height();
			FillAndBindNextConstantBuffer(&blurData, sizeof(BlurData), D3D11_PIXEL_SHADER, 0);

			Graphics::Context->Draw(3, 0);
		}

	}

	// We need to un-bind (deactivate) the shadow map as a 
	// shader resource since we'll be using it as a depth buffer
	// at the beginning of next frame!  To make it easy, I'm simply
	// unbinding all SRV's from pixel shader stage here
	ID3D11ShaderResourceView* nullSRVs[128] = {};
	Graphics::Context->PSSetShaderResources(0, 128, nullSRVs);

	ImGui::Render(); // Turns this frame’s UI into renderable triangles
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData()); // Draws it to the screen

	// Frame END
	// - These should happen exactly ONCE PER FRAME
	// - At the very end of the frame (after drawing *everything*)
	{
		// Present at the end of the frame
		bool vsync = Graphics::VsyncState();
		Graphics::SwapChain->Present(
			vsync ? 1 : 0,
			vsync ? 0 : DXGI_PRESENT_ALLOW_TEARING);

		// Re-bind back buffer and depth buffer after presenting
		Graphics::Context->OMSetRenderTargets(
			1,
			Graphics::BackBufferRTV.GetAddressOf(),
			Graphics::DepthBufferDSV.Get());
	}
}



