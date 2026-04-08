
#include "ShaderIncludes.hlsli"

TextureCube SkyTexture: register(t0);
SamplerState BasicSampler : register(s0);

cbuffer ExternalData : register(b0)
{
    float4 colorTint;
    float2 uvScale;
    float2 uvOffset;
    float3 cameraPos;
    float time;
    float3 ambientColor;
    float pad;
    Light lights[5];
}

float4 main(SkyVertexToPixel input) : SV_TARGET
{
    return SkyTexture.Sample(BasicSampler, input.sampleDir);
}