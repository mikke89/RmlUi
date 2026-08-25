#include "shader_common.hlsli"

cbuffer UniformBlockTransform : register(b0, space1) {
    float4x4 Transform : packoffset(c0);
};

cbuffer UniformBlockTranslate : register(b1, space1) {
    float2 Translate : packoffset(c0);
};

VertexOutput main(VertexInput input) {
    VertexOutput output;
    output.TexCoord = input.TexCoord;
    output.Color = input.Color;
    float4 position = float4(input.Position + Translate, 0, 1);
    output.Position = mul(Transform, position);
    return output;
}
