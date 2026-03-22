#include "Game.h"
#include "Graphics.h"
#include "Vertex.h"
#include "Input.h"
#include "PathHelpers.h"
#include "Window.h"
#include "Mesh.h"
#include "BufferStructs.h"
#include "Material.h"

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

	// Initialize ImGui itself & platform/renderer backends
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplWin32_Init(Window::Handle());
	ImGui_ImplDX11_Init(Graphics::Device.Get(), Graphics::Context.Get());
	// Pick a style (uncomment one of these 3)
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();
	//ImGui::StyleColorsClassic();

	cameras.push_back(std::make_shared<Camera>(Window::AspectRatio(), XMFLOAT3(0.0f, 11.0f, -18.0f), XM_PIDIV4));
	cameras.push_back(std::make_shared<Camera>(Window::AspectRatio(), XMFLOAT3(0.0f, -5.0f, -5.0f), XM_PIDIV2));

	activeCamera = cameras[0];
	activeIndex = 0;
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

	// Input Layout
	{
		D3D11_INPUT_ELEMENT_DESC inputElements[3] = {};

		inputElements[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;	
		inputElements[0].SemanticName = "POSITION";							
		inputElements[0].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;	

		inputElements[1].Format = DXGI_FORMAT_R32G32_FLOAT;			
		inputElements[1].SemanticName = "TEXCOORD";							
		inputElements[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;	

		inputElements[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		inputElements[2].SemanticName = "NORMAL";
		inputElements[2].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;

		// Create the input layout, verifying our description against actual shader code
		Graphics::Device->CreateInputLayout(
			inputElements,							// An array of descriptions
			3,										// How many elements in that array?
			vertexShaderBlob->GetBufferPointer(),	// Pointer to the code of a shader that uses this layout
			vertexShaderBlob->GetBufferSize(),		// Size of the shader code that uses this layout
			inputLayout.GetAddressOf());			// Address of the resulting ID3D11InputLayout pointer
	}
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
	// load shaders
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
	LoadVertexShader(vertexShader, L"VertexShader.cso");

	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader;
	LoadPixelShader(pixelShader, L"PixelShader.cso");
	Microsoft::WRL::ComPtr<ID3D11PixelShader> uvPixelShader;
	LoadPixelShader(uvPixelShader, L"DebugUVsPS.cso");
	Microsoft::WRL::ComPtr<ID3D11PixelShader> normalPixelShader;
	LoadPixelShader(normalPixelShader, L"DebugNormalsPS.cso");
	Microsoft::WRL::ComPtr<ID3D11PixelShader> customPixelShader;
	LoadPixelShader(customPixelShader, L"CustomPS.cso");

	// create materials
	std::shared_ptr<Material> redMaterial = std::make_shared<Material>(XMFLOAT4(1.0f, 0.25f, 0.25f, 0.0f), vertexShader, pixelShader);
	materials.push_back(redMaterial);
	std::shared_ptr<Material> uvMaterial = std::make_shared<Material>(XMFLOAT4(0.25f, 1.0f, 0.25f, 0.0f), vertexShader, uvPixelShader);
	materials.push_back(uvMaterial);
	std::shared_ptr<Material> normalMaterial = std::make_shared<Material>(XMFLOAT4(0.25f, 0.25f, 1.0f, 0.0f), vertexShader, normalPixelShader);
	materials.push_back(normalMaterial);
	std::shared_ptr<Material> customMaterial = std::make_shared<Material>(XMFLOAT4(0.25f, 0.25f, 1.0f, 0.0f), vertexShader, customPixelShader);
	materials.push_back(customMaterial);

	// Create Meshes
	meshes.push_back(std::make_shared<Mesh>(FixPath("../../Assets/Meshes/sphere.obj").c_str()));
	meshes.push_back(std::make_shared<Mesh>(FixPath("../../Assets/Meshes/torus.obj").c_str()));
	meshes.push_back(std::make_shared<Mesh>(FixPath("../../Assets/Meshes/helix.obj").c_str()));
	meshes.push_back(std::make_shared<Mesh>(FixPath("../../Assets/Meshes/cube.obj").c_str()));
	meshes.push_back(std::make_shared<Mesh>(FixPath("../../Assets/Meshes/cylinder.obj").c_str()));
	meshes.push_back(std::make_shared<Mesh>(FixPath("../../Assets/Meshes/quad.obj").c_str()));
	meshes.push_back(std::make_shared<Mesh>(FixPath("../../Assets/Meshes/quad_double_sided.obj").c_str()));

	// create GameEntities
	for (int i = 0; i < 4; i++)
	{
		entities.push_back(GameEntity(meshes[0], materials[i]));
		entities.push_back(GameEntity(meshes[1], materials[i]));
		entities.push_back(GameEntity(meshes[2], materials[i]));
		entities.push_back(GameEntity(meshes[3], materials[i]));
		entities.push_back(GameEntity(meshes[4], materials[i]));
		entities.push_back(GameEntity(meshes[5], materials[i]));
		entities.push_back(GameEntity(meshes[6], materials[i]));

		entities[0 + (7 * i)].GetTransform()->MoveAbsolute(-7.5f, (float)(3 * i), 0.0f);
		entities[1 + (7 * i)].GetTransform()->MoveAbsolute(-5.0f, (float)(3 * i), 0.0f);
		entities[2 + (7 * i)].GetTransform()->MoveAbsolute(-2.5f, (float)(3 * i), 0.0f);
		entities[3 + (7 * i)].GetTransform()->MoveAbsolute(0.0f, (float)(3 * i), 0.0f);
		entities[4 + (7 * i)].GetTransform()->MoveAbsolute(2.5f, (float)(3 * i), 0.0f);
		entities[5 + (7 * i)].GetTransform()->MoveAbsolute(5.0f, (float)(3 * i), 0.0f);
		entities[6 + (7 * i)].GetTransform()->MoveAbsolute(7.5f, (float)(3 * i), 0.0f);
	}
	




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
	for (int i = 0; i < entities.size(); i++)
	{
		entities[i].GetTransform()->Rotate(0, deltaTime * 1.0f, 0.0f);	
	}


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
		Graphics::Context->ClearDepthStencilView(Graphics::DepthBufferDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
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
			vsData.viewMat = activeCamera->GetViewMatrix();
			vsData.projMat = activeCamera->GetProjMatrix();

			FillAndBindNextConstantBuffer(
				&vsData,
				sizeof(VertexShaderData),
				D3D11_VERTEX_SHADER,
				0);

			// Pixel Shader Constant Buffer
			PixelShaderData psData = {};
			psData.colorTint = entities[i].GetMaterial()->GetColorTint();
			psData.time = totalTime;

			FillAndBindNextConstantBuffer(
				&psData,
				sizeof(PixelShaderData),
				D3D11_PIXEL_SHADER,
				0);

			// DRAW
			entities[i].Draw();
		}
	}

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



