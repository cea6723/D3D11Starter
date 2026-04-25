cbuffer externalData : register(b0)
{
    float redOffset;
    float greenOffset;
    float blueOffset;
}
struct VertexToPixel
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

Texture2D Pixels : register(t0);
SamplerState ClampSampler : register(s0);

float4 main(VertexToPixel input) : SV_TARGET
{ 
    float2 direction = input.uv - float2(0.5f, 0.5f);
    
    float red = Pixels.Sample(ClampSampler, input.uv + (direction * redOffset)).r;
    float green = Pixels.Sample(ClampSampler, input.uv + (direction * greenOffset)).g;
    float blue = Pixels.Sample(ClampSampler, input.uv + (direction * blueOffset)).b;
    
    return float4(red, green, blue, 1.0f); 
}