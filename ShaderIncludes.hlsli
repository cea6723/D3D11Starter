#ifndef __GGP_SHADER_INCLUDES__
#define __GGP_SHADER_INCLUDES__

#define LIGHT_TYPE_DIRECTIONAL 0
#define LIGHT_TYPE_POINT       1
#define LIGHT_TYPE_SPOT        2

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

float3 DiffuseCalc(Light light, float3 lightDir, float3 normal)
{
    return saturate(dot(normal, -lightDir)) * light.color * light.intensity;
}

float3 SpecularCalc(Light light, float3 lightDir, float3 normal, float3 dirToCamera)
{
    float3 refl = reflect(lightDir, normal);
    float3 RdotV = saturate(dot(refl, dirToCamera));
    return pow(RdotV, 200) * light.color * light.intensity;
}

float Attenuate(Light light, float3 worldPos)
{
    float dist = distance(light.position, worldPos);
    float att = saturate(1.0f - (dist * dist / (light.range * light.range)));
    return att * att;
}

float3 DirectionLightCalc(Light light, float3 cameraPos, VertexToPixel input)
{
    float3 lightDir = normalize(light.direction);
    float3 dirToCamera = normalize(cameraPos - input.wordPos);
    
    float3 diffuseTerm = DiffuseCalc(light, lightDir, input.normal);
    float3 specularTerm = SpecularCalc(light, lightDir, input.normal, dirToCamera);
    
    specularTerm *= any(diffuseTerm);
    
    return diffuseTerm + specularTerm;
}

float3 PointLightCalc(Light light, float3 cameraPos, VertexToPixel input)
{
    float3 lightDir = normalize(input.wordPos - light.position);
    float3 dirToCamera = normalize(cameraPos - input.wordPos);
    
    float3 diffuseTerm = DiffuseCalc(light, lightDir, input.normal);
    float3 specularTerm = SpecularCalc(light, lightDir, input.normal, dirToCamera);
    
    specularTerm *= any(diffuseTerm);
    
    return (diffuseTerm + specularTerm) * Attenuate(light, input.wordPos);
}

float3 SpotLightCalc(Light light, float3 cameraPos, VertexToPixel input)
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
    return PointLightCalc(light, cameraPos, input) * spotTerm;
}

#endif
