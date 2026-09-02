// Gradient fragment shader for RmlUi's linear-gradient/radial-gradient/conic-gradient decorators
// (and their repeating variants). Ported from RmlUi's own OpenGL reference backend's GLSL source
// (RmlUi_Renderer_GL3.cpp's shader_frag_gradient, lines 96-155 as of this port) to the HLSL this
// SDL_GPU backend's other shaders use (see shader_frag_texture.frag for the input signature this
// mirrors, and shader_vert.vert for the vertex stage this shares unchanged -- no new vertex stage
// needed, only this fragment shader plus its own uniform buffer).
//
// Uniform layout below uses explicit packoffset (matches this backend's existing convention in
// shader_vert.vert) so the C++-side struct in RenderInterface_SDL_GPU.cpp can mirror it exactly:
//   c0.x  = Func (int)         c0.y = NumStops (int)      c0.zw = P (float2)
//   c1.xy = V (float2)         c1.zw = unused padding
//   c2..c17  = StopColors[16], one float4 per register
//   c18..c33 = StopPositions[16], one float per register (rest of each register unused --
//              HLSL cbuffer arrays always occupy a full 16-byte slot per element)
#define MAX_NUM_STOPS 16

cbuffer GradientUniforms : register(b0, space3) {
    int Func         : packoffset(c0.x);
    int NumStops     : packoffset(c0.y);
    float2 P         : packoffset(c0.z);
    float2 V         : packoffset(c1.x);
    float4 StopColors[MAX_NUM_STOPS]    : packoffset(c2);
    float  StopPositions[MAX_NUM_STOPS] : packoffset(c18);
};

#define LINEAR 0
#define RADIAL 1
#define CONIC 2
#define REPEATING_LINEAR 3
#define REPEATING_RADIAL 4
#define REPEATING_CONIC 5
#define PI 3.14159265

// GLSL's mod(x, y) is x - y*floor(x/y) (always same sign as y) -- HLSL's fmod() instead follows
// C's fmod (same sign as x), which gives the wrong wrap direction for repeating gradients with
// t < t0. Reimplement GLSL's version explicitly to keep the repeating-gradient math a faithful
// port instead of an approximation.
float GlslMod(float x, float y) {
    return x - y * floor(x / y);
}

float4 MixStopColors(float t) {
    float4 color = StopColors[0];
    for (int i = 1; i < NumStops; i++)
        color = lerp(color, StopColors[i], smoothstep(StopPositions[i - 1], StopPositions[i], t));
    return color;
}

float4 main(float4 Color : TEXCOORD0, float2 TexCoord : TEXCOORD1) : SV_Target0 {
    float t = 0.0;

    if (Func == LINEAR || Func == REPEATING_LINEAR) {
        float distSquare = dot(V, V);
        float2 Vc = TexCoord - P;
        t = dot(V, Vc) / distSquare;
    }
    else if (Func == RADIAL || Func == REPEATING_RADIAL) {
        float2 Vc = TexCoord - P;
        t = length(V * Vc);
    }
    else if (Func == CONIC || Func == REPEATING_CONIC) {
        // Rotate (TexCoord - P) by the angle encoded in V, written out as scalar math instead of
        // a float2x2 constructor + mul() to sidestep HLSL/GLSL's opposite default matrix-major
        // conventions -- this is the exact arithmetic GLSL's `mat2(V.x,-V.y,V.y,V.x) * Vc` expands
        // to, verified by hand rather than trusted to compiler-specific matrix layout defaults.
        float2 Vc = TexCoord - P;
        float2 Vr;
        Vr.x = V.x * Vc.x + V.y * Vc.y;
        Vr.y = -V.y * Vc.x + V.x * Vc.y;
        t = 0.5 + atan2(-Vr.x, Vr.y) / (2.0 * PI);
    }

    if (Func == REPEATING_LINEAR || Func == REPEATING_RADIAL || Func == REPEATING_CONIC) {
        float t0 = StopPositions[0];
        float t1 = StopPositions[NumStops - 1];
        t = t0 + GlslMod(t - t0, t1 - t0);
    }

    return Color * MixStopColors(t);
}
