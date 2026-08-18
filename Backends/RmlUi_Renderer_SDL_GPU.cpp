#include "RmlUi_Renderer_SDL_GPU.h"
#include "RmlUi_SDL_GPU/compiled/ShadersCompiled.h"
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/DecorationTypes.h>
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Core/Log.h>
#include <RmlUi/Core/Math.h>
#include <RmlUi/Core/SystemInterface.h>
#include <RmlUi/Core/Types.h>
#include <SDL3_image/SDL_image.h>
#include <algorithm>
#include <cstring>

// Debug groups are only emitted for a device created in debug mode, and cost a string push per frame otherwise.
#ifndef RMLUI_BACKEND_SDL_GPU_DEBUG
	#define RMLUI_BACKEND_SDL_GPU_DEBUG false
#endif

using namespace Rml;

enum ShaderFormat {
	ShaderFormatSPIRV,
	ShaderFormatMSL,
	ShaderFormatDXIL,
	ShaderFormatCount,
};

struct ShaderDefinition {
	// The ShaderId this row stands for, as an int because the enum is private to the renderer.
	int id;
	Span<const byte> data[ShaderFormatCount];
	// SDL takes the application's word for these rather than deriving them from the blob. They come from the
	// reflection compile_shaders.py can emit (`-d json`).
	int uniforms;
	int samplers;
	SDL_GPUShaderStage stage;
};

#undef X
#define X(name)            \
	Span<const byte>       \
	{                      \
		name, sizeof(name) \
	}
// Metal Shading Language is text, and is stored as a string literal rather than a byte array -- see
// compile_shaders.py. The terminator is not part of the shader, and SDL takes the length rather than looking for one,
// so it has to come off here.
#define X_TEXT(name)                                            \
	Span<const byte>                                            \
	{                                                           \
		reinterpret_cast<const byte*>(name), sizeof(name) - 1   \
	}
#define SHADER_BLOBS(name) {X(name##_spirv), X_TEXT(name##_msl), X(name##_dxil)}
#define SHADER_DEF(id, name, uniforms, samplers, stage) \
	{static_cast<int>(ShaderId::id), SHADER_BLOBS(name), uniforms, samplers, stage}

static SDL_GPUShader* CreateShaderFromMemory(SDL_GPUDevice* device, const ShaderDefinition& shader)
{
	SDL_GPUShaderFormat sdl_shader_format = SDL_GetGPUShaderFormats(device);
	ShaderFormat format = ShaderFormatCount;
	const char* entrypoint = nullptr;
	if (sdl_shader_format & SDL_GPU_SHADERFORMAT_SPIRV)
	{
		sdl_shader_format = SDL_GPU_SHADERFORMAT_SPIRV;
		format = ShaderFormatSPIRV;
		entrypoint = "main";
	}
	else if (sdl_shader_format & SDL_GPU_SHADERFORMAT_DXIL)
	{
		sdl_shader_format = SDL_GPU_SHADERFORMAT_DXIL;
		format = ShaderFormatDXIL;
		entrypoint = "main";
	}
	else if (sdl_shader_format & SDL_GPU_SHADERFORMAT_MSL)
	{
		sdl_shader_format = SDL_GPU_SHADERFORMAT_MSL;
		format = ShaderFormatMSL;
		entrypoint = "main0";
	}
	else
	{
		RMLUI_ERRORMSG("Invalid shader format");
		return nullptr;
	}
	SDL_GPUShaderCreateInfo info{};
	info.code = static_cast<const Uint8*>(shader.data[format].data());
	info.code_size = shader.data[format].size();
	info.entrypoint = entrypoint;
	info.format = sdl_shader_format;
	info.stage = shader.stage;
	info.num_samplers = shader.samplers;
	info.num_uniform_buffers = shader.uniforms;
	SDL_GPUShader* sdl_shader = SDL_CreateGPUShader(device, &info);
	if (!sdl_shader)
		Log::Message(Log::LT_ERROR, "Failed to create shader: %s", SDL_GetError());
	return sdl_shader;
}

static Colourf ConvertToColorf(ColourbPremultiplied c0)
{
	Colourf result;
	for (int i = 0; i < 4; i++)
		result[i] = (1.f / 255.f) * float(c0[i]);
	return result;
}

// -- Buffer pool -------------------------------------------------------------

// Returns the smallest bucket that can hold the given size, or -1 if the size exceeds the largest bucket.
static int BucketForSize(uint32_t size, uint32_t min_size, int num_buckets)
{
	uint32_t capacity = min_size;
	for (int bucket = 0; bucket < num_buckets; bucket++)
	{
		if (capacity >= size)
			return bucket;
		capacity <<= 1;
	}
	return -1;
}

void RenderInterface_SDL_GPU::BufferPool::Initialize(SDL_GPUDevice* in_device, SDL_GPUBufferUsageFlags in_usage, const char* in_debug_name)
{
	device = in_device;
	usage = in_usage;
	debug_name = in_debug_name;
}

void RenderInterface_SDL_GPU::BufferPool::ReleaseAll()
{
	for (const UniquePtr<Buffer>& buffer : buffers)
	{
		SDL_ReleaseGPUTransferBuffer(device, buffer->transfer_buffer);
		SDL_ReleaseGPUBuffer(device, buffer->buffer);
	}
	buffers.clear();
	for (Vector<Buffer*>& free_list : free_lists)
		free_list.clear();
}

RenderInterface_SDL_GPU::Buffer* RenderInterface_SDL_GPU::BufferPool::Acquire(uint32_t size, int frame)
{
	const int bucket = BucketForSize(size, min_buffer_size, num_buffer_buckets);
	if (bucket < 0)
	{
		Log::Message(Log::LT_ERROR, "Geometry of %u bytes exceeds the largest supported buffer size", size);
		return nullptr;
	}

	Vector<Buffer*>& free_list = free_lists[bucket];
	if (!free_list.empty())
	{
		Buffer* buffer = free_list.back();
		free_list.pop_back();
		buffer->last_used_frame = frame;
		return buffer;
	}

	const uint32_t capacity = min_buffer_size << bucket;

	UniquePtr<Buffer> buffer = MakeUnique<Buffer>();
	{
		SDL_GPUTransferBufferCreateInfo info{};
		info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
		info.size = capacity;
		buffer->transfer_buffer = SDL_CreateGPUTransferBuffer(device, &info);
	}
	{
		SDL_GPUBufferCreateInfo info{};
		info.usage = usage;
		info.size = capacity;
		buffer->buffer = SDL_CreateGPUBuffer(device, &info);
	}
	if (!buffer->transfer_buffer || !buffer->buffer)
	{
		Log::Message(Log::LT_ERROR, "Failed to create buffer(s): %s", SDL_GetError());
		SDL_ReleaseGPUTransferBuffer(device, buffer->transfer_buffer);
		SDL_ReleaseGPUBuffer(device, buffer->buffer);
		return nullptr;
	}

	SDL_SetGPUBufferName(device, buffer->buffer, debug_name);

	buffer->capacity = capacity;
	buffer->bucket = bucket;
	buffer->last_used_frame = frame;

	buffers.push_back(std::move(buffer));
	return buffers.back().get();
}

void RenderInterface_SDL_GPU::BufferPool::Release(Buffer* buffer)
{
	RMLUI_ASSERT(buffer);
	free_lists[buffer->bucket].push_back(buffer);
}

void RenderInterface_SDL_GPU::BufferPool::Trim(int frame)
{
	// Buffers are marked in place rather than collected into a list, so that the sweep stays linear in the pool size
	// even when a whole document's worth of geometry expires at once.
	bool any_expired = false;

	for (Vector<Buffer*>& free_list : free_lists)
	{
		auto it = std::remove_if(free_list.begin(), free_list.end(), [&](Buffer* buffer) {
			// A buffer whose upload has not been flushed is still referenced by the pending list.
			if (buffer->pending_upload >= 0 || frame - buffer->last_used_frame <= buffer_retention_frames)
				return false;
			buffer->expired = true;
			any_expired = true;
			return true;
		});
		free_list.erase(it, free_list.end());
	}

	if (!any_expired)
		return;

	for (const UniquePtr<Buffer>& buffer : buffers)
	{
		if (!buffer->expired)
			continue;
		SDL_ReleaseGPUTransferBuffer(device, buffer->transfer_buffer);
		SDL_ReleaseGPUBuffer(device, buffer->buffer);
	}

	buffers.erase(std::remove_if(buffers.begin(), buffers.end(), [](const UniquePtr<Buffer>& buffer) { return buffer->expired; }),
		buffers.end());
}

// -- Render targets ----------------------------------------------------------

static SDL_GPUTextureFormat SelectDepthStencilFormat(SDL_GPUDevice* device)
{
	const SDL_GPUTextureFormat candidates[] = {SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT, SDL_GPU_TEXTUREFORMAT_D32_FLOAT_S8_UINT};
	for (SDL_GPUTextureFormat format : candidates)
	{
		if (SDL_GPUTextureSupportsFormat(device, format, SDL_GPU_TEXTURETYPE_2D, SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET))
			return format;
	}
	return SDL_GPU_TEXTUREFORMAT_INVALID;
}

void RenderInterface_SDL_GPU::RenderLayerStack::Initialize(SDL_GPUDevice* in_device)
{
	device = in_device;
	supported_depth_stencil_format = SelectDepthStencilFormat(device);
	depth_stencil_format = supported_depth_stencil_format;
	if (depth_stencil_format == SDL_GPU_TEXTUREFORMAT_INVALID)
		Log::Message(Log::LT_WARNING, "No supported depth/stencil format found, clip masks will be unavailable");
}

bool RenderInterface_SDL_GPU::RenderLayerStack::CreateTarget(RenderTarget& target, const char* debug_name)
{
	// Before the first frame the stack has no size yet. RmlUi can still ask for a layer then, if the backend skipped
	// the frame; there is nothing to create and nothing to complain about.
	if (width <= 0 || height <= 0)
		return false;

	SDL_GPUTextureCreateInfo info{};
	info.type = SDL_GPU_TEXTURETYPE_2D;
	info.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
	info.format = layer_format;
	info.width = static_cast<Uint32>(width);
	info.height = static_cast<Uint32>(height);
	info.layer_count_or_depth = 1;
	info.num_levels = 1;

	target.color = SDL_CreateGPUTexture(device, &info);
	if (!target.color)
	{
		Log::Message(Log::LT_ERROR, "Failed to create render target: %s", SDL_GetError());
		return false;
	}

	SDL_SetGPUTextureName(device, target.color, debug_name);
	target.width = width;
	target.height = height;
	return true;
}

void RenderInterface_SDL_GPU::RenderLayerStack::DestroyTargets()
{
	for (RenderTarget& target : layers)
		SDL_ReleaseGPUTexture(device, target.color);
	for (RenderTarget& target : postprocess)
	{
		SDL_ReleaseGPUTexture(device, target.color);
		target = {};
	}
	layers.clear();
	layers_size = 0;

	if (depth_stencil)
	{
		SDL_ReleaseGPUTexture(device, depth_stencil);
		depth_stencil = nullptr;
	}
}

void RenderInterface_SDL_GPU::RenderLayerStack::ReleaseAll()
{
	DestroyTargets();
	width = 0;
	height = 0;
}

void RenderInterface_SDL_GPU::RenderLayerStack::BeginFrame(int in_width, int in_height)
{
	RMLUI_ASSERT(layers_size == 0);

	if (in_width != width || in_height != height)
	{
		DestroyTargets();
		width = in_width;
		height = in_height;

		// Every target is being built afresh, so an allocation that failed last time gets another go. Without this
		// the format below latches to INVALID on the first failure and clip masks never come back.
		depth_stencil_format = supported_depth_stencil_format;

		if (depth_stencil_format != SDL_GPU_TEXTUREFORMAT_INVALID)
		{
			SDL_GPUTextureCreateInfo info{};
			info.type = SDL_GPU_TEXTURETYPE_2D;
			info.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
			info.format = depth_stencil_format;
			info.width = static_cast<Uint32>(width);
			info.height = static_cast<Uint32>(height);
			info.layer_count_or_depth = 1;
			info.num_levels = 1;
			depth_stencil = SDL_CreateGPUTexture(device, &info);
			if (!depth_stencil)
			{
				Log::Message(Log::LT_ERROR, "Failed to create depth/stencil target: %s", SDL_GetError());
				// Whether a pass carries the attachment and whether a pipeline declares one have to be the same
				// question, or the two disagree and every draw becomes invalid. Drop the format so both say no.
				depth_stencil_format = SDL_GPU_TEXTUREFORMAT_INVALID;
			}
		}
	}

	PushLayer();
}

void RenderInterface_SDL_GPU::RenderLayerStack::EndFrame()
{
	RMLUI_ASSERT(layers_size == 1);
	PopLayer();
}

