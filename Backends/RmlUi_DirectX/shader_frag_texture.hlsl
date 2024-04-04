/*
	hlsl version of glsl shader 
	author: wh1t3lord
*/


struct sInputData
{
	float4 inputPos : SV_Position;
	float4 inputColor : COLOR;
	float2 inputUV : TEXCOORD;
};

Texture2D g_InputTexture : register(t0);

SamplerState g_SamplerLinear
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Wrap;
	AddressV = Wrap;
};


float4 main(const sInputData inputArgs) : SV_TARGET 
{ 
	return inputArgs.inputColor * g_InputTexture.sample(g_SamplerLinear, inputArgs.inputUV); 
}