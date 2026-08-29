#include "shader_common.hlsli"

float4 main(Varyings input) : SV_Target0 {
    return input.Color;
}