Rml::LayerHandle RenderInterface_SDL_GPU::RenderLayerStack::PushLayer()
{
	RMLUI_ASSERT(layers_size <= static_cast<int>(layers.size()));

	// A layer is pushed even when its texture could not be created, so the stack stays balanced against PopLayer().
	if (layers_size == static_cast<int>(layers.size()))
	{
		RenderTarget target;
		CreateTarget(target, "RmlUi layer");
		layers.push_back(target);
	}
	else if (!layers[static_cast<size_t>(layers_size)].color)
	{
		// An earlier push failed to allocate this target. Try again rather than leaving the layer dead for as long as
		// the window keeps its size.
		CreateTarget(layers[static_cast<size_t>(layers_size)], "RmlUi layer");
	}

	layers_size += 1;
	return static_cast<Rml::LayerHandle>(layers_size - 1);
}

void RenderInterface_SDL_GPU::RenderLayerStack::PopLayer()
{
	RMLUI_ASSERT(layers_size > 0);
	if (layers_size > 0)
		layers_size -= 1;
}

const RenderInterface_SDL_GPU::RenderTarget& RenderInterface_SDL_GPU::RenderLayerStack::GetLayer(Rml::LayerHandle layer) const
{
	RMLUI_ASSERT(static_cast<size_t>(layer) < static_cast<size_t>(layers_size));
	return layers[static_cast<size_t>(layer)];
}

const RenderInterface_SDL_GPU::RenderTarget& RenderInterface_SDL_GPU::RenderLayerStack::GetTopLayer() const
{
	return GetLayer(GetTopLayerHandle());
}

Rml::LayerHandle RenderInterface_SDL_GPU::RenderLayerStack::GetTopLayerHandle() const
{
	RMLUI_ASSERT(layers_size > 0);
	return static_cast<Rml::LayerHandle>(layers_size - 1);
}

const RenderInterface_SDL_GPU::RenderTarget& RenderInterface_SDL_GPU::RenderLayerStack::EnsurePostprocess(int index)
{
	RMLUI_ASSERT(index >= 0 && index < num_postprocess_targets);
	RenderTarget& target = postprocess[index];
	// Created on first use, and retried on every later one: a scene without layer effects never needs these, and a
	// failed allocation should not disable compositing for as long as the window keeps its size.
	if (!target.color)
		CreateTarget(target, "RmlUi postprocess");
	return target;
}

void RenderInterface_SDL_GPU::RenderLayerStack::SwapPostprocessPrimarySecondary()
{
	EnsurePostprocess(0);
	EnsurePostprocess(1);
	std::swap(postprocess[0], postprocess[1]);
}

// -- Setup -------------------------------------------------------------------

SDL_GPUShader* RenderInterface_SDL_GPU::GetShader(ShaderId id)
{
	// Indexed by ShaderId, and must stay in that order. It lives here rather than at file scope so that each row can
	// name the enumerator it belongs to; the two asserts below are what that buys.
	static const ShaderDefinition shader_definitions[] = {
		SHADER_DEF(VertMain, shader_vert, 2, 0, SDL_GPU_SHADERSTAGE_VERTEX),
		SHADER_DEF(VertPassthrough, shader_vert_passthrough, 0, 0, SDL_GPU_SHADERSTAGE_VERTEX),
		SHADER_DEF(VertBlur, shader_vert_blur, 1, 0, SDL_GPU_SHADERSTAGE_VERTEX),
		SHADER_DEF(FragColor, shader_frag_color, 0, 0, SDL_GPU_SHADERSTAGE_FRAGMENT),
		SHADER_DEF(FragTexture, shader_frag_texture, 0, 1, SDL_GPU_SHADERSTAGE_FRAGMENT),
		SHADER_DEF(FragGradient, shader_frag_gradient, 1, 0, SDL_GPU_SHADERSTAGE_FRAGMENT),
		SHADER_DEF(FragCreation, shader_frag_creation, 1, 0, SDL_GPU_SHADERSTAGE_FRAGMENT),
		SHADER_DEF(FragPassthrough, shader_frag_passthrough, 0, 1, SDL_GPU_SHADERSTAGE_FRAGMENT),
		SHADER_DEF(FragColorMatrix, shader_frag_color_matrix, 1, 1, SDL_GPU_SHADERSTAGE_FRAGMENT),
		SHADER_DEF(FragBlendMask, shader_frag_blend_mask, 0, 2, SDL_GPU_SHADERSTAGE_FRAGMENT),
		SHADER_DEF(FragBlur, shader_frag_blur, 1, 1, SDL_GPU_SHADERSTAGE_FRAGMENT),
		SHADER_DEF(FragDropShadow, shader_frag_drop_shadow, 1, 1, SDL_GPU_SHADERSTAGE_FRAGMENT),
	};
	static_assert(sizeof(shader_definitions) / sizeof(shader_definitions[0]) == num_shaders,
		"shader_definitions needs one row per ShaderId, in ShaderId order");

	const int index = static_cast<int>(id);
	RMLUI_ASSERT(shader_definitions[index].id == index);

	if (shaders[index] || shader_failed[index])
		return shaders[index];

	shaders[index] = CreateShaderFromMemory(device, shader_definitions[index]);
	shader_failed[index] = !shaders[index];
	return shaders[index];
}

#undef SHADER_DEF
#undef SHADER_BLOBS
#undef X

void RenderInterface_SDL_GPU::ReleaseShaders()
{
	for (int i = 0; i < num_shaders; i++)
	{
		if (shaders[i])
			SDL_ReleaseGPUShader(device, shaders[i]);
		shaders[i] = nullptr;
		shader_failed[i] = false;
	}
}

RenderInterface_SDL_GPU::ProgramShaders RenderInterface_SDL_GPU::GetProgramShaders(ProgramId program)
{
	// The programs drawing geometry submitted by RmlUi share a vertex stage, which applies the transform. The
	// postprocess programs give their quad in clip space and so need none; blur has one of its own because it works
	// out where its samples fall there rather than once per fragment.
	static constexpr ProgramShaders program_shaders[] = {
		{ShaderId::VertMain, ShaderId::FragColor},
		{ShaderId::VertMain, ShaderId::FragTexture},
		{ShaderId::VertMain, ShaderId::FragGradient},
		{ShaderId::VertMain, ShaderId::FragCreation},
		{ShaderId::VertPassthrough, ShaderId::FragPassthrough},
		{ShaderId::VertPassthrough, ShaderId::FragColorMatrix},
		{ShaderId::VertPassthrough, ShaderId::FragBlendMask},
		{ShaderId::VertBlur, ShaderId::FragBlur},
		{ShaderId::VertPassthrough, ShaderId::FragDropShadow},
	};
	static_assert(sizeof(program_shaders) / sizeof(program_shaders[0]) == static_cast<int>(ProgramId::Count),
		"program_shaders needs one row per ProgramId, in ProgramId order");

	return program_shaders[static_cast<int>(program)];
}

SDL_GPUColorTargetBlendState RenderInterface_SDL_GPU::GetBlendState(Blending blend, bool writes_stencil)
{
	SDL_GPUColorTargetBlendState state{};
	if (writes_stencil)
	{
		// Nothing but the stencil is written, so no blending has anything to act on.
		state.enable_color_write_mask = true;
		state.color_write_mask = 0;
	}
	else if (blend == Blending::Blend)
	{
		state.enable_blend = true;
		state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
		state.color_blend_op = SDL_GPU_BLENDOP_ADD;
		state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
		state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
		state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
		state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	}
	else if (blend == Blending::Constant)
	{
		// The destination is dropped and the source scaled by the blend constant, which is how opacity is applied.
		state.enable_blend = true;
		state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
		state.color_blend_op = SDL_GPU_BLENDOP_ADD;
		state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_CONSTANT_COLOR;
		state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_CONSTANT_COLOR;
		state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
		state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
	}
	// Blending::Replace writes the source as it is, which is what an untouched state does.
	return state;
}

SDL_GPUDepthStencilState RenderInterface_SDL_GPU::GetDepthStencilState(StencilMode stencil, bool writes_stencil)
{
	SDL_GPUDepthStencilState state{};
	if (stencil == StencilMode::Off)
		return state;

	SDL_GPUStencilOpState op{};
	op.fail_op = SDL_GPU_STENCILOP_KEEP;
	op.depth_fail_op = SDL_GPU_STENCILOP_KEEP;
	switch (stencil)
	{
	case StencilMode::TestEqual:
		op.compare_op = SDL_GPU_COMPAREOP_EQUAL;
		op.pass_op = SDL_GPU_STENCILOP_KEEP;
		break;
	case StencilMode::WriteSet:
		op.compare_op = SDL_GPU_COMPAREOP_ALWAYS;
		op.pass_op = SDL_GPU_STENCILOP_REPLACE;
		break;
	case StencilMode::WriteIntersect:
		// Tested rather than written unconditionally: only pixels already holding the mask are raised, so no value can
		// collide with a later generation.
		op.compare_op = SDL_GPU_COMPAREOP_EQUAL;
		op.pass_op = SDL_GPU_STENCILOP_INCREMENT_AND_CLAMP;
		break;
	case StencilMode::Off:
		break;
	}

	state.enable_stencil_test = true;
	state.compare_mask = 0xFF;
	state.write_mask = writes_stencil ? 0xFF : 0x00;
	state.front_stencil_state = op;
	state.back_stencil_state = op;
	return state;
}

