#pragma once

#include <DirectXMath.h>
#include <wrl/client.h>
#include <d3d11.h>

class Material
{
private:
	const char* name;

	DirectX::XMFLOAT4 colorTint;
	DirectX::XMFLOAT2 uvScale = DirectX::XMFLOAT2(1, 1);
	DirectX::XMFLOAT2 uvOffset = DirectX::XMFLOAT2(0, 0);

	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> textureSRVs[128];
	Microsoft::WRL::ComPtr<ID3D11SamplerState> samplers[16];

	unsigned int maxSRVIndex = 0;

public:
	Material(const char* _name, DirectX::XMFLOAT4 _colorTint, Microsoft::WRL::ComPtr<ID3D11VertexShader> _vertexShader, Microsoft::WRL::ComPtr<ID3D11PixelShader> _pixelShader);

	const char* GetName();

	DirectX::XMFLOAT4 GetColorTint();
	DirectX::XMFLOAT2 GetUVScale();
	DirectX::XMFLOAT2 GetUVOffset();

	Microsoft::WRL::ComPtr<ID3D11VertexShader> GetVertexShader();
	Microsoft::WRL::ComPtr<ID3D11PixelShader> GetPixelShader();

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetSRV(unsigned int index);
	int GetMaxSRVIndex();

	void SetColorTint(DirectX::XMFLOAT4 _colorTint);
	void SetUVScale(DirectX::XMFLOAT2 _uvScale);
	void SetUVOffset(DirectX::XMFLOAT2 _uvOffset);

	void SetVertexShader(Microsoft::WRL::ComPtr<ID3D11VertexShader> _vertexShader);
	void SetPixelShader(Microsoft::WRL::ComPtr<ID3D11PixelShader> _pixelShader);

	void AddTextureSRV(unsigned int index, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv);
	void AddSampler(unsigned int index, Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler);
	void BindTexturesAndSamplers();
};

