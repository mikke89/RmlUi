#include "shader_common.hlsli"

// The quad to draw: the two corners in clip space, then the texture coordinate at the first corner
// followed by the span across the quad. Both are per-pass, which is why they are constants and not
// vertex data -- see DrawPostprocessQuad() in the renderer.
cbuffer UniformBlockBlur : register(b0, space1) {
    float4 QuadPosition : packoffset(c0);
    float4 QuadTexCoord : packoffset(c1);
    float2 TexelOffset : packoffset(c2);
};

// The offsets of the samples the fragment stage takes are the same for every fragment in a row, so they are computed
// here rather than once per fragment. The quad itself comes from the constant buffer, as in the passthrough stage.
BlurVertexOutput main(VertexInput input) {
    float2 tex_coord = QuadTexCoord.xy + input.TexCoord * QuadTexCoord.zw;
    BlurVertexOutput output;
    for (int i = 0; i < BLUR_SIZE; i++)
        output.TexCoord[i] = tex_coord - float(i - BLUR_NUM_WEIGHTS + 1) * TexelOffset;
    output.Position = float4(lerp(QuadPosition.xy, QuadPosition.zw, input.TexCoord), 0, 1);
    return output;
}
