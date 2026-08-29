#include "shader_common.hlsli"

Texture2D<float4> Texture : register(t0, space2);
SamplerState Sampler : register(s0, space2);

cbuffer UniformBlockDropShadow : register(b0, space3) {
    float2 TexCoordMin : packoffset(c0);
    float2 TexCoordMax : packoffset(c0.z);
    float4 Color : packoffset(c1);
};

float4 main(PostVaryings input) : SV_Target0 {
    float2 in_region = step(TexCoordMin, input.TexCoord) * step(input.TexCoord, TexCoordMax);
    return Texture.Sample(Sampler, input.TexCoord).a * in_region.x * in_region.y * Color;
}
