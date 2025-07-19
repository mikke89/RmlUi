cbuffer UniformBlock : register(b0, space1)
{
    float4x4 Transform : packoffset(c0);
    float2 Translate   : packoffset(c4);
};

struct Input {
    float4 Position  : TEXCOORD0;
    float2 TexCoord  : TEXCOORD1;
    float4 InColor   : COLOR0;
};

struct Output {
    float4 Position  : SV_Position;
    float2 TexCoord  : TEXCOORD0;
    float4 Color     : TEXCOORD1;
};

Output main(Input input) {
    Output output;
    output.TexCoord = input.TexCoord;
    output.Color = input.InColor;
    float4 position = float4(input.Position.xy + Translate, input.Position.z, input.Position.w);
    output.Position = mul(Transform, position);
    return output;
}