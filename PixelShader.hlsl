
#include "ShaderIncludes.hlsli"

Texture2D Albedo : register(t0);
Texture2D NormalMap : register(t1);
Texture2D RoughnessMap : register(t2);
Texture2D MetalnessMap : register(t3);

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
    input.uv = input.uv * uvScale + uvOffset;
    
    // ALBEDO
    float4 surfaceColor = pow(Albedo.Sample(BasicSampler, input.uv), 2.2f);
    
    // NORMAL MAP
    // unpack normal from texture
    float3 unpackedNormal = normalize(NormalMap.Sample(BasicSampler, input.uv).rgb * 2.0f - 1.0f);
    
    // create TBN matrix
    float3 N = normalize(input.normal);
    float3 T = normalize(input.tangent - dot(input.tangent, N) * N);
    float3 B = cross(N, T);
    float3x3 TBN = float3x3(T, B, N);
    
    // transform normal from texture
    float3 finalNormal = mul(unpackedNormal, TBN);
    input.normal = finalNormal;
    
    // ROUGHNESS MAP
    float roughness = RoughnessMap.Sample(BasicSampler, input.uv).r;
    
    // METALNESS MAP
    float metalness = MetalnessMap.Sample(BasicSampler, input.uv).r;
    
    // SPECULAR COLOR
    float3 specularColor = lerp(F0_NON_METAL, surfaceColor.rgb, metalness);
    
    // LIGHT CALCULATIONS
    float3 totalLight = 0;  
    for (int i = 0; i < 5; i++)
    {
        float3 lightCalc = 0;
        switch (lights[i].type)
        {
            case 0: // DIRECTIONAL
                lightCalc = DirectionLightCalc(lights[i], cameraPos, surfaceColor.rgb, roughness, metalness, specularColor, input);
                break;
            case 1: // POINT
                lightCalc = PointLightCalc(lights[i], cameraPos, surfaceColor.rgb, roughness, metalness, specularColor, input);
                break;
            case 2: // SPOT
                lightCalc = SpotLightCalc(lights[i], cameraPos, surfaceColor.rgb, roughness, metalness, specularColor, input);
                break;
                
        }
        totalLight += lightCalc;
    } 
    
    // gamma correction
    totalLight = pow(totalLight, 1.0f / 2.2f);
    
    //return float4(input.tangent, 1);
    return float4(totalLight, 1);
}