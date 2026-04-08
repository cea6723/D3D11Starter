
#include "ShaderIncludes.hlsli"


cbuffer ExternalData : register(b0)
{
    matrix viewMat;
    matrix projMat;
}

SkyVertexToPixel main(VertexShaderInput input)
{
	// Set up output struct
    SkyVertexToPixel output;

    matrix viewNoTranslation = viewMat;
    viewNoTranslation._14 = 0;
    viewNoTranslation._24 = 0;
    viewNoTranslation._34 = 0;
    
    output.position = mul(mul(projMat, viewNoTranslation), float4(input.localPosition, 1.0f));
    output.position.z = output.position.w;
    
    output.sampleDir = input.localPosition;

	// Whatever we return will make its way through the pipeline to the
	// next programmable stage we're using (the pixel shader for now)
    return output;
}