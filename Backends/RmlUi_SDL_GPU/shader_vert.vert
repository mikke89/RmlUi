struct Input {
    float3 Position : TEXCOORD0;
    float2 TexCoord : TEXCOORD1;
};

struct Output {
    float2 TexCoord : TEXCOORD0;
    float4 Position : SV_Position;
};

Output main(Input input) {
    Output output;
    output.TexCoord = input.TexCoord;
    output.Position = float4(input.Position, 1.0f);
    return output;
}