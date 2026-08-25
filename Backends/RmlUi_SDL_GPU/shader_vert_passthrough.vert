#include "shader_common.hlsli"

// Draws a quad given directly in clip space, for the postprocess passes.
PostVertexOutput main(VertexInput input) {
    PostVertexOutput output;
    output.TexCoord = input.TexCoord;
    output.Position = float4(input.Position, 0, 1);
    return output;
}
