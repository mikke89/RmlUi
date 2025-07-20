
// NOTE: Seems like TEXCOORD0 is getting optimized out and TEXCOORD1 is taking its spot
// Might need to fix when later versions of SDL_shadercross preserve unused bindings
float4 main(float4 Color : TEXCOORD0) : SV_Target0 {
    return Color;
}