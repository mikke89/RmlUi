#include "shader_common.hlsli"

Texture2D<float4> Texture : register(t0, space2);
Texture2D<float4> MaskTexture : register(t1, space2);
SamplerState Sampler : register(s0, space2);
SamplerState MaskSampler : register(s1, space2);

float4 main(PostVaryings input) : SV_Target0 {
    float4 tex_color = Texture.Sample(Sampler, input.TexCoord);
    float mask_alpha = MaskTexture.Sample(MaskSampler, input.TexCoord).a;
    return tex_color * mask_alpha;
}
