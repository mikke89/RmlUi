cbuffer UniformBlockTransform : register(b0, space1) {
    float4x4 Transform : packoffset(c0);
};

cbuffer UniformBlockTranslate : register(b1, space1) {
    float2 Translate : packoffset(c0);
};

struct Input {
    float2 Position : TEXCOORD0;
    float4 InColor : TEXCOORD1;
    float2 TexCoord : TEXCOORD2;
};

struct Output {
    float4 Color : TEXCOORD0;
    float2 TexCoord : TEXCOORD1;
    float4 Position : SV_Position;
};

Output main(Input input) {
    Output output;
    output.TexCoord = input.TexCoord;
    output.Color = input.InColor;
    float4 position = float4(input.Position + Translate, 0, 1);
    output.Position = mul(Transform, position);
    return output;
}