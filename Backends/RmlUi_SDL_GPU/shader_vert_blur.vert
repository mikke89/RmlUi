#include "shader_common.hlsli"

cbuffer UniformBlockBlur : register(b0, space1) {
    float2 TexelOffset : packoffset(c0);
};

// The offsets of the samples the fragment stage takes are the same for every fragment in a row, so they are computed
// here rather than once per fragment.
BlurVertexOutput main(VertexInput input) {
    BlurVertexOutput output;
    for (int i = 0; i < BLUR_SIZE; i++)
        output.TexCoord[i] = input.TexCoord - float(i - BLUR_NUM_WEIGHTS + 1) * TexelOffset;
    output.Position = float4(input.Position, 0, 1);
    return output;
}
