#include "shader_common.hlsli"

Texture2D<float4> Texture : register(t0, space2);
SamplerState Sampler : register(s0, space2);

cbuffer UniformBlockBlur : register(b0, space3) {
    float4 Weights : packoffset(c0);
    float2 TexCoordMin : packoffset(c1);
    float2 TexCoordMax : packoffset(c1.z);
};

float4 main(BlurVaryings input) : SV_Target0 {
    float4 color = float4(0.0, 0.0, 0.0, 0.0);
    for (int i = 0; i < BLUR_SIZE; i++) {
        float2 in_region = step(TexCoordMin, input.TexCoord[i]) * step(input.TexCoord[i], TexCoordMax);
        color += Texture.Sample(Sampler, input.TexCoord[i]) * in_region.x * in_region.y * Weights[abs(i - BLUR_NUM_WEIGHTS + 1)];
    }
    return color;
}
