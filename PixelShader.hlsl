
#include "ShaderIncludes.hlsli"

Texture2D SurfaceTexture : register(t0);
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

// --------------------------------------------------------
// The entry point (main method) for our pixel shader
// 
// - Input is the data coming down the pipeline (defined by the struct)
// - Output is a single color (float4)
// - Has a special semantic (SV_TARGET), which means 
//    "put the output of this into the current render target"
// - Named "main" because that's the default the shader compiler looks for
// --------------------------------------------------------
float4 main(VertexToPixel input) : SV_TARGET
{
    input.normal = normalize(input.normal);
    input.uv = input.uv * uvScale + uvOffset;
    
    float4 surfaceColor = SurfaceTexture.Sample(BasicSampler, input.uv);
    float3 ambientTerm = ambientColor.rgbr * surfaceColor * colorTint;
    
    float3 totalLight;
    
    for (int i = 0; i < 5; i++)
    {
        float3 lightCalc;
        switch (lights[i].type)
        {
            case 0: // DIRECTIONAL
                lightCalc = DirectionLightCalc(lights[i], cameraPos, input);
                break;
            case 1: // POINT
                lightCalc = PointLightCalc(lights[i], cameraPos, input);
                break;
            case 2: // SPOT
                lightCalc = SpotLightCalc(lights[i], cameraPos, input);
                break;
                
        }
        totalLight += lightCalc;
    }
    
    totalLight *= surfaceColor.rgb;
    totalLight += ambientTerm;
    
    return float4(totalLight, 1);
}