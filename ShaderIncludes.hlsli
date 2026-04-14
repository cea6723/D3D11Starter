#ifndef __GGP_SHADER_INCLUDES__
#define __GGP_SHADER_INCLUDES__

#define LIGHT_TYPE_DIRECTIONAL 0
#define LIGHT_TYPE_POINT       1
#define LIGHT_TYPE_SPOT        2

#define MIN_ROUGHNESS 0.0000001f
#define F0_NON_METAL 0.04f
#define PI 3.14159265359f

// code goes here
struct VertexShaderInput
{
    float3 localPosition : POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
};

struct VertexToPixel
{
    float4 screenPosition : SV_POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 wordPos : POSITION;
    float3 tangent : TANGENT;
};

struct SkyVertexToPixel
{
    float4 position : SV_POSITION;
    float3 sampleDir : DIRECTION;
};

struct Light
{
    int type;
    float3 direction;
    float range;
    float3 position;
    float intensity;
    float3 color;
    float spotInnerAngle;
    float spotOuterAngle;
    float2 padding;
};

float D_GGX(float3 n, float3 h, float roughness)
{
    float NdotH = saturate(dot(n, h));
    float NdotH2 = NdotH * NdotH;
    float a = roughness * roughness;
    float a2 = max(a * a, MIN_ROUGHNESS);
    
    float denomToSqaure = NdotH2 * (a2 - 1.0f) + 1.0f;
    return a2 / (PI * denomToSqaure * denomToSqaure);
}

float G_SchlickGGX(float3 n, float3 v, float roughness)
{
    float k = pow(roughness + 1.0f, 2) / 8.0f;
    float NdotV = saturate(dot(n, v));
    return 1.0f / (NdotV * (1.0f - k) + k);
}

float3 F_Schlick(float3 v, float3 h, float3 f0)
{
    float VdotH = saturate(dot(v, h));
    return f0 + (1.0f - f0) * pow(1.0f - VdotH, 5);
}

float3 DiffuseEnergyConserve(float3 diffuse, float3 F, float metalness)
{
    return diffuse * saturate(1.0f - F) * (1.0f - metalness);
}

// n: the normal (after normal mapping)
// l: the lightvector (normalized direction to thelight)
// v: the viewvector (normalized direction to thecamera)
// roughness: from the roughness map
// f0: specular colorfor this pixel
float3 MicrofacetBRDF(float3 n, float3 l, float3 v, float roughness, float3 f0, out float3 outF)
{
    float3 h = normalize(v + l);
    
    float D = D_GGX(n, h, roughness);
    outF = F_Schlick(v, h, f0);
    float G = G_SchlickGGX(n, v, roughness) * G_SchlickGGX(n, l, roughness);
    
    return saturate(dot(n, l)) * (D * outF * G) / 4.0f;
}

float3 DiffuseCalc(float3 lightDir, float3 normal)
{
    return saturate(dot(normal, lightDir));
}

float Attenuate(Light light, float3 worldPos)
{
    float dist = distance(light.position, worldPos);
    float att = saturate(1.0f - (dist * dist / (light.range * light.range)));
    return att * att;
}

float3 DirectionLightCalc(Light light, float3 cameraPos, float3 surfaceColor, float roughness, float metalness, float3 f0, VertexToPixel input)
{
    float3 toLight = normalize(-light.direction);
    float3 dirToCamera = normalize(cameraPos - input.wordPos);
    
    float3 F;
    float3 diffuseTerm = DiffuseCalc(toLight, input.normal);
    float3 specularTerm = MicrofacetBRDF(input.normal, toLight, dirToCamera, roughness, f0, F);
    
    diffuseTerm = DiffuseEnergyConserve(diffuseTerm, F, metalness);
    
    return (diffuseTerm * surfaceColor + specularTerm) * light.intensity * light.color;
}

float3 PointLightCalc(Light light, float3 cameraPos, float3 surfaceColor, float roughness, float metalness, float3 f0, VertexToPixel input)
{
    float3 toLight = normalize(light.position - input.wordPos);
    float3 dirToCamera = normalize(cameraPos - input.wordPos);
    
    float3 F;
    float3 diffuseTerm = DiffuseCalc(toLight, input.normal);
    float3 specularTerm = MicrofacetBRDF(input.normal, toLight, dirToCamera, roughness, f0, F);
    
    diffuseTerm = DiffuseEnergyConserve(diffuseTerm, F, metalness);
    
    return (diffuseTerm * surfaceColor + specularTerm) * light.intensity * light.color * Attenuate(light, input.wordPos);
}

float3 SpotLightCalc(Light light, float3 cameraPos, float3 surfaceColor, float roughness, float metalness, float3 f0, VertexToPixel input)
{
    float3 lightToPixel = normalize(input.wordPos - light.position);
    float3 lightDir = normalize(light.direction);
    
    // Get cos(angle) between pixel and light direction
    float pixelAngle = saturate(dot(lightToPixel, lightDir));
    // Get cosines of angles and calculate range
    float cosOuter = cos(light.spotOuterAngle);
    float cosInner = cos(light.spotInnerAngle);
    float falloffRange = cosOuter - cosInner;
    // Linear falloff over the range, clamp 0-1, apply to light calc
    float spotTerm = saturate((cosOuter - pixelAngle) / falloffRange);
    return PointLightCalc(light, cameraPos, surfaceColor, roughness, metalness, f0, input) * spotTerm;
}

#endif
