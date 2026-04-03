
#include "ShaderIncludes.hlsli"

float4 main(VertexToPixel input) : SV_TARGET
{
    return float4(input.uv, 0, 1);
}