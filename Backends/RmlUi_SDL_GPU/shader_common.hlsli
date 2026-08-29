// Definitions shared by every RmlUi SDL GPU shader.
//
// The varying structures live here rather than in each shader on purpose: keeping one declaration for each interface
// makes it plain that the two stages agree.
//
// SDL_shadercross drops varyings a shader does not read, and what that costs depends on the target. SPIR-V and MSL key
// the location off the semantic index, so a dropped varying leaves the rest where they were. DXIL does not -- DXC
// repacks the signature and the survivors slide down into the freed registers, which D3D12 then refuses to link
// against a vertex shader that put them elsewhere. So an interface must carry only what every shader using it reads:
// that is why the postprocess stages have a pair of their own rather than sharing Varyings.
//
// Resource spaces are fixed by SDL GPU: the vertex stage takes textures and samplers from space0 and uniform buffers
// from space1, the fragment stage takes them from space2 and space3.

#ifndef RMLUI_SDL_GPU_SHADER_COMMON
#define RMLUI_SDL_GPU_SHADER_COMMON

// Mirrored in the renderer; see the constants next to GradientUniforms in RmlUi_Renderer_SDL_GPU.h.
#define MAX_NUM_STOPS 16
// Rounded up so that every stop position can be addressed as an element of a float4.
#define MAX_NUM_STOPS_PACKED ((MAX_NUM_STOPS + 3) / 4)

#define BLUR_SIZE 7
#define BLUR_NUM_WEIGHTS ((BLUR_SIZE + 1) / 2)

#define PI 3.14159265

// HLSL's fmod truncates towards zero where GLSL's mod floors; the shaders were written against the latter.
#define glsl_mod(x, y) ((x) - (y) * floor((x) / (y)))

// The vertex format RmlUi submits.
struct VertexInput {
	float2 Position : TEXCOORD0;
	float4 Color : TEXCOORD1;
	float2 TexCoord : TEXCOORD2;
};

// What the main and passthrough vertex shaders hand to the fragment stage.
struct Varyings {
	float4 Color : TEXCOORD0;
	float2 TexCoord : TEXCOORD1;
};

struct VertexOutput {
	float4 Color : TEXCOORD0;
	float2 TexCoord : TEXCOORD1;
	float4 Position : SV_Position;
};

// What the postprocess stages exchange. A full-target quad carries no per-vertex colour, and none of the fragment
// shaders reading this want one, so TexCoord takes the first semantic and nothing can be dropped out from under it.
struct PostVaryings {
	float2 TexCoord : TEXCOORD0;
};

struct PostVertexOutput {
	float2 TexCoord : TEXCOORD0;
	float4 Position : SV_Position;
};

// Blur reads the same texel row several times, so the offsets are computed once per vertex. The array takes semantics
// TEXCOORD0 through TEXCOORD6, which is why this interface carries no colour.
struct BlurVaryings {
	float2 TexCoord[BLUR_SIZE] : TEXCOORD0;
};

struct BlurVertexOutput {
	float2 TexCoord[BLUR_SIZE] : TEXCOORD0;
	float4 Position : SV_Position;
};

#endif
