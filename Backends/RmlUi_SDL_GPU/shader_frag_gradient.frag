#include "shader_common.hlsli"

// Must match ShaderGradientFunction in the renderer.
#define LINEAR 0
#define RADIAL 1
#define CONIC 2
#define REPEATING_LINEAR 3
#define REPEATING_RADIAL 4
#define REPEATING_CONIC 5

cbuffer UniformBlockGradient : register(b0, space3) {
    float2 P;   // linear: starting point,         radial: center,                        conic: center
    float2 V;   // linear: vector to ending point, radial: 2d curvature (inverse radius), conic: angled unit vector
    int Func;   // one of the definitions above
    int NumStops;
    // The stop positions are packed four to a row, because every array element would otherwise start a new 16-byte row
    // and three quarters of the space would be padding.
    float4 StopColors[MAX_NUM_STOPS];
    float4 StopPositions[MAX_NUM_STOPS_PACKED]; // normalized, 0 -> starting point, 1 -> ending point
};

#define GET_STOP_POS(i) (StopPositions[(i) >> 2][(i) & 3])

float4 LerpStopColors(float t) {
    float4 color = StopColors[0];
    for (int i = 1; i < NumStops; i++)
        color = lerp(color, StopColors[i], smoothstep(GET_STOP_POS(i - 1), GET_STOP_POS(i), t));
    return color;
}

float4 main(Varyings input) : SV_Target0 {
    float t = 0.0;

    if (Func == LINEAR || Func == REPEATING_LINEAR) {
        float dist_square = dot(V, V);
        float2 delta = input.TexCoord - P;
        t = dot(V, delta) / dist_square;
    }
    else if (Func == RADIAL || Func == REPEATING_RADIAL) {
        float2 delta = input.TexCoord - P;
        t = length(V * delta);
    }
    else if (Func == CONIC || Func == REPEATING_CONIC) {
        float2x2 R = float2x2(V.x, -V.y, V.y, V.x);
        float2 delta = mul(input.TexCoord - P, R);
        t = 0.5 + atan2(-delta.x, delta.y) / (2.0 * PI);
    }

    if (Func == REPEATING_LINEAR || Func == REPEATING_RADIAL || Func == REPEATING_CONIC) {
        float t0 = GET_STOP_POS(0);
        float t1 = GET_STOP_POS(NumStops - 1);
        t = t0 + glsl_mod(t - t0, t1 - t0);
    }

    return input.Color * LerpStopColors(t);
}
