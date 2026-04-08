#pragma once

#include <d3d11.h>
#include <wrl/client.h>

#include "Mesh.h"
#include "Camera.h"

class Sky
{
public:
	Sky(std::shared_ptr<Mesh> _mesh, Microsoft::WRL::ComPtr<ID3D11SamplerState> _samplerState, Microsoft::WRL::ComPtr<ID3D11PixelShader> _pixelShader,
	Microsoft::WRL::ComPtr<ID3D11VertexShader> _vertexShader, 
		const wchar_t* right,
		const wchar_t* left,
		const wchar_t* up,
		const wchar_t* down,
		const wchar_t* front,
		const wchar_t* back);
	void Draw();

private:
	Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerState;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthStencilState;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizerState;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
	std::shared_ptr<Mesh> mesh;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> CreateCubemap(
		const wchar_t* right,
		const wchar_t* left,
		const wchar_t* up,
		const wchar_t* down,
		const wchar_t* front,
		const wchar_t* back);
};

