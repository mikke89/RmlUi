Texture2D<float4> Texture : register(t0, space2);
SamplerState Sampler : register(s0, space2);

// NOTE: Seems like TEXCOORD0 is getting optimized out and TEXCOORD1 is taking its spot
// Might need to fix when later versions of SDL_shadercross preserve unused bindings
float4 main(float4 Color : TEXCOORD0, float2 TexCoord : TEXCOORD1) : SV_Target0 {
    return Color * Texture.Sample(Sampler, TexCoord);
}