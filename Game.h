#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include <vector>
#include <memory>
#include <DirectXMath.h>
#include <d3d11shadertracing.h>

#include "Mesh.h"
#include "GameEntity.h"
#include "Camera.h"
#include "Lights.h"
#include "Sky.h"

class Game
{
public:
	// Basic OOP setup
	Game();
	~Game();
	Game(const Game&) = delete; // Remove copy constructor
	Game& operator=(const Game&) = delete; // Remove copy-assignment operator

	// Primary functions
	void Update(float deltaTime, float totalTime);
	void Draw(float deltaTime, float totalTime);
	void OnResize();

private:

	// Initialization helper methods - feel free to customize, combine, remove, etc.
	void FillAndBindNextConstantBuffer(void* data, unsigned int dataSizeInBytes, D3D11_SHADER_TYPE shaderType, unsigned int registerSlot);
	void LoadVertexShader(Microsoft::WRL::ComPtr<ID3D11VertexShader>& vertexShader, const std::wstring& filePath);
	void LoadPixelShader(Microsoft::WRL::ComPtr<ID3D11PixelShader>& pixelShader, const std::wstring& filePath);
	void LoadContent();
	void ShadowMapSetUp();
	void RenderShadowMap();
	void ImGuiHelper(float deltaTime);
	void BuildUI();

	// Note the usage of ComPtr below
	//  - This is a smart pointer for objects that abide by the
	//     Component Object Model, which DirectX objects do
	//  - More info here: https://github.com/Microsoft/DirectXTK/wiki/ComPtr

	// Buffers to hold actual geometry data
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> constBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> pixelConstBuffer;

	// Ring Constant Buffer
	Microsoft::WRL::ComPtr<ID3D11DeviceContext1> context1;
	Microsoft::WRL::ComPtr<ID3D11Buffer> constantBufferHeap;
	unsigned int cbHeapSizeInBytes;
	unsigned int cbHeapOffsetInBytes;

	// Shaders and shader-related constructs
	Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;

	std::vector<std::shared_ptr<Mesh>> meshes;
	std::vector<GameEntity> entities;
	std::vector<std::shared_ptr<Material>> materials;

	// Lights
	std::vector<Light> lights;

	// Shadows
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> shadowDSV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shadowSRV;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> shadowRasterizer;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> shadowSampler;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> shadowVS;
	DirectX::XMFLOAT4X4 lightViewMatrix;
	DirectX::XMFLOAT4X4 lightProjectionMatrix;

	// Sky
	std::shared_ptr<Sky> sky;

	// ImGUI fields
	std::vector<std::shared_ptr<Camera>> cameras;
	std::shared_ptr<Camera> activeCamera;
	float bgColor[4] = { 0.4f, 0.6f, 0.75f, 1.0f };
	float ambientColor[4] = { 0.1f, 0.15f, 0.25f, 1.0f };
	float colorTint[4] = {1.0f, 1.0f, 1.0f, 1.0f};
	float offset[3] = {};
	bool isDemoVisible = false;
	int activeIndex;
	int dragNum = 0;
};

