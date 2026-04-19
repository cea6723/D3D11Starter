#pragma once
#include <DirectXMath.h>
#include "Lights.h"

struct VertexShaderData
{
	DirectX::XMFLOAT4X4 worldMat;
	DirectX::XMFLOAT4X4 worldInvTransMat;
	DirectX::XMFLOAT4X4 viewMat;
	DirectX::XMFLOAT4X4 projMat;
	DirectX::XMFLOAT4X4 lightView;
	DirectX::XMFLOAT4X4 lightProj;
};

struct SkyVertexShaderData
{
	DirectX::XMFLOAT4X4 viewMat;
	DirectX::XMFLOAT4X4 projMat;
};

struct PixelShaderData
{
	DirectX::XMFLOAT4 colorTint;
	DirectX::XMFLOAT2 uvScale;
	DirectX::XMFLOAT2 uvOffset;
	DirectX::XMFLOAT3 cameraPos;
	float time;
	DirectX::XMFLOAT3 ambientColor;
	float pad;
	Light lights[5];
};