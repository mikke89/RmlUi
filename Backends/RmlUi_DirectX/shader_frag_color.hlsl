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

float4 main(const sInputData inputArgs) : SV_TARGET 
{ 
	return inputArgs.inputColor; 
}