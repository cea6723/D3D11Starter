#include "Material.h"
#include "Graphics.h"

Material::Material(const char* _name, DirectX::XMFLOAT4 _colorTint, Microsoft::WRL::ComPtr<ID3D11VertexShader> _vertexShader, Microsoft::WRL::ComPtr<ID3D11PixelShader> _pixelShader)
{
    name = _name;
    colorTint = _colorTint;
    vertexShader = _vertexShader;
    pixelShader = _pixelShader;
}

const char* Material::GetName()
{
    return name;
}

DirectX::XMFLOAT4 Material::GetColorTint()
{
    return colorTint;
}

DirectX::XMFLOAT2 Material::GetUVScale()
{
    return uvScale;
}

DirectX::XMFLOAT2 Material::GetUVOffset()
{
    return uvOffset;
}

Microsoft::WRL::ComPtr<ID3D11VertexShader> Material::GetVertexShader()
{
    return vertexShader;
}

Microsoft::WRL::ComPtr<ID3D11PixelShader> Material::GetPixelShader()
{
    return pixelShader;
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Material::GetSRV(unsigned int index)
{
    return textureSRVs[index];
}

int Material::GetMaxSRVIndex()
{
    return maxSRVIndex;
}

void Material::SetColorTint(DirectX::XMFLOAT4 _colorTint)
{
    colorTint = _colorTint;
}

void Material::SetUVScale(DirectX::XMFLOAT2 _uvScale)
{
    uvScale = _uvScale;
}

void Material::SetUVOffset(DirectX::XMFLOAT2 _uvOffset)
{
    uvOffset = _uvOffset;
}

void Material::SetVertexShader(Microsoft::WRL::ComPtr<ID3D11VertexShader> _vertexShader)
{
    vertexShader = _vertexShader;
}

void Material::SetPixelShader(Microsoft::WRL::ComPtr<ID3D11PixelShader> _pixelShader)
{
    pixelShader = _pixelShader;
}

void Material::AddTextureSRV(unsigned int index, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv)
{
    textureSRVs[index] = srv;
    if (index > maxSRVIndex) maxSRVIndex = index;
}

void Material::AddSampler(unsigned int index, Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler)
{
    samplers[index] = sampler;
}

void Material::BindTexturesAndSamplers()
{
    for (int i = 0; i < 128; i++)
    {
        Graphics::Context->PSSetShaderResources(i, 1, textureSRVs[i].GetAddressOf());
    }

    for (int i = 0; i < 16; i++)
    {
        Graphics::Context->PSSetSamplers(i, 1, samplers[i].GetAddressOf());
    }
}