SDL_GPUGraphicsPipeline* RenderInterface_SDL_GPU::GetPipeline(ProgramId program, Blending blend, StencilMode stencil)
{
	const bool writes_stencil = (stencil == StencilMode::WriteSet || stencil == StencilMode::WriteIntersect);
	// Colour writes are off in the writing modes, so the blend mode makes no difference there. Folding it away keeps
	// the cache from holding pipelines that only differ in state nothing reads.
	const PipelineKey key{program, writes_stencil ? Blending::Blend : blend, stencil};
	for (const PipelineEntry& entry : pipelines)
	{
		if (entry.key == key)
			return entry.pipeline;
	}

	SDL_GPUColorTargetDescription target{};
	target.format = layer_format;
	target.blend_state = GetBlendState(blend, writes_stencil);

	SDL_GPUVertexAttribute attrib[3]{};
	attrib[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
	attrib[0].location = 0;
	attrib[0].offset = offsetof(Vertex, position);
	attrib[1].format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM;
	attrib[1].location = 1;
	attrib[1].offset = offsetof(Vertex, colour);
	attrib[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
	attrib[2].location = 2;
	attrib[2].offset = offsetof(Vertex, tex_coord);

	SDL_GPUVertexBufferDescription buffer{};
	buffer.pitch = sizeof(Vertex);

	SDL_GPUGraphicsPipelineCreateInfo info{};
	const ProgramShaders program_shaders = GetProgramShaders(program);
	info.vertex_shader = GetShader(program_shaders.vertex);
	info.fragment_shader = GetShader(program_shaders.fragment);
	if (!info.vertex_shader || !info.fragment_shader)
	{
		// A mask this program cannot draw is a mask nothing may be tested against.
		if (writes_stencil)
			stencil_pipelines_failed = true;
		pipelines.push_back({key, nullptr});
		return nullptr;
	}

	info.target_info.num_color_targets = 1;
	info.target_info.color_target_descriptions = &target;

	// The stencil buffer is attached to every pass so that clip masks can use it without reopening passes. Depth
	// testing is never used, and the stencil test stays off until there is a mask to apply.
	const SDL_GPUTextureFormat depth_stencil_format = render_layers.GetDepthStencilFormat();
	if (depth_stencil_format != SDL_GPU_TEXTUREFORMAT_INVALID)
	{
		info.target_info.has_depth_stencil_target = true;
		info.target_info.depth_stencil_format = depth_stencil_format;
	}

	info.depth_stencil_state = GetDepthStencilState(stencil, writes_stencil);

	info.vertex_input_state.num_vertex_attributes = 3;
	info.vertex_input_state.num_vertex_buffers = 1;
	info.vertex_input_state.vertex_attributes = attrib;
	info.vertex_input_state.vertex_buffer_descriptions = &buffer;

	SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(device, &info);
	if (!pipeline)
	{
		Log::Message(Log::LT_ERROR, "Failed to create graphics pipeline: %s", SDL_GetError());
		// A mask that cannot be written must not be tested against either, or every draw would be compared with a
		// value no pixel holds and the whole document would disappear. Take clip masks out of service instead.
		if (stencil != StencilMode::Off)
			stencil_pipelines_failed = true;
	}

	// Cached even on failure. Without that, a pipeline that cannot be built is retried, and logged, once per draw call.
	pipelines.push_back({key, pipeline});
	return pipeline;
}

void RenderInterface_SDL_GPU::ReleasePipelines()
{
	for (const PipelineEntry& entry : pipelines)
	{
		if (entry.pipeline)
			SDL_ReleaseGPUGraphicsPipeline(device, entry.pipeline);
	}
	pipelines.clear();
	bound_pipeline = nullptr;
	// The cache is what remembers the refusal, so dropping it gives the stencil pipelines another go.
	stencil_pipelines_failed = false;
}

// The window is not used: layers have a format of their own, so nothing here depends on what the swapchain looks like.
// The parameter stays for the sake of the backend's call and of the renderers this one is modelled on.
RenderInterface_SDL_GPU::RenderInterface_SDL_GPU(SDL_GPUDevice* device, SDL_Window* /*window*/) : device(device)
{
	render_layers.Initialize(device);

	vertex_buffers.Initialize(device, SDL_GPU_BUFFERUSAGE_VERTEX, "RmlUi vertices");
	index_buffers.Initialize(device, SDL_GPU_BUFFERUSAGE_INDEX, "RmlUi indices");

	SDL_GPUSamplerCreateInfo info{};
	info.min_filter = SDL_GPU_FILTER_LINEAR;
	info.mag_filter = SDL_GPU_FILTER_LINEAR;
	info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
	info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
	info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
	info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
	linear_sampler = SDL_CreateGPUSampler(device, &info);

	// The postprocess passes offset and scale their texture coordinates and do sample outside the image; a repeating
	// sampler would bring colours back from the far edge.
	info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
	info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
	info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
	clamp_sampler = SDL_CreateGPUSampler(device, &info);

	if (!linear_sampler || !clamp_sampler)
		Log::Message(Log::LT_ERROR, "Failed to create sampler: %s", SDL_GetError());
}

RenderInterface_SDL_GPU::~RenderInterface_SDL_GPU()
{
	Shutdown();
}

void RenderInterface_SDL_GPU::Shutdown()
{
	if (shutdown_complete)
		return;

	SubmitUploads();
	EndRenderPass();
	frame_active = false;

	// Releasing resources still in flight is allowed, but waiting keeps shutdown ordering simple to reason about.
	SDL_WaitForGPUIdle(device);

	ReleaseQuads();

	pending_uploads.clear();
	vertex_buffers.ReleaseAll();
	index_buffers.ReleaseAll();
	render_layers.ReleaseAll();

	ReleasePipelines();
	pipelines_depth_stencil_format = SDL_GPU_TEXTUREFORMAT_INVALID;
	ReleaseShaders();

	if (linear_sampler)
		SDL_ReleaseGPUSampler(device, linear_sampler);
	linear_sampler = nullptr;
	if (clamp_sampler)
		SDL_ReleaseGPUSampler(device, clamp_sampler);
	clamp_sampler = nullptr;

	shutdown_complete = true;
}

// -- Frame and pass management -----------------------------------------------

void RenderInterface_SDL_GPU::BeginFrame(SDL_GPUCommandBuffer* in_command_buffer, SDL_GPUTexture* in_swapchain_texture, uint32_t width,
	uint32_t height)
{
	command_buffer = in_command_buffer;
	swapchain_texture = in_swapchain_texture;
	swapchain_width = width;
	swapchain_height = height;
	frame_index += 1;
	frame_active = true;

	projection = Matrix4f::ProjectOrtho(0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, -10'000.f, 10'000.f);
	transform = projection;

	scissor_enabled = false;
	scissor_region = Rectanglei::FromSize({static_cast<int>(width), static_cast<int>(height)});

	// RmlUi resets its render state between frames, so no mask carries over. The stencil buffer is cleared with the
	// base layer below, which resets the generation counters with it.
	clip_mask_enabled = false;

	InvalidateRenderPassState();

#if RMLUI_BACKEND_SDL_GPU_DEBUG
	SDL_PushGPUDebugGroup(command_buffer, "RmlUi frame");
#endif

	render_layers.BeginFrame(static_cast<int>(width), static_cast<int>(height));

	// A pipeline records whether its pass carries a depth/stencil attachment, so the cache is only valid while that
	// answer holds. It can change on the frame the targets are (re)created.
	const SDL_GPUTextureFormat depth_stencil_format = render_layers.GetDepthStencilFormat();
	if (depth_stencil_format != pipelines_depth_stencil_format)
	{
		ReleasePipelines();
		pipelines_depth_stencil_format = depth_stencil_format;
	}

	EnsureQuads(static_cast<int>(width), static_cast<int>(height));

	// Opening the pass here rather than at the first draw gets the base layer cleared even for an empty frame, and
	// clears the stencil buffer for the clip mask.
	EnsureRenderPass(render_layers.GetTopLayer(), true, true);
}

void RenderInterface_SDL_GPU::EndFrame()
{
	// The backend skips BeginFrame() for a frame it cannot present, most often because the window is minimized, but
	// still calls this. The layer stack must not be popped then: it was never pushed, and taking it below the base
	// layer leaves the stack corrupt for every frame that follows. Uploads are still drained, since a skipped frame is
	// the one case where nothing else will.
	if (!frame_active)
	{
		SubmitUploads();
		return;
	}
	frame_active = false;

	EndRenderPass();

	// The base layer holds the frame; hand it to the swapchain. The blit converts to whatever format the window uses,
	// which is why layers can keep a fixed one.
	const RenderTarget* base_layer = render_layers.GetBaseLayer();
	if (command_buffer && swapchain_texture && base_layer && base_layer->color)
	{
		SDL_GPUBlitInfo blit{};
		blit.source.texture = base_layer->color;
		blit.source.w = static_cast<Uint32>(base_layer->width);
		blit.source.h = static_cast<Uint32>(base_layer->height);
		blit.destination.texture = swapchain_texture;
		blit.destination.w = swapchain_width;
		blit.destination.h = swapchain_height;
		blit.load_op = SDL_GPU_LOADOP_DONT_CARE;
		blit.filter = SDL_GPU_FILTER_NEAREST;
		SDL_BlitGPUTexture(command_buffer, &blit);
	}

	render_layers.EndFrame();

#if RMLUI_BACKEND_SDL_GPU_DEBUG
	if (command_buffer)
		SDL_PopGPUDebugGroup(command_buffer);
#endif

	// The caller submits the frame's command buffer right after this returns, so this is the last moment at which the
	// transfers this frame depends on can still be placed ahead of it.
	SubmitUploads();

	// Free buffers accumulate at the peak usage of the application. Release the ones that have gone unused, but only
	// every so often since the sweep walks the whole pool.
	if (frame_index % buffer_retention_frames == 0)
	{
		vertex_buffers.Trim(frame_index);
		index_buffers.Trim(frame_index);
	}

	command_buffer = nullptr;
	swapchain_texture = nullptr;
}

void RenderInterface_SDL_GPU::InvalidateRenderPassState()
{
	bound_pipeline = nullptr;
	bound_texture = nullptr;
	bound_mask_texture = nullptr;
	bound_sampler = nullptr;
	bound_vertex_buffer = nullptr;
	bound_index_buffer = nullptr;
	transform_dirty = true;
	translation_dirty = true;
	scissor_dirty = true;
	stencil_reference_dirty = true;
	blend_constant_dirty = true;
	// A new pass starts with the scissor covering the whole target, so the cached value no longer reflects reality.
	applied_scissor = {-1, -1, -1, -1};
}

bool RenderInterface_SDL_GPU::EnsureRenderPass(const RenderTarget& target, bool clear_color, bool clear_stencil)
{
	if (render_pass && active_target_texture == target.color && !clear_color && !clear_stencil)
		return true;

	// Ended before the bail-out too, so that a caller which ignores the result can never go on to draw into whatever
	// target the previous pass happened to hold.
	EndRenderPass();

	if (!command_buffer || !target.color)
		return false;

	SDL_GPUColorTargetInfo color_info{};
	color_info.texture = target.color;
	color_info.load_op = clear_color ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD;
	color_info.store_op = SDL_GPU_STOREOP_STORE;
	color_info.cycle = clear_color;

	SDL_GPUDepthStencilTargetInfo depth_stencil_info{};
	SDL_GPUDepthStencilTargetInfo* depth_stencil_ptr = nullptr;
	if (SDL_GPUTexture* depth_stencil = render_layers.GetDepthStencil())
	{
		depth_stencil_info.texture = depth_stencil;
		depth_stencil_info.load_op = SDL_GPU_LOADOP_DONT_CARE;
		depth_stencil_info.store_op = SDL_GPU_STOREOP_DONT_CARE;
		// The clip mask is a property of the frame, not of a layer: RmlUi sets a mask and then pushes layers under
		// it, expecting it to still apply, and only re-issues the mask when it changes. So the one stencil buffer is
		// shared by every target and carried across passes, and it is cleared only when asked for explicitly.
		depth_stencil_info.stencil_load_op = clear_stencil ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD;
		depth_stencil_info.stencil_store_op = SDL_GPU_STOREOP_STORE;
		depth_stencil_info.clear_stencil = 0;
		depth_stencil_ptr = &depth_stencil_info;
	}

	render_pass = SDL_BeginGPURenderPass(command_buffer, &color_info, 1, depth_stencil_ptr);
	if (!render_pass)
	{
		Log::Message(Log::LT_ERROR, "Failed to begin render pass: %s", SDL_GetError());
		active_target_texture = nullptr;
		return false;
	}

	if (clear_stencil)
	{
		stencil_high_water = 0;
		stencil_test_value = 0;
	}

	active_target_texture = target.color;
	InvalidateRenderPassState();
	return true;
}

void RenderInterface_SDL_GPU::EndRenderPass()
{
	if (render_pass)
	{
		SDL_EndGPURenderPass(render_pass);
		render_pass = nullptr;
	}
	active_target_texture = nullptr;
}

bool RenderInterface_SDL_GPU::EnsureUploadPass()
{
	if (upload_copy_pass)
		return true;

	if (!upload_command_buffer)
	{
		upload_command_buffer = SDL_AcquireGPUCommandBuffer(device);
		if (!upload_command_buffer)
		{
			Log::Message(Log::LT_ERROR, "Failed to acquire command buffer: %s", SDL_GetError());
			return false;
		}
	}

	upload_copy_pass = SDL_BeginGPUCopyPass(upload_command_buffer);
	if (!upload_copy_pass)
	{
		Log::Message(Log::LT_ERROR, "Failed to begin copy pass: %s", SDL_GetError());
		SDL_CancelGPUCommandBuffer(upload_command_buffer);
		upload_command_buffer = nullptr;
		return false;
	}

	return true;
}

void RenderInterface_SDL_GPU::SubmitUploads()
{
	if (upload_copy_pass)
	{
		SDL_EndGPUCopyPass(upload_copy_pass);
		upload_copy_pass = nullptr;
	}
	if (upload_command_buffer)
	{
		SDL_SubmitGPUCommandBuffer(upload_command_buffer);
		upload_command_buffer = nullptr;
	}

	pending_upload_bytes = 0;
}

bool RenderInterface_SDL_GPU::FlushGeometryUploads()
{
	if (pending_uploads.empty())
		return true;
	if (!EnsureUploadPass())
		return false;

	for (const PendingUpload& upload : pending_uploads)
	{
		upload.buffer->pending_upload = -1;
		if (upload.size == 0)
			continue;

		SDL_GPUTransferBufferLocation location{};
		location.transfer_buffer = upload.buffer->transfer_buffer;

		SDL_GPUBufferRegion region{};
		region.buffer = upload.buffer->buffer;
		region.size = upload.size;

		// Cycling lets the driver hand us fresh storage when the buffer is still being read by an earlier frame,
		// which is what makes it safe to reuse a buffer immediately after releasing its geometry.
		SDL_UploadToGPUBuffer(upload_copy_pass, &location, &region, true);

		// A binding records the storage the buffer had at the time, not the buffer itself, so cycling leaves the one
		// already recorded in the open pass pointing at the previous contents. The cache is keyed on the buffer, which
		// has not changed, so without this the next draw would skip the rebind and read the old mesh.
		if (upload.buffer->buffer == bound_vertex_buffer)
			bound_vertex_buffer = nullptr;
		if (upload.buffer->buffer == bound_index_buffer)
			bound_index_buffer = nullptr;
	}

	pending_uploads.clear();
	return true;
}

// -- Geometry ----------------------------------------------------------------

CompiledGeometryHandle RenderInterface_SDL_GPU::CompileGeometry(Span<const Vertex> vertices, Span<const int> indices)
{
	const uint32_t vertex_size = static_cast<uint32_t>(vertices.size() * sizeof(Vertex));
	const uint32_t index_size = static_cast<uint32_t>(indices.size() * sizeof(int));
	if (vertex_size == 0 || index_size == 0)
		return 0;

	Buffer* vertex_buffer = vertex_buffers.Acquire(vertex_size, frame_index);
	Buffer* index_buffer = index_buffers.Acquire(index_size, frame_index);
	if (!vertex_buffer || !index_buffer)
	{
		if (vertex_buffer)
			vertex_buffers.Release(vertex_buffer);
		if (index_buffer)
			index_buffers.Release(index_buffer);
		return 0;
	}

	void* vertex_data = SDL_MapGPUTransferBuffer(device, vertex_buffer->transfer_buffer, true);
	void* index_data = SDL_MapGPUTransferBuffer(device, index_buffer->transfer_buffer, true);
	if (!vertex_data || !index_data)
	{
		Log::Message(Log::LT_ERROR, "Failed to map transfer buffer(s): %s", SDL_GetError());
		if (vertex_data)
			SDL_UnmapGPUTransferBuffer(device, vertex_buffer->transfer_buffer);
		if (index_data)
			SDL_UnmapGPUTransferBuffer(device, index_buffer->transfer_buffer);
		// Mapping cycles the transfer buffer, so whatever a queued upload was going to read is gone even on the side
		// that succeeded. Drop those entries instead of letting them copy undefined bytes.
		CancelUpload(vertex_buffer);
		CancelUpload(index_buffer);
		vertex_buffers.Release(vertex_buffer);
		index_buffers.Release(index_buffer);
		return 0;
	}

	std::memcpy(vertex_data, vertices.data(), vertex_size);
	std::memcpy(index_data, indices.data(), index_size);
	SDL_UnmapGPUTransferBuffer(device, vertex_buffer->transfer_buffer);
	SDL_UnmapGPUTransferBuffer(device, index_buffer->transfer_buffer);

	// Uploads are queued rather than recorded here. RmlUi compiles each mesh immediately before drawing it, so
	// recording on the spot would mean a copy pass per mesh; queueing lets the flush before the next draw collect all
	// of them into one.
	QueueUpload(vertex_buffer, vertex_size);
	QueueUpload(index_buffer, index_size);

	GeometryView* geometry = new GeometryView();
	geometry->vertex_buffer = vertex_buffer;
	geometry->index_buffer = index_buffer;
	geometry->num_indices = static_cast<int>(indices.size());

	return reinterpret_cast<CompiledGeometryHandle>(geometry);
}

void RenderInterface_SDL_GPU::QueueUpload(Buffer* buffer, uint32_t size)
{
	// A buffer can be released and handed out again before its upload has been flushed. In that case the transfer
	// buffer already holds the new contents, so the queued entry is updated rather than duplicated.
	if (buffer->pending_upload >= 0)
	{
		pending_uploads[static_cast<size_t>(buffer->pending_upload)].size = size;
		return;
	}

	buffer->pending_upload = static_cast<int>(pending_uploads.size());
	pending_uploads.push_back({buffer, size});
}

void RenderInterface_SDL_GPU::CancelUpload(Buffer* buffer)
{
	// The entry is kept but emptied: the indices of every other buffer point into this same list, so it must not be
	// erased from. FlushGeometryUploads() skips zero-sized entries.
	if (buffer->pending_upload >= 0)
		pending_uploads[static_cast<size_t>(buffer->pending_upload)].size = 0;
}

void RenderInterface_SDL_GPU::ReleaseGeometry(CompiledGeometryHandle handle)
{
	GeometryView* geometry = reinterpret_cast<GeometryView*>(handle);
	if (!geometry)
		return;

	// Releasing is immediate: the buffers keep their contents until they are handed out again, and cycling on the
	// next upload protects any commands still in flight.
	vertex_buffers.Release(geometry->vertex_buffer);
	index_buffers.Release(geometry->index_buffer);
	geometry->vertex_buffer->last_used_frame = frame_index;
	geometry->index_buffer->last_used_frame = frame_index;

	delete geometry;
}

bool RenderInterface_SDL_GPU::DrawGeometry(const GeometryView& geometry, const DrawState& state)
{
	SDL_GPUGraphicsPipeline* pipeline = GetPipeline(state.program, state.blend, state.stencil);
	if (!pipeline)
		return false;

	if (pipeline != bound_pipeline)
	{
		SDL_BindGPUGraphicsPipeline(render_pass, pipeline);
		bound_pipeline = pipeline;
		bound_texture = nullptr;
		bound_mask_texture = nullptr;
		bound_sampler = nullptr;
	}

	SDL_GPUSampler* sampler = state.sampler ? state.sampler : linear_sampler;
	if (state.texture && (state.texture != bound_texture || state.mask_texture != bound_mask_texture || sampler != bound_sampler))
	{
		SDL_GPUTextureSamplerBinding texture_bindings[2]{};
		texture_bindings[0].texture = state.texture;
		texture_bindings[0].sampler = sampler;
		Uint32 num_bindings = 1;
		if (state.mask_texture)
		{
			texture_bindings[1].texture = state.mask_texture;
			texture_bindings[1].sampler = sampler;
			num_bindings = 2;
		}
		SDL_BindGPUFragmentSamplers(render_pass, 0, texture_bindings, num_bindings);
		bound_texture = state.texture;
		bound_mask_texture = state.mask_texture;
		bound_sampler = sampler;
	}

	if (geometry.vertex_buffer->buffer != bound_vertex_buffer)
	{
		SDL_GPUBufferBinding binding{};
		binding.buffer = geometry.vertex_buffer->buffer;
		SDL_BindGPUVertexBuffers(render_pass, 0, &binding, 1);
		bound_vertex_buffer = binding.buffer;
	}

	if (geometry.index_buffer->buffer != bound_index_buffer)
	{
		SDL_GPUBufferBinding binding{};
		binding.buffer = geometry.index_buffer->buffer;
		SDL_BindGPUIndexBuffer(render_pass, &binding, SDL_GPU_INDEXELEMENTSIZE_32BIT);
		bound_index_buffer = binding.buffer;
	}

	if (scissor_dirty)
		ApplyScissor();

	if (state.stencil != StencilMode::Off && (stencil_reference_dirty || state.stencil_reference != applied_stencil_reference))
	{
		SDL_SetGPUStencilReference(render_pass, state.stencil_reference);
		applied_stencil_reference = state.stencil_reference;
		stencil_reference_dirty = false;
	}

	if (state.blend == Blending::Constant && (blend_constant_dirty || state.blend_constant != applied_blend_constant))
	{
		const SDL_FColor constants{state.blend_constant, state.blend_constant, state.blend_constant, state.blend_constant};
		SDL_SetGPUBlendConstants(render_pass, constants);
		applied_blend_constant = state.blend_constant;
		blend_constant_dirty = false;
	}

	// Only the programs whose vertex stage declares them; pushing to a slot a shader does not have would be data no
	// draw can read. The postprocess programs give their quad in clip space and so take neither, except for blur,
	// which takes the spacing of its samples in the slot the transform would have used.
	if (GetProgramShaders(state.program).vertex == ShaderId::VertMain)
	{
		if (state.transform)
		{
			SDL_PushGPUVertexUniformData(command_buffer, 0, state.transform, sizeof(Matrix4f));
			transform_dirty = true;
		}
		else if (transform_dirty)
		{
			SDL_PushGPUVertexUniformData(command_buffer, 0, &transform, sizeof(transform));
			transform_dirty = false;
		}

		if (translation_dirty || state.translation != pushed_translation)
		{
			SDL_PushGPUVertexUniformData(command_buffer, 1, &state.translation, sizeof(state.translation));
			pushed_translation = state.translation;
			translation_dirty = false;
		}
	}
	else if (state.vertex_uniforms)
	{
		SDL_PushGPUVertexUniformData(command_buffer, 0, state.vertex_uniforms, state.vertex_uniforms_size);
		transform_dirty = true;
	}

	if (state.fragment_uniforms)
		SDL_PushGPUFragmentUniformData(command_buffer, 0, state.fragment_uniforms, state.fragment_uniforms_size);

	SDL_DrawGPUIndexedPrimitives(render_pass, geometry.num_indices, 1, 0, 0, 0);
	return true;
}

bool RenderInterface_SDL_GPU::HasStencil() const
{
	return render_layers.GetDepthStencilFormat() != SDL_GPU_TEXTUREFORMAT_INVALID && !stencil_pipelines_failed;
}

RenderInterface_SDL_GPU::StencilMode RenderInterface_SDL_GPU::GetClipMaskMode() const
{
	return (clip_mask_enabled && HasStencil()) ? StencilMode::TestEqual : StencilMode::Off;
}

void RenderInterface_SDL_GPU::RenderGeometry(CompiledGeometryHandle handle, Vector2f translation, TextureHandle texture)
{
	GeometryView* geometry = reinterpret_cast<GeometryView*>(handle);
	if (!geometry || !command_buffer)
		return;

	// Record any transfer this draw depends on. It goes to the upload command buffer, which is submitted ahead of the
	// frame's, so the render pass opened below stays open until the frame ends.
	if (!FlushGeometryUploads())
		return;
	if (!EnsureRenderPass(render_layers.GetTopLayer()))
		return;

	DrawState state;
	state.program = (texture != 0) ? ProgramId::Texture : ProgramId::Color;
	state.stencil = GetClipMaskMode();
	state.stencil_reference = stencil_test_value;
	state.texture = reinterpret_cast<SDL_GPUTexture*>(texture);
	state.translation = translation;
	DrawGeometry(*geometry, state);
}

// -- Textures ----------------------------------------------------------------

TextureHandle RenderInterface_SDL_GPU::LoadTexture(Vector2i& texture_dimensions, const String& source)
{
	FileInterface* file_interface = GetFileInterface();
	FileHandle file_handle = file_interface->Open(source);
	if (!file_handle)
		return 0;

	file_interface->Seek(file_handle, 0, SEEK_END);
	const size_t buffer_size = file_interface->Tell(file_handle);
	file_interface->Seek(file_handle, 0, SEEK_SET);

	UniquePtr<byte[]> buffer(new byte[buffer_size]);
	file_interface->Read(buffer.get(), buffer_size, file_handle);
	file_interface->Close(file_handle);

	const size_t i_ext = source.rfind('.');
	const String extension = (i_ext == String::npos ? String() : source.substr(i_ext + 1));

	SDL_Surface* surface = IMG_LoadTyped_IO(SDL_IOFromMem(buffer.get(), int(buffer_size)), 1, extension.c_str());
	if (!surface)
		return 0;

	if (surface->format != SDL_PIXELFORMAT_RGBA32)
	{
		SDL_Surface* converted_surface = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
		SDL_DestroySurface(surface);
		if (!converted_surface)
			return 0;

		surface = converted_surface;
	}

	// Convert colors to premultiplied alpha, which is necessary for correct alpha compositing.
	const size_t pixels_byte_size = static_cast<size_t>(surface->w) * static_cast<size_t>(surface->h) * 4;
	byte* pixels = static_cast<byte*>(surface->pixels);
	for (size_t i = 0; i < pixels_byte_size; i += 4)
	{
		const byte alpha = pixels[i + 3];
		for (size_t j = 0; j < 3; ++j)
			pixels[i + j] = byte(int(pixels[i + j]) * int(alpha) / 255);
	}

	texture_dimensions = {surface->w, surface->h};
	const TextureHandle handle = GenerateTexture({pixels, pixels_byte_size}, texture_dimensions);

	SDL_DestroySurface(surface);

	return handle;
}

SDL_GPUTexture* RenderInterface_SDL_GPU::CreateTexture(int width, int height, SDL_GPUTextureFormat format, SDL_GPUTextureUsageFlags usage,
	const char* debug_name)
{
	SDL_GPUTextureCreateInfo info{};
	info.type = SDL_GPU_TEXTURETYPE_2D;
	info.usage = usage;
	info.format = format;
	info.width = static_cast<Uint32>(width);
	info.height = static_cast<Uint32>(height);
	info.layer_count_or_depth = 1;
	info.num_levels = 1;

	SDL_GPUTexture* texture = SDL_CreateGPUTexture(device, &info);
	if (!texture)
	{
		Log::Message(Log::LT_ERROR, "Failed to create texture: %s", SDL_GetError());
		return nullptr;
	}

	SDL_SetGPUTextureName(device, texture, debug_name);
	return texture;
}

TextureHandle RenderInterface_SDL_GPU::GenerateTexture(Span<const byte> source, Vector2i source_dimensions)
{
	if (source_dimensions.x <= 0 || source_dimensions.y <= 0)
		return 0;

	const uint32_t byte_size = static_cast<uint32_t>(source_dimensions.x) * static_cast<uint32_t>(source_dimensions.y) * 4;
	if (source.size() < static_cast<size_t>(byte_size))
	{
		Log::Message(Log::LT_ERROR, "Texture source of %zu bytes is too small for %dx%d pixels", source.size(), source_dimensions.x,
			source_dimensions.y);
		return 0;
	}

	SDL_GPUTransferBuffer* transfer_buffer = nullptr;
	{
		SDL_GPUTransferBufferCreateInfo info{};
		info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
		info.size = byte_size;
		transfer_buffer = SDL_CreateGPUTransferBuffer(device, &info);
		if (!transfer_buffer)
		{
			Log::Message(Log::LT_ERROR, "Failed to create transfer buffer: %s", SDL_GetError());
			return 0;
		}
	}

	void* dst = SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
	if (!dst)
	{
		Log::Message(Log::LT_ERROR, "Failed to map transfer buffer: %s", SDL_GetError());
		SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
		return 0;
	}

	std::memcpy(dst, source.data(), byte_size);
	SDL_UnmapGPUTransferBuffer(device, transfer_buffer);

	SDL_GPUTexture* texture =
		CreateTexture(source_dimensions.x, source_dimensions.y, content_format, SDL_GPU_TEXTUREUSAGE_SAMPLER, "RmlUi texture");
	if (!texture)
	{
		SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
		return 0;
	}

	if (!EnsureUploadPass())
	{
		SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
		SDL_ReleaseGPUTexture(device, texture);
		return 0;
	}

	SDL_GPUTextureTransferInfo transfer_info{};
	transfer_info.transfer_buffer = transfer_buffer;

	SDL_GPUTextureRegion region{};
	region.texture = texture;
	region.w = source_dimensions.x;
	region.h = source_dimensions.y;
	region.d = 1;

	SDL_UploadToGPUTexture(upload_copy_pass, &transfer_info, &region, false);

	// Releasing here is a request, not a destruction: the recorded copy holds a reference, and SDL frees the transfer
	// buffer once the command buffer it belongs to has completed.
	SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);

	// Loading a document generates many textures without ever ending a frame, so cap how much a single command buffer
	// is allowed to accumulate.
	pending_upload_bytes += byte_size;
	if (pending_upload_bytes >= max_pending_upload_bytes)
		SubmitUploads();

	return reinterpret_cast<TextureHandle>(texture);
}

void RenderInterface_SDL_GPU::ReleaseTexture(TextureHandle texture_handle)
{
	// SDL defers the actual destruction until the GPU is done with the texture, so this is safe mid-frame.
	SDL_ReleaseGPUTexture(device, reinterpret_cast<SDL_GPUTexture*>(texture_handle));
	if (reinterpret_cast<SDL_GPUTexture*>(texture_handle) == bound_texture)
		bound_texture = nullptr;
}

// -- State -------------------------------------------------------------------

void RenderInterface_SDL_GPU::EnableScissorRegion(bool enable)
{
	if (scissor_enabled == enable)
		return;
	scissor_enabled = enable;
	scissor_dirty = true;
}

void RenderInterface_SDL_GPU::SetScissorRegion(Rectanglei region)
{
	if (scissor_region == region)
		return;
	scissor_region = region;
	if (scissor_enabled)
		scissor_dirty = true;
}

Rectanglei RenderInterface_SDL_GPU::GetScissorRegion() const
{
	if (scissor_override_active)
		return scissor_override;

	const Rectanglei target = Rectanglei::FromSize({render_layers.GetWidth(), render_layers.GetHeight()});
	if (!scissor_enabled)
		return target;
	if (!scissor_region.Valid())
		return Rectanglei::FromSize({0, 0});
	return scissor_region;
}

void RenderInterface_SDL_GPU::SetScissorOverride(Rectanglei region)
{
	scissor_override_active = true;
	scissor_override = region;
	scissor_dirty = true;
}

void RenderInterface_SDL_GPU::ClearScissorOverride()
{
	scissor_override_active = false;
	scissor_dirty = true;
}

Rectanglei RenderInterface_SDL_GPU::GetActiveScissor() const
{
	const Rectanglei target = Rectanglei::FromSize({render_layers.GetWidth(), render_layers.GetHeight()});
	const Rectanglei region = GetScissorRegion().IntersectIfValid(target);

	const int left = Math::Clamp(region.Left(), 0, target.Right());
	const int top = Math::Clamp(region.Top(), 0, target.Bottom());
	const int right = Math::Max(Math::Min(region.Right(), target.Right()), left);
	const int bottom = Math::Max(Math::Min(region.Bottom(), target.Bottom()), top);

	return Rectanglei::FromCorners({left, top}, {right, bottom});
}

void RenderInterface_SDL_GPU::ApplyScissor()
{
	const Rectanglei region = GetActiveScissor();

	SDL_Rect rect;
	rect.x = region.Left();
	rect.y = region.Top();
	rect.w = region.Width();
	rect.h = region.Height();

	if (rect.x != applied_scissor.x || rect.y != applied_scissor.y || rect.w != applied_scissor.w || rect.h != applied_scissor.h)
	{
		applied_scissor = rect;
		SDL_SetGPUScissor(render_pass, &applied_scissor);
	}

	scissor_dirty = false;
}

void RenderInterface_SDL_GPU::SetTransform(const Matrix4f* new_transform)
{
	transform = new_transform ? (projection * (*new_transform)) : projection;
	transform_dirty = true;
}

// -- Clip mask ---------------------------------------------------------------

void RenderInterface_SDL_GPU::EnableClipMask(bool enable)
{
	clip_mask_enabled = enable;
}

/*
    SDL GPU can only clear the stencil buffer as a pass begins, so instead of clearing per mask, every Set takes a
    value never written since the last real clear; older masks are left where they are, holding smaller values, and
    the test is for equality.

    stencil_high_water keeps the invariant that a Set hands out a value strictly greater than anything in the buffer.
    Counting generations would not do: Intersect raises the mask above the value its Set handed out, so a mask
    narrowed a few times would reach what the next Set picks. max_clip_mask_depth is the reserve kept for nesting.

    A generation is committed only once the draw meant to write it has been issued: one that reached no pixel would
    match nothing, and every draw under it would be culled.
*/
void RenderInterface_SDL_GPU::RenderToClipMask(Rml::ClipMaskOperation operation, CompiledGeometryHandle handle, Vector2f translation)
{
	GeometryView* geometry = reinterpret_cast<GeometryView*>(handle);
	if (!geometry || !command_buffer || !HasStencil())
		return;

	if (!FlushGeometryUploads())
		return;

	// Eight bits hold only so many generations. When they run out the buffer has to be cleared for real, which means
	// restarting the pass; the colour is loaded back, so only the mask is lost. Worth doing solely where the mask is
	// about to be replaced wholesale, since Intersect builds on what is already there.
	const bool replaces_mask = (operation != Rml::ClipMaskOperation::Intersect);
	const bool clear_stencil = replaces_mask && (stencil_high_water >= max_stencil_generation);
	if (!EnsureRenderPass(render_layers.GetTopLayer(), false, clear_stencil))
		return;

	// What a replacing operation takes: the value just above everything the buffer holds. The clear above guarantees
	// there is one left, so this cannot wrap.
	const uint8_t generation = static_cast<uint8_t>(stencil_high_water + 1);

	DrawState state;
	state.stencil = StencilMode::WriteSet;
	state.translation = translation;

	switch (operation)
	{
	case Rml::ClipMaskOperation::Set:
	{
		state.stencil_reference = generation;
	}
	break;
	case Rml::ClipMaskOperation::SetInverse:
	{
		// The mask is everything the geometry does not cover: raise the whole region to the new generation, then
		// punch the geometry back down with zero, which no generation ever equals. The fill is the only thing that
		// puts the generation in the buffer, so without it leave the mask already in force.
		DrawState fill;
		fill.stencil = StencilMode::WriteSet;
		fill.stencil_reference = generation;
		fill.transform = &projection;
		if (!clear_quad || !DrawGeometry(*reinterpret_cast<GeometryView*>(clear_quad), fill))
			return;

		// The buffer holds the new value from here on, whether or not the punch below lands, so the bound has to
		// move with it — otherwise a later Set could hand the same value out a second time.
		stencil_high_water = generation;
		state.stencil_reference = 0;
	}
	break;
	case Rml::ClipMaskOperation::Intersect:
	{
		// Raising the covered area by one keeps only what the previous mask and this geometry have in common at the
		// new value. The pipeline tests for the current generation, so pixels the old mask did not hold are left
		// exactly where they were.
		state.stencil = StencilMode::WriteIntersect;
		state.stencil_reference = stencil_test_value;
	}
	break;
	}

	if (!DrawGeometry(*geometry, state))
		return;

	if (operation == Rml::ClipMaskOperation::Intersect)
	{
		if (stencil_test_value < 0xFF)
		{
			stencil_test_value += 1;
			stencil_high_water = stencil_test_value;
		}
		else if (!stencil_reserve_exhausted)
		{
			// INCREMENT_AND_CLAMP has nowhere left to go, so this Intersect and every one after it leaves the mask
			// as it was: content they should have clipped away stays visible. Only reachable by nesting masks
			// deeper than the reserve, and otherwise entirely silent, so say it once.
			stencil_reserve_exhausted = true;
			Log::Message(Log::LT_WARNING, "Clip masks nested deeper than %d levels, the mask no longer narrows",
				max_clip_mask_depth);
		}
	}
	else
	{
		stencil_test_value = generation;
		stencil_high_water = generation;
	}
}

// -- Shaders -----------------------------------------------------------------

CompiledShaderHandle RenderInterface_SDL_GPU::CompileShader(const String& name, const Dictionary& parameters)
{
	auto ApplyColorStopList = [](CompiledShader& shader, const Dictionary& shader_parameters) {
		auto it = shader_parameters.find("color_stop_list");
		RMLUI_ASSERT(it != shader_parameters.end() && it->second.GetType() == Variant::COLORSTOPLIST);
		const ColorStopList& color_stop_list = it->second.GetReference<ColorStopList>();
		const int num_stops = Math::Min((int)color_stop_list.size(), max_num_stops);

		shader.stop_positions.resize(num_stops);
		shader.stop_colors.resize(num_stops);
		for (int i = 0; i < num_stops; i++)
		{
			const ColorStop& stop = color_stop_list[i];
			RMLUI_ASSERT(stop.position.unit == Unit::NUMBER);
			shader.stop_positions[i] = stop.position.number;
			shader.stop_colors[i] = ConvertToColorf(stop.color);
		}
	};

	CompiledShader shader = {};

	if (name == "linear-gradient")
	{
		shader.type = CompiledShaderType::Gradient;
		const bool repeating = Rml::Get(parameters, "repeating", false);
		shader.gradient_function = (repeating ? ShaderGradientFunction::RepeatingLinear : ShaderGradientFunction::Linear);
		shader.p = Rml::Get(parameters, "p0", Vector2f(0.f));
		shader.v = Rml::Get(parameters, "p1", Vector2f(0.f)) - shader.p;
		ApplyColorStopList(shader, parameters);
	}
	else if (name == "radial-gradient")
	{
		shader.type = CompiledShaderType::Gradient;
		const bool repeating = Rml::Get(parameters, "repeating", false);
		shader.gradient_function = (repeating ? ShaderGradientFunction::RepeatingRadial : ShaderGradientFunction::Radial);
		shader.p = Rml::Get(parameters, "center", Vector2f(0.f));
		shader.v = Vector2f(1.f) / Rml::Get(parameters, "radius", Vector2f(1.f));
		ApplyColorStopList(shader, parameters);
	}
	else if (name == "conic-gradient")
	{
		shader.type = CompiledShaderType::Gradient;
		const bool repeating = Rml::Get(parameters, "repeating", false);
		shader.gradient_function = (repeating ? ShaderGradientFunction::RepeatingConic : ShaderGradientFunction::Conic);
		shader.p = Rml::Get(parameters, "center", Vector2f(0.f));
		const float angle = Rml::Get(parameters, "angle", 0.f);
		shader.v = {Math::Cos(angle), Math::Sin(angle)};
		ApplyColorStopList(shader, parameters);
	}
	else if (name == "shader")
	{
		const String value = Rml::Get(parameters, "value", String());
		if (value == "creation")
		{
			shader.type = CompiledShaderType::Creation;
			shader.dimensions = Rml::Get(parameters, "dimensions", Vector2f(0.f));
		}
	}

	if (shader.type != CompiledShaderType::Invalid)
		return reinterpret_cast<CompiledShaderHandle>(new CompiledShader(std::move(shader)));

	Log::Message(Log::LT_WARNING, "Unsupported shader type '%s'.", name.c_str());
	return {};
}

void RenderInterface_SDL_GPU::RenderShader(CompiledShaderHandle shader_handle, CompiledGeometryHandle geometry_handle, Vector2f translation,
	TextureHandle /*texture*/)
{
	const CompiledShader* shader = reinterpret_cast<CompiledShader*>(shader_handle);
	GeometryView* geometry = reinterpret_cast<GeometryView*>(geometry_handle);
	if (!shader || !geometry || !command_buffer)
		return;

	if (!FlushGeometryUploads())
		return;
	if (!EnsureRenderPass(render_layers.GetTopLayer()))
		return;

	DrawState state;
	state.stencil = GetClipMaskMode();
	state.stencil_reference = stencil_test_value;
	state.translation = translation;

	switch (shader->type)
	{
	case CompiledShaderType::Gradient:
	{
		RMLUI_ASSERT(shader->stop_positions.size() == shader->stop_colors.size());
		const int num_stops = Math::Min((int)shader->stop_positions.size(), max_num_stops);

		// Built on the stack every draw. The buffer is a few hundred bytes and SDL copies it out of here as the draw
		// is recorded, so there is nothing to be gained by keeping it around between calls.
		GradientUniforms uniforms = {};
		uniforms.p = shader->p;
		uniforms.v = shader->v;
		uniforms.func = static_cast<int>(shader->gradient_function);
		uniforms.num_stops = num_stops;
		for (int i = 0; i < num_stops; i++)
		{
			uniforms.stop_colors[i] = shader->stop_colors[i];
			uniforms.stop_positions[i] = shader->stop_positions[i];
		}

		state.program = ProgramId::Gradient;
		state.fragment_uniforms = &uniforms;
		state.fragment_uniforms_size = sizeof(uniforms);
		DrawGeometry(*geometry, state);
	}
	break;
	case CompiledShaderType::Creation:
	{
		CreationUniforms uniforms = {};
		uniforms.dimensions = shader->dimensions;
		uniforms.value = static_cast<float>(GetSystemInterface()->GetElapsedTime());

		state.program = ProgramId::Creation;
		state.fragment_uniforms = &uniforms;
		state.fragment_uniforms_size = sizeof(uniforms);
		DrawGeometry(*geometry, state);
	}
	break;
	case CompiledShaderType::Invalid:
	{
		Log::Message(Log::LT_WARNING, "Unhandled render shader %d.", static_cast<int>(shader->type));
	}
	break;
	}
}

void RenderInterface_SDL_GPU::ReleaseShader(CompiledShaderHandle shader_handle)
{
	delete reinterpret_cast<CompiledShader*>(shader_handle);
}

// -- Filters -----------------------------------------------------------------

// Past a certain radius the image is scaled down before the fixed seven-sample kernel is applied. Works out how many
// times to halve it, and what standard deviation to blur the result with.
static void SigmaToParameters(const float desired_sigma, int& out_pass_level, float& out_sigma)
{
	constexpr int max_num_passes = 10;
	static_assert(max_num_passes < 31, "");
	constexpr float max_single_pass_sigma = 3.0f;
	out_pass_level = Math::Clamp(Math::Log2(int(desired_sigma * (2.f / max_single_pass_sigma))), 0, max_num_passes);
	out_sigma = Math::Clamp(desired_sigma / float(1 << out_pass_level), 0.0f, max_single_pass_sigma);
}

// The region a postprocess pass may read from, in texture coordinates. Samples outside it are dropped by the shader,
// which is what stops a blur from dragging in whatever the postprocess target held from an earlier composite.
static void SetTexCoordLimits(Vector2f& out_tex_coord_min, Vector2f& out_tex_coord_max, Rectanglei rectangle, Vector2i target_size)
{
	// Offset by half-texel values so that texture lookups are clamped to fragment centers, thereby avoiding color
	// bleeding from neighboring texels due to bilinear interpolation.
	out_tex_coord_min = (Vector2f(rectangle.p0) + Vector2f(0.5f)) / Vector2f(target_size);
	out_tex_coord_max = (Vector2f(rectangle.p1) - Vector2f(0.5f)) / Vector2f(target_size);
}

// The normalized half of a Gaussian kernel: the centre weight followed by one weight per step away from it, since the
// two sides are the same.
static void SetBlurWeights(float* out_weights, int num_weights, float sigma)
{
	float normalization = 0.0f;
	for (int i = 0; i < num_weights; i++)
	{
		if (Math::Absolute(sigma) < 0.1f)
			out_weights[i] = float(i == 0);
		else
			out_weights[i] = Math::Exp(-float(i * i) / (2.0f * sigma * sigma)) / (Math::SquareRoot(2.f * Math::RMLUI_PI) * sigma);

		normalization += (i == 0 ? 1.f : 2.0f) * out_weights[i];
	}
	for (int i = 0; i < num_weights; i++)
		out_weights[i] /= normalization;
}

CompiledFilterHandle RenderInterface_SDL_GPU::CompileFilter(const String& name, const Dictionary& parameters)
{
	CompiledFilter filter = {};

	if (name == "opacity")
	{
		filter.type = FilterType::Passthrough;
		filter.blend_factor = Rml::Get(parameters, "value", 1.0f);
	}
	else if (name == "blur")
	{
		filter.type = FilterType::Blur;
		filter.sigma = Rml::Get(parameters, "sigma", 1.0f);
	}
	else if (name == "drop-shadow")
	{
		filter.type = FilterType::DropShadow;
		filter.sigma = Rml::Get(parameters, "sigma", 0.f);
		filter.color = Rml::Get(parameters, "color", Colourb()).ToPremultiplied();
		filter.offset = Rml::Get(parameters, "offset", Vector2f(0.f));
	}
	else if (name == "brightness")
	{
		filter.type = FilterType::ColorMatrix;
		const float value = Rml::Get(parameters, "value", 1.0f);
		filter.color_matrix = Matrix4f::Diag(value, value, value, 1.f);
	}
	else if (name == "contrast")
	{
		filter.type = FilterType::ColorMatrix;
		const float value = Rml::Get(parameters, "value", 1.0f);
		const float grayness = 0.5f - 0.5f * value;
		filter.color_matrix = Matrix4f::Diag(value, value, value, 1.f);
		filter.color_matrix.SetColumn(3, Vector4f(grayness, grayness, grayness, 1.f));
	}
	else if (name == "invert")
	{
		filter.type = FilterType::ColorMatrix;
		const float value = Math::Clamp(Rml::Get(parameters, "value", 1.0f), 0.f, 1.f);
		const float inverted = 1.f - 2.f * value;
		filter.color_matrix = Matrix4f::Diag(inverted, inverted, inverted, 1.f);
		filter.color_matrix.SetColumn(3, Vector4f(value, value, value, 1.f));
	}
	else if (name == "grayscale")
	{
		filter.type = FilterType::ColorMatrix;
		const float value = Rml::Get(parameters, "value", 1.0f);
		const float rev_value = 1.f - value;
		const Vector3f gray = value * Vector3f(0.2126f, 0.7152f, 0.0722f);
		// clang-format off
		filter.color_matrix = Matrix4f::FromRows(
			{gray.x + rev_value, gray.y,             gray.z,             0.f},
			{gray.x,             gray.y + rev_value, gray.z,             0.f},
			{gray.x,             gray.y,             gray.z + rev_value, 0.f},
			{0.f,                0.f,                0.f,                1.f}
		);
		// clang-format on
	}
	else if (name == "sepia")
	{
		filter.type = FilterType::ColorMatrix;
		const float value = Rml::Get(parameters, "value", 1.0f);
		const float rev_value = 1.f - value;
		const Vector3f r_mix = value * Vector3f(0.393f, 0.769f, 0.189f);
		const Vector3f g_mix = value * Vector3f(0.349f, 0.686f, 0.168f);
		const Vector3f b_mix = value * Vector3f(0.272f, 0.534f, 0.131f);
		// clang-format off
		filter.color_matrix = Matrix4f::FromRows(
			{r_mix.x + rev_value, r_mix.y,             r_mix.z,             0.f},
			{g_mix.x,             g_mix.y + rev_value, g_mix.z,             0.f},
			{b_mix.x,             b_mix.y,             b_mix.z + rev_value, 0.f},
			{0.f,                 0.f,                 0.f,                 1.f}
		);
		// clang-format on
	}
	else if (name == "hue-rotate")
	{
		// Hue-rotation and saturation values based on: https://www.w3.org/TR/filter-effects-1/#attr-valuedef-type-huerotate
		filter.type = FilterType::ColorMatrix;
		const float value = Rml::Get(parameters, "value", 1.0f);
		const float s = Math::Sin(value);
		const float c = Math::Cos(value);
		// clang-format off
		filter.color_matrix = Matrix4f::FromRows(
			{0.213f + 0.787f * c - 0.213f * s,  0.715f - 0.715f * c - 0.715f * s,  0.072f - 0.072f * c + 0.928f * s,  0.f},
			{0.213f - 0.213f * c + 0.143f * s,  0.715f + 0.285f * c + 0.140f * s,  0.072f - 0.072f * c - 0.283f * s,  0.f},
			{0.213f - 0.213f * c - 0.787f * s,  0.715f - 0.715f * c + 0.715f * s,  0.072f + 0.928f * c + 0.072f * s,  0.f},
			{0.f,                               0.f,                               0.f,                               1.f}
		);
		// clang-format on
	}
	else if (name == "saturate")
	{
		filter.type = FilterType::ColorMatrix;
		const float value = Rml::Get(parameters, "value", 1.0f);
		// clang-format off
		filter.color_matrix = Matrix4f::FromRows(
			{0.213f + 0.787f * value,  0.715f - 0.715f * value,  0.072f - 0.072f * value,  0.f},
			{0.213f - 0.213f * value,  0.715f + 0.285f * value,  0.072f - 0.072f * value,  0.f},
			{0.213f - 0.213f * value,  0.715f - 0.715f * value,  0.072f + 0.928f * value,  0.f},
			{0.f,                      0.f,                      0.f,                      1.f}
		);
		// clang-format on
	}

	if (filter.type != FilterType::Invalid)
		return reinterpret_cast<CompiledFilterHandle>(new CompiledFilter(std::move(filter)));

	Log::Message(Log::LT_WARNING, "Unsupported filter type '%s'.", name.c_str());
	return {};
}

void RenderInterface_SDL_GPU::ReleaseFilter(CompiledFilterHandle filter)
{
	delete reinterpret_cast<CompiledFilter*>(filter);
}

// Large radii shrink the image rather than widen the kernel, which is fixed at seven samples. Everything after the
// downscale happens in the top-left corner of the targets, on a region halved along with the image; the last step
// scales it back over the region it came from.
void RenderInterface_SDL_GPU::RenderBlur(float sigma, const RenderTarget& source_destination, const RenderTarget& temp, Rectanglei window)
{
	RMLUI_ASSERT(&source_destination != &temp);
	if (!source_destination.color || !temp.color)
		return;
	if (window.Width() <= 0 || window.Height() <= 0)
		return;

	int pass_level = 0;
	SigmaToParameters(sigma, pass_level, sigma);

	const bool had_scissor_override = scissor_override_active;
	const Rectanglei previous_scissor_override = scissor_override;

	Rectanglei scissor = window;

	// Downscale by iterative half-scaling with bilinear filtering, to reduce aliasing. The quad is drawn over the
	// corner the smaller image occupies rather than shrinking the viewport: one less piece of pass state to restore.
	const Rectanglei half_target = Rectanglei::FromSize({source_destination.width / 2, source_destination.height / 2});

	// Scale UVs if we have even dimensions, such that texture fetches align perfectly between texels, thereby
	// producing a 50% blend of neighboring texels.
	const Vector2f uv_scaling = {(source_destination.width % 2 == 1) ? (1.f - 1.f / float(source_destination.width)) : 1.f,
		(source_destination.height % 2 == 1) ? (1.f - 1.f / float(source_destination.height)) : 1.f};

	for (int i = 0; i < pass_level; i++)
	{
		scissor.p0 = (scissor.p0 + Vector2i(1)) / 2;
		scissor.p1 = Math::Max(scissor.p1 / 2, scissor.p0);
		const bool from_source = (i % 2 == 0);
		SetScissorOverride(scissor);

		DrawState state;
		state.program = ProgramId::Passthrough;
		state.blend = Blending::Replace;
		state.sampler = clamp_sampler;
		state.texture = (from_source ? source_destination : temp).color;
		DrawPostprocessQuad(from_source ? temp : source_destination, state, half_target, Vector2f(0.f), uv_scaling);
	}

	SetScissorOverride(scissor);

	// Ensure texture data end up in the temp buffer. Depending on the last downscaling, we might need to move it from
	// the source_destination buffer.
	if (pass_level % 2 == 0)
		DrawTextureToTarget(temp, source_destination.color, Blending::Replace);

	BlurUniforms uniforms = {};
	SetBlurWeights(uniforms.weights, blur_num_weights, sigma);
	SetTexCoordLimits(uniforms.tex_coord_min, uniforms.tex_coord_max, scissor, {source_destination.width, source_destination.height});

	BlurVertexUniforms vertex_uniforms = {};

	DrawState state;
	state.program = ProgramId::Blur;
	state.blend = Blending::Replace;
	state.sampler = clamp_sampler;
	state.fragment_uniforms = &uniforms;
	state.fragment_uniforms_size = sizeof(uniforms);
	state.vertex_uniforms = &vertex_uniforms;
	state.vertex_uniforms_size = sizeof(vertex_uniforms);

	// Blur render pass - vertical.
	vertex_uniforms.texel_offset = Vector2f(0.f, 1.f) / float(temp.height);
	state.texture = temp.color;
	DrawPostprocessQuad(source_destination, state);

	// Add a 1px transparent border around the blur region by first clearing with a padded scissor. This helps prevent
	// artifacts when upscaling the blur result in the later step: sampling along the edge of the region otherwise
	// reaches a texel beyond it, which holds whatever the target had from an earlier composite.
	SetScissorOverride(scissor.Extend(1));
	ClearRegion(temp);
	SetScissorOverride(scissor);

	// Blur render pass - horizontal.
	vertex_uniforms.texel_offset = Vector2f(1.f, 0.f) / float(source_destination.width);
	state.texture = source_destination.color;
	DrawPostprocessQuad(temp, state);

	// Scale the blurred image back over the region it came from -- unless the halving left nothing to scale. A window
	// narrower than twice the downscale factor collapses to no area, and a quad with no extent in its texture
	// coordinates would still cover the whole window and paint it with the border clear's single transparent texel,
	// erasing what was to be blurred. Leave the unblurred image standing instead.
	if (scissor.Width() > 0 && scissor.Height() > 0)
	{
		SetScissorOverride(window);
		BlitRegion(source_destination, temp, scissor, window);

		// The upscale above might be jittery at low resolutions (large pass levels). This is especially noticeable
		// when moving an element with backdrop blur around, or when trying to click or hover an element within a
		// blurred region since it may be rendered at an offset. For more stable and accurate rendering we next
		// upscale the blur image by an exact power-of-two. However, this may not fill the edges completely, so we
		// need to do the above first. Note that this strategy may sometimes result in visible seams.
		const Vector2i target_min = scissor.p0 * (1 << pass_level);
		const Vector2i target_max = scissor.p1 * (1 << pass_level);
		if (target_min != window.p0 || target_max != window.p1)
			BlitRegion(source_destination, temp, scissor, Rectanglei::FromCorners(target_min, target_max));
	}

	if (had_scissor_override)
		SetScissorOverride(previous_scissor_override);
	else
		ClearScissorOverride();
}

/*
    Every filter reads the primary target and writes the secondary, then the two are swapped, so the output is always
    the primary target.

    These passes deliberately go untested against the clip mask -- hence StencilMode::Off on every draw below. What
    the mask cuts away is cut away by the final draw into the destination either way, and testing here would leave a
    blur reading an image with the masked-out part already missing, then smearing it back inwards along the edge.
*/
void RenderInterface_SDL_GPU::RenderFilters(Span<const CompiledFilterHandle> filter_handles)
{
	for (const CompiledFilterHandle filter_handle : filter_handles)
	{
		const CompiledFilter& filter = *reinterpret_cast<const CompiledFilter*>(filter_handle);
		// References into the postprocess targets have to be taken afresh for each filter: the swap at the end of the
		// previous one exchanged what they name.
		const RenderTarget& primary = render_layers.GetPostprocessPrimary();
		const RenderTarget& secondary = render_layers.GetPostprocessSecondary();
		if (!primary.color || !secondary.color)
			return;

		switch (filter.type)
		{
		case FilterType::Passthrough:
		{
			// The source scaled by a constant and the destination dropped, which is opacity applied to the layer as a
			// whole rather than to each element in it.
			DrawState state;
			state.program = ProgramId::Passthrough;
			state.blend = Blending::Constant;
			state.blend_constant = filter.blend_factor;
			state.sampler = clamp_sampler;
			state.texture = primary.color;
			DrawPostprocessQuad(secondary, state);

			render_layers.SwapPostprocessPrimarySecondary();
		}
		break;
		case FilterType::Blur:
		{
			RenderBlur(filter.sigma, primary, secondary, GetActiveScissor());
		}
		break;
		case FilterType::DropShadow:
		{
			const Rectanglei window = GetActiveScissor();

			DropShadowUniforms uniforms = {};
			SetTexCoordLimits(uniforms.tex_coord_min, uniforms.tex_coord_max, window, {primary.width, primary.height});
			uniforms.color = ConvertToColorf(filter.color);

			// The shadow is the image moved by the offset and painted in a single colour. Moving the image one way
			// means sampling it the other, hence the negation; both axes are negated because texture coordinates run
			// the same way as the offset does.
			const Vector2f uv_offset = -filter.offset / Vector2f(static_cast<float>(primary.width), static_cast<float>(primary.height));

			DrawState state;
			state.program = ProgramId::DropShadow;
			state.blend = Blending::Replace;
			state.sampler = clamp_sampler;
			state.texture = primary.color;
			state.fragment_uniforms = &uniforms;
			state.fragment_uniforms_size = sizeof(uniforms);
			DrawPostprocessQuad(secondary, state, Rectanglei::FromSize({secondary.width, secondary.height}), uv_offset, Vector2f(1.f));

			if (filter.sigma >= 0.5f)
			{
				const RenderTarget& tertiary = render_layers.GetPostprocessTertiary();
				RenderBlur(filter.sigma, secondary, tertiary, window);
			}

			// The element itself over the shadow it casts.
			DrawTextureToTarget(secondary, primary.color, Blending::Blend);

			render_layers.SwapPostprocessPrimarySecondary();
		}
		break;
		case FilterType::ColorMatrix:
		{
			ColorMatrixUniforms uniforms = {};
			uniforms.color_matrix = filter.color_matrix;

			DrawState state;
			state.program = ProgramId::ColorMatrix;
			state.blend = Blending::Replace;
			state.sampler = clamp_sampler;
			state.texture = primary.color;
			state.fragment_uniforms = &uniforms;
			state.fragment_uniforms_size = sizeof(uniforms);
			DrawPostprocessQuad(secondary, state);

			render_layers.SwapPostprocessPrimarySecondary();
		}
		break;
		case FilterType::MaskImage:
		{
			const RenderTarget& blend_mask = render_layers.GetBlendMask();
			if (!blend_mask.color)
				break;

			DrawState state;
			state.program = ProgramId::BlendMask;
			state.blend = Blending::Replace;
			state.sampler = clamp_sampler;
			state.texture = primary.color;
			state.mask_texture = blend_mask.color;
			DrawPostprocessQuad(secondary, state);

			render_layers.SwapPostprocessPrimarySecondary();
		}
		break;
		case FilterType::Invalid:
		{
			Log::Message(Log::LT_WARNING, "Unhandled render filter %d.", static_cast<int>(filter.type));
		}
		break;
		}
	}
}

// -- Layers ------------------------------------------------------------------

void RenderInterface_SDL_GPU::ReleaseQuads()
{
	if (fullscreen_quad)
		ReleaseGeometry(fullscreen_quad);
	if (clear_quad)
		ReleaseGeometry(clear_quad);

	fullscreen_quad = {};
	clear_quad = {};
	quad_width = 0;
	quad_height = 0;
}

bool RenderInterface_SDL_GPU::EnsureQuads(int width, int height)
{
	if (fullscreen_quad && clear_quad && quad_width == width && quad_height == height)
		return true;

	ReleaseQuads();

	const int indices[6] = {0, 1, 2, 0, 2, 3};
	const float w = static_cast<float>(width);
	const float h = static_cast<float>(height);

	// Given in clip space and so the same whatever the target size, unlike the clear quad below. Texture coordinate
	// (0,0) belongs at the top-left corner, which is where clip space has y at +1. The colour goes unread -- the
	// postprocess programs take none -- but the vertex format has one.
	const ColourbPremultiplied white(255, 255, 255, 255);
	const Vertex fullscreen_vertices[4] = {
		{{-1.f, 1.f}, white, {0.f, 0.f}},
		{{1.f, 1.f}, white, {1.f, 0.f}},
		{{1.f, -1.f}, white, {1.f, 1.f}},
		{{-1.f, -1.f}, white, {0.f, 1.f}},
	};
	fullscreen_quad = CompileGeometry({fullscreen_vertices, 4}, {indices, 6});

	// Drawn with blending off, this writes transparent black over whatever it covers.
	const ColourbPremultiplied transparent(0, 0, 0, 0);
	const Vertex clear_vertices[4] = {
		{{0.f, 0.f}, transparent, {0.f, 0.f}},
		{{w, 0.f}, transparent, {1.f, 0.f}},
		{{w, h}, transparent, {1.f, 1.f}},
		{{0.f, h}, transparent, {0.f, 1.f}},
	};
	clear_quad = CompileGeometry({clear_vertices, 4}, {indices, 6});

	if (!fullscreen_quad || !clear_quad)
	{
		// Without these, PushLayer() falls back to clearing the whole attachment and CompositeLayers() cannot run at
		// all, so say so rather than letting layers silently stop working.
		Log::Message(Log::LT_ERROR, "Failed to build the compositing quads, layer effects will be degraded");
		return false;
	}

	quad_width = width;
	quad_height = height;
	return true;
}

void RenderInterface_SDL_GPU::ClearScissorRegion()
{
	if (!clear_quad || !render_pass)
		return;
	if (!FlushGeometryUploads())
		return;

	// Left unmasked deliberately: this stands in for a clear of the layer's colour, and a clear is not something the
	// clip mask has any say over.
	DrawState state;
	state.blend = Blending::Replace;
	state.transform = &projection;
	DrawGeometry(*reinterpret_cast<GeometryView*>(clear_quad), state);
}

void RenderInterface_SDL_GPU::ClearRegion(const RenderTarget& target)
{
	if (!EnsureRenderPass(target))
		return;
	ClearScissorRegion();
}

bool RenderInterface_SDL_GPU::DrawPostprocessQuad(const RenderTarget& destination, const DrawState& state, Rectanglei region,
	Vector2f uv_offset, Vector2f uv_scaling)
{
	if (!command_buffer || !destination.color || destination.width <= 0 || destination.height <= 0)
		return false;

	const Rectanglei full_target = Rectanglei::FromSize({destination.width, destination.height});
	const bool covers_target = (region == full_target && uv_offset == Vector2f(0.f) && uv_scaling == Vector2f(1.f));

	// Built for this draw alone whenever the quad is not simply the whole target: a downscaling pass covers a corner
	// of it, and an upscaling one maps a rectangle onto a rectangle. The buffers come from the same pool as any other
	// mesh, so a released one is normally handed straight back on the next call.
	CompiledGeometryHandle temporary_quad = {};
	if (!covers_target)
	{
		const float width = static_cast<float>(destination.width);
		const float height = static_cast<float>(destination.height);
		// Clip space runs from -1 to 1 across the target, with y pointing the other way to the pixel rows.
		const float x0 = 2.f * static_cast<float>(region.Left()) / width - 1.f;
		const float x1 = 2.f * static_cast<float>(region.Right()) / width - 1.f;
		const float y0 = 1.f - 2.f * static_cast<float>(region.Top()) / height;
		const float y1 = 1.f - 2.f * static_cast<float>(region.Bottom()) / height;
		const Vector2f uv0 = uv_offset;
		const Vector2f uv1 = uv_offset + uv_scaling;

		const ColourbPremultiplied white(255, 255, 255, 255);
		const Vertex vertices[4] = {
			{{x0, y0}, white, {uv0.x, uv0.y}},
			{{x1, y0}, white, {uv1.x, uv0.y}},
			{{x1, y1}, white, {uv1.x, uv1.y}},
			{{x0, y1}, white, {uv0.x, uv1.y}},
		};
		const int indices[6] = {0, 1, 2, 0, 2, 3};
		temporary_quad = CompileGeometry({vertices, 4}, {indices, 6});
		if (!temporary_quad)
			return false;
	}
	else if (!fullscreen_quad)
	{
		return false;
	}

	const CompiledGeometryHandle quad = covers_target ? fullscreen_quad : temporary_quad;

	bool drawn = false;
	if (FlushGeometryUploads() && EnsureRenderPass(destination))
		drawn = DrawGeometry(*reinterpret_cast<GeometryView*>(quad), state);

	if (temporary_quad)
		ReleaseGeometry(temporary_quad);

	return drawn;
}

bool RenderInterface_SDL_GPU::DrawPostprocessQuad(const RenderTarget& destination, const DrawState& state)
{
	return DrawPostprocessQuad(destination, state, Rectanglei::FromSize({destination.width, destination.height}), Vector2f(0.f),
		Vector2f(1.f));
}

bool RenderInterface_SDL_GPU::DrawTextureToTarget(const RenderTarget& destination, SDL_GPUTexture* source, Blending blend, StencilMode stencil)
{
	if (!source)
		return false;

	DrawState state;
	state.program = ProgramId::Passthrough;
	state.blend = blend;
	state.stencil = stencil;
	state.stencil_reference = stencil_test_value;
	state.texture = source;
	state.sampler = clamp_sampler;
	return DrawPostprocessQuad(destination, state);
}

void RenderInterface_SDL_GPU::BlitRegion(const RenderTarget& destination, const RenderTarget& source, Rectanglei source_region,
	Rectanglei destination_region)
{
	if (!source.color || source.width <= 0 || source.height <= 0)
		return;

	const Vector2f source_size = {static_cast<float>(source.width), static_cast<float>(source.height)};
	const Vector2f uv_offset = Vector2f(source_region.p0) / source_size;
	const Vector2f uv_scaling = Vector2f(source_region.Size()) / source_size;

	DrawState state;
	state.program = ProgramId::Passthrough;
	state.blend = Blending::Replace;
	state.texture = source.color;
	state.sampler = clamp_sampler;
	DrawPostprocessQuad(destination, state, destination_region, uv_offset, uv_scaling);
}

LayerHandle RenderInterface_SDL_GPU::PushLayer()
{
	const LayerHandle handle = render_layers.PushLayer();
	const RenderTarget& target = render_layers.GetTopLayer();

	if (clear_quad)
	{
		// The contract only asks for the layer to be transparent black within the active scissor, and RmlUi sets a
		// tight scissor around the element before pushing. Clearing the whole attachment instead would cost a
		// full-screen write per layer, which is ruinous on documents with many effects.
		//
		// Only when the pass really did move to the new layer: the clear is a draw, and a draw with the pass still on
		// the layer below would punch a transparent hole in content that is already there.
		if (EnsureRenderPass(target))
			ClearScissorRegion();
	}
	else
	{
		// No quad to draw the cleared region with. Clearing the whole attachment is still correct, only slower. The
		// stencil is left alone: the clip mask in force when the layer was pushed still applies inside it.
		EnsureRenderPass(target, true);
	}

	return handle;
}

void RenderInterface_SDL_GPU::PopLayer()
{
	render_layers.PopLayer();
	// The next draw re-targets the new top layer through EnsureRenderPass().
}

void RenderInterface_SDL_GPU::CompositeLayers(LayerHandle source, LayerHandle destination, Rml::BlendMode blend_mode,
	Span<const CompiledFilterHandle> filters)
{
	// Compositing goes via the postprocess targets when the source cannot simply be sampled into the destination. The
	// contract allows both handles to name the same layer, and a texture cannot be a colour attachment and a sampler
	// binding at once; filters read and write those targets in turn and so need the detour as well. Everything else
	// takes the direct path: the detour costs a second full-region draw, which measures at about a quarter of the
	// frame on a filter-heavy document.
	// A layer whose texture could not be created has nothing to composite.
	if (!render_layers.GetLayer(source).color)
		return;

	const bool via_postprocess = (source == destination) || !filters.empty();

	// The mask applies to the draw that reaches the destination, and only to it. The hop into the postprocess target
	// is a move of the source's pixels, not a rendering of them, and masking it would cut away pixels the filters
	// still have to see.
	const StencilMode stencil = GetClipMaskMode();
	const Blending blending = (blend_mode == Rml::BlendMode::Replace) ? Blending::Replace : Blending::Blend;

	if (via_postprocess)
	{
		// The second hop below samples whatever this one leaves in the postprocess target. If it could not be issued
		// -- no target to allocate, no buffer to build the quad from -- that target still holds the previous
		// composite, and going on would blend a stale image into the destination. Better to leave the destination as
		// it is: the frame is already degraded, and an error has been logged where the failure happened.
		if (!DrawTextureToTarget(render_layers.GetPostprocessPrimary(), render_layers.GetLayer(source).color, Blending::Replace))
			return;

		RenderFilters(filters);

		// Taken only now: each filter swaps the primary and secondary targets, so a reference from before the chain
		// ran would name the scratch target rather than the result.
		const RenderTarget& result = render_layers.GetPostprocessPrimary();
		DrawTextureToTarget(render_layers.GetLayer(destination), result.color, blending, stencil);
	}
	else
	{
		DrawTextureToTarget(render_layers.GetLayer(destination), render_layers.GetLayer(source).color, blending, stencil);
	}

	// No pass is opened for the new top layer here. Every path that draws opens the one it needs, and the caller pops
	// the layer immediately afterwards, so doing it now only buys a render pass that is closed without a single draw.
}

TextureHandle RenderInterface_SDL_GPU::SaveLayerAsTexture()
{
	if (!command_buffer)
		return 0;

	// Sized from the scissor as submitted, not from the clamped one: RmlUi records the texture's dimensions from the
	// same unclamped rectangle, and a texture smaller than that would be stretched when drawn. Only the copied
	// rectangle is clamped to what the layer actually holds.
	const Rectanglei region = GetScissorRegion();
	const Rectanglei copy_region = GetActiveScissor();
	if (region.Width() <= 0 || region.Height() <= 0 || copy_region.Width() <= 0 || copy_region.Height() <= 0)
		return 0;

	const RenderTarget& source = render_layers.GetTopLayer();
	if (!source.color)
		return 0;

	// The copy fills only the part of the region the layer actually holds. When the two differ, the rest of the
	// texture has to be cleared, or it would hand RmlUi whatever the allocation happened to contain.
	const bool partial = (copy_region.Width() != region.Width() || copy_region.Height() != region.Height());

	SDL_GPUTextureUsageFlags usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
	if (partial)
		usage |= SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;

	SDL_GPUTexture* texture = CreateTexture(region.Width(), region.Height(), layer_format, usage, "RmlUi layer texture");
	if (!texture)
		return 0;

	// The copy has to see what this frame has drawn so far, so it belongs to the frame's command buffer rather than
	// the upload one. That means closing the render pass and reopening it afterwards.
	EndRenderPass();

	if (partial)
	{
		SDL_GPUColorTargetInfo clear_info{};
		clear_info.texture = texture;
		clear_info.load_op = SDL_GPU_LOADOP_CLEAR;
		clear_info.store_op = SDL_GPU_STOREOP_STORE;
		if (SDL_GPURenderPass* clear_pass = SDL_BeginGPURenderPass(command_buffer, &clear_info, 1, nullptr))
			SDL_EndGPURenderPass(clear_pass);
	}

	SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(command_buffer);
	if (!copy_pass)
	{
		Log::Message(Log::LT_ERROR, "Failed to begin copy pass: %s", SDL_GetError());
		SDL_ReleaseGPUTexture(device, texture);
		return 0;
	}

	SDL_GPUTextureLocation source_location{};
	source_location.texture = source.color;
	source_location.x = static_cast<Uint32>(copy_region.Left());
	source_location.y = static_cast<Uint32>(copy_region.Top());

	SDL_GPUTextureLocation destination_location{};
	destination_location.texture = texture;
	destination_location.x = static_cast<Uint32>(copy_region.Left() - region.Left());
	destination_location.y = static_cast<Uint32>(copy_region.Top() - region.Top());

	SDL_CopyGPUTextureToTexture(copy_pass, &source_location, &destination_location, static_cast<Uint32>(copy_region.Width()),
		static_cast<Uint32>(copy_region.Height()), 1, false);
	SDL_EndGPUCopyPass(copy_pass);

	EnsureRenderPass(source);

	return reinterpret_cast<TextureHandle>(texture);
}

// Keeps the top layer as the image a MaskImage filter multiplies by. It goes to a target of its own so the filter
// chain's two stay free; only one saved mask is ever in flight, so one target is enough and the filter carries
// nothing but its type.
CompiledFilterHandle RenderInterface_SDL_GPU::SaveLayerAsMaskImage()
{
	if (!command_buffer)
		return {};

	const RenderTarget& source = render_layers.GetTopLayer();
	const RenderTarget& primary = render_layers.GetPostprocessPrimary();
	const RenderTarget& blend_mask = render_layers.GetBlendMask();
	if (!source.color || !primary.color || !blend_mask.color)
		return {};

	// By way of the postprocess target rather than straight across, which is the route the other backends take too:
	// this first hop is where a multisampled layer will be resolved. As in CompositeLayers(), the second hop copies
	// whatever the first one left, so a failure there must not be carried forward -- handing back a mask filter that
	// multiplies by the previous mask is worse than handing back none, which leaves the element simply unmasked.
	if (!DrawTextureToTarget(primary, source.color, Blending::Replace))
		return {};
	if (!DrawTextureToTarget(blend_mask, primary.color, Blending::Replace))
		return {};

	CompiledFilter filter = {};
	filter.type = FilterType::MaskImage;
	return reinterpret_cast<CompiledFilterHandle>(new CompiledFilter(std::move(filter)));
}

// -- Screen capture ----------------------------------------------------------

namespace {
/*
    What the capture takes from the device, given back by the scope rather than by hand. Every step of a capture can
    fail, and each failure has to return what the steps before it took -- written out, that is the same two calls
    repeated down the function, where leaving one out costs a leak that nothing reports.
*/
struct ScopedTransferBuffer {
	SDL_GPUDevice* device = nullptr;
	SDL_GPUTransferBuffer* buffer = nullptr;

	~ScopedTransferBuffer()
	{
		if (buffer)
			SDL_ReleaseGPUTransferBuffer(device, buffer);
	}
};

// Cancelled unless it was submitted, which the holder is told by having its buffer taken away.
struct ScopedCommandBuffer {
	SDL_GPUCommandBuffer* buffer = nullptr;

	~ScopedCommandBuffer()
	{
		if (buffer)
			SDL_CancelGPUCommandBuffer(buffer);
	}
};
} // namespace

bool RenderInterface_SDL_GPU::CaptureScreen(int& out_width, int& out_height, int& out_num_components, UniquePtr<byte[]>& out_data)
{
	// Reached through GetBaseLayer() rather than GetLayer(0): this runs after EndFrame(), which has taken the base
	// layer off the stack, and the layer stack is empty altogether until the first frame has been rendered.
	const RenderTarget* base_layer = render_layers.GetBaseLayer();
	if (!base_layer || !base_layer->color || base_layer->width <= 0 || base_layer->height <= 0)
		return false;

	const int width = base_layer->width;
	const int height = base_layer->height;
	const uint32_t byte_size = static_cast<uint32_t>(width) * static_cast<uint32_t>(height) * 4;

	SDL_GPUTransferBufferCreateInfo transfer_info{};
	transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD;
	transfer_info.size = byte_size;
	const ScopedTransferBuffer transfer{device, SDL_CreateGPUTransferBuffer(device, &transfer_info)};
	if (!transfer.buffer)
	{
		Log::Message(Log::LT_ERROR, "Failed to create transfer buffer: %s", SDL_GetError());
		return false;
	}

	ScopedCommandBuffer capture{SDL_AcquireGPUCommandBuffer(device)};
	if (!capture.buffer)
	{
		Log::Message(Log::LT_ERROR, "Failed to acquire command buffer: %s", SDL_GetError());
		return false;
	}

	SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(capture.buffer);
	if (!copy_pass)
	{
		Log::Message(Log::LT_ERROR, "Failed to begin copy pass: %s", SDL_GetError());
		return false;
	}

	SDL_GPUTextureRegion region{};
	region.texture = base_layer->color;
	region.w = static_cast<Uint32>(width);
	region.h = static_cast<Uint32>(height);
	region.d = 1;

	SDL_GPUTextureTransferInfo destination{};
	destination.transfer_buffer = transfer.buffer;

	SDL_DownloadFromGPUTexture(copy_pass, &region, &destination);
	SDL_EndGPUCopyPass(copy_pass);

	SDL_GPUFence* fence = SDL_SubmitGPUCommandBufferAndAcquireFence(capture.buffer);
	// Submitted, so the buffer belongs to the device from here whether or not a fence came back.
	capture.buffer = nullptr;
	if (!fence)
	{
		Log::Message(Log::LT_ERROR, "Failed to submit capture command buffer: %s", SDL_GetError());
		return false;
	}
	// The one call that establishes the download has actually run, so a failure here means the bytes below are
	// undefined. Reporting success on those would show up in the visual test suite as a failing test rather than as
	// a broken capture.
	const bool waited = SDL_WaitForGPUFences(device, true, &fence, 1);
	SDL_ReleaseGPUFence(device, fence);
	if (!waited)
	{
		Log::Message(Log::LT_ERROR, "Failed to wait for the capture to complete: %s", SDL_GetError());
		return false;
	}

	const byte* mapped = static_cast<const byte*>(SDL_MapGPUTransferBuffer(device, transfer.buffer, false));
	if (!mapped)
	{
		Log::Message(Log::LT_ERROR, "Failed to map transfer buffer: %s", SDL_GetError());
		return false;
	}

	out_width = width;
	out_height = height;
	out_num_components = 3;
	out_data = UniquePtr<byte[]>(new byte[static_cast<size_t>(width) * static_cast<size_t>(height) * 3]);

	for (int y = 0; y < height; y++)
	{
		const byte* source_row = mapped + static_cast<size_t>(height - 1 - y) * static_cast<size_t>(width) * 4;
		byte* destination_row = out_data.get() + static_cast<size_t>(y) * static_cast<size_t>(width) * 3;
		for (int x = 0; x < width; x++)
		{
			destination_row[x * 3 + 0] = source_row[x * 4 + 0];
			destination_row[x * 3 + 1] = source_row[x * 4 + 1];
			destination_row[x * 3 + 2] = source_row[x * 4 + 2];
		}
	}

	SDL_UnmapGPUTransferBuffer(device, transfer.buffer);
	return true;
}
