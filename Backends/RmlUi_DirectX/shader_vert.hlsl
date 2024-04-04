/*
	hlsl version of glsl shader
	author: wh1t3lord
*/

struct sInputData 
{
	float2 inPosition : POSITION;
	float4 inColor : COLOR;
	float2 inTexCoord : TEXCOORD;
};

struct sOutputData
{
	float4 outPosition : SV_Position;
	float4 outColor : COLOR;
	float2 outUV : TEXCOORD;
};

cbuffer ConstantBuffer : register(b0)
{
	float4x4 m_transform;
	float2 m_translate;
};

sOutputData main(const sInputData inArgs)
{
	sOutputData result;

	float2 translatedPos = inArgs.inPosition + m_translate;
	float4 resPos = mul(m_transform, float4(translatedPos.x, translatedPos.y, 0.0, 1.0));

	result.outPosition = resPos;
	result.outColor = inArgs.inColor;
	result.outUV = inArgs.inTexCoord;

#if RMLUI_PREMULTIPLIED_ALPHA
	// Pre-multiply vertex colors with their alpha.
	result.outColor.rgb = result.outColor.rgb * result.outColor.a;
#endif

	return result;
};