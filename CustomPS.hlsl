
cbuffer ExternalData : register(b0)
{
    float4 colorTint;
    float time;
}

struct VertexToPixel
{
    float4 screenPosition : SV_POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
};

float4 main(VertexToPixel input) : SV_TARGET
{
    return float4(((sin(time)) + 1) / 2.0f, abs(sin(input.uv.y * 25.0f)) * 0.6f, input.uv.y, 1);
}