#include "shader_common.hlsli"

// The quad to draw: the two corners in clip space, then the texture coordinate at the first corner
// followed by the span across the quad. Both are per-pass, which is why they are constants and not
// vertex data -- see DrawPostprocessQuad() in the renderer.
cbuffer UniformBlockQuad : register(b0, space1) {
    float4 QuadPosition : packoffset(c0);
    float4 QuadTexCoord : packoffset(c1);
};

// Draws the quad given by the constant buffer, for the postprocess passes. The vertex buffer carries nothing but the
// corner each vertex stands for, in its texture coordinate, so every pass draws the same four vertices however small
// a rectangle of the target it covers.
PostVertexOutput main(VertexInput input) {
    PostVertexOutput output;
    output.TexCoord = QuadTexCoord.xy + input.TexCoord * QuadTexCoord.zw;
    output.Position = float4(lerp(QuadPosition.xy, QuadPosition.zw, input.TexCoord), 0, 1);
    return output;
}
