#pragma once
#include <DirectXMath.h>

struct VertexShaderData
{
	DirectX::XMFLOAT4X4 worldMat;
	DirectX::XMFLOAT4X4 viewMat;
	DirectX::XMFLOAT4X4 projMat;
};

struct PixelShaderData
{
	DirectX::XMFLOAT4 colorTint;
	float time;
};