#include "RmlUi_Renderer_SDL_GPU.h"
#include "RmlUi_SDL_GPU/ShadersCompiledSPV.h"
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Core/Log.h>
#include <RmlUi/Core/Math.h>
#include <RmlUi/Core/Types.h>
#include <SDL3_image/SDL_image.h>
#include <algorithm>
#include <cstring>

// Debug groups are only emitted for a device created in debug mode, and cost a string push per frame otherwise.
#ifndef RMLUI_BACKEND_SDL_GPU_DEBUG
	#define RMLUI_BACKEND_SDL_GPU_DEBUG false
#endif

using namespace Rml;

enum ShaderType {
	ShaderTypeColor,
	ShaderTypeTexture,
	ShaderTypeVert,
	ShaderTypeCount,
};

enum ShaderFormat {
	ShaderFormatSPIRV,
	ShaderFormatMSL,
	ShaderFormatDXIL,
	ShaderFormatCount,
};

struct Shader {
	Span<const byte> data[ShaderFormatCount];
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

static const Shader shaders[ShaderTypeCount] = {
	{{X(shader_frag_color_spirv), X(shader_frag_color_msl), X(shader_frag_color_dxil)}, 0, 0, SDL_GPU_SHADERSTAGE_FRAGMENT},
	{{X(shader_frag_texture_spirv), X(shader_frag_texture_msl), X(shader_frag_texture_dxil)}, 0, 1, SDL_GPU_SHADERSTAGE_FRAGMENT},
	{{X(shader_vert_spirv), X(shader_vert_msl), X(shader_vert_dxil)}, 2, 0, SDL_GPU_SHADERSTAGE_VERTEX}};

#undef X

static SDL_GPUShader* CreateShaderFromMemory(SDL_GPUDevice* device, ShaderType type)
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
	const Shader& shader = shaders[type];
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
	{
		Log::Message(Log::LT_ERROR, "Failed to create shader: %s", SDL_GetError());
		RMLUI_ERROR;
	}
	return sdl_shader;
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
	depth_stencil_format = SelectDepthStencilFormat(device);
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

bool RenderInterface_SDL_GPU::CreateShaders()
{
	color_shader = CreateShaderFromMemory(device, ShaderTypeColor);
	texture_shader = CreateShaderFromMemory(device, ShaderTypeTexture);
	vertex_shader = CreateShaderFromMemory(device, ShaderTypeVert);
	return color_shader && texture_shader && vertex_shader;
}

SDL_GPUGraphicsPipeline* RenderInterface_SDL_GPU::GetPipeline(ProgramId program, Rml::BlendMode blend)
{
	const PipelineKey key{program, blend};
	for (const PipelineEntry& entry : pipelines)
	{
		if (entry.key == key)
			return entry.pipeline;
	}

	SDL_GPUColorTargetDescription target{};
	target.format = layer_format;
	if (blend == Rml::BlendMode::Blend)
	{
		// Colours are premultiplied, so the source contributes in full.
		target.blend_state.enable_blend = true;
		target.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
		target.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
		target.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
		target.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
		target.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
		target.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	}

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
	info.vertex_shader = vertex_shader;
	info.fragment_shader = (program == ProgramId::Texture) ? texture_shader : color_shader;

	info.target_info.num_color_targets = 1;
	info.target_info.color_target_descriptions = &target;

	// The stencil buffer is attached to every pass so that clip masks can use it without reopening passes. Depth and
	// stencil tests stay off until there is a mask to apply.
	const SDL_GPUTextureFormat depth_stencil_format = render_layers.GetDepthStencilFormat();
	if (depth_stencil_format != SDL_GPU_TEXTUREFORMAT_INVALID)
	{
		info.target_info.has_depth_stencil_target = true;
		info.target_info.depth_stencil_format = depth_stencil_format;
	}

	info.vertex_input_state.num_vertex_attributes = 3;
	info.vertex_input_state.num_vertex_buffers = 1;
	info.vertex_input_state.vertex_attributes = attrib;
	info.vertex_input_state.vertex_buffer_descriptions = &buffer;

	SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(device, &info);
	if (!pipeline)
		Log::Message(Log::LT_ERROR, "Failed to create graphics pipeline: %s", SDL_GetError());

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
}

// The window is not used: layers have a format of their own, so nothing here depends on what the swapchain looks like.
// The parameter stays for the sake of the backend's call and of the renderers this one is modelled on.
RenderInterface_SDL_GPU::RenderInterface_SDL_GPU(SDL_GPUDevice* device, SDL_Window* /*window*/) : device(device)
{
	render_layers.Initialize(device);
	if (!CreateShaders())
	{
		Log::Message(Log::LT_ERROR, "Failed to create the renderer's shaders, nothing will be drawn");
		RMLUI_ERROR;
	}

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
	if (!linear_sampler)
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

	if (color_shader)
		SDL_ReleaseGPUShader(device, color_shader);
	if (texture_shader)
		SDL_ReleaseGPUShader(device, texture_shader);
	if (vertex_shader)
		SDL_ReleaseGPUShader(device, vertex_shader);
	if (linear_sampler)
		SDL_ReleaseGPUSampler(device, linear_sampler);

	color_shader = nullptr;
	texture_shader = nullptr;
	vertex_shader = nullptr;
	linear_sampler = nullptr;

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
	EnsureRenderPass(render_layers.GetTopLayer(), true);
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
	// Pipeline, resource bindings and scissor are all render pass state, so they must be re-applied whenever a new
	// pass begins. Uniforms belong to the command buffer, but re-pushing them is cheap insurance.
	bound_pipeline = nullptr;
	bound_texture = nullptr;
	bound_vertex_buffer = nullptr;
	bound_index_buffer = nullptr;
	transform_dirty = true;
	translation_dirty = true;
	scissor_dirty = true;
	// A new pass starts with the scissor covering the whole target, so the cached value no longer reflects reality.
	applied_scissor = {-1, -1, -1, -1};
}

bool RenderInterface_SDL_GPU::EnsureRenderPass(const RenderTarget& target, bool clear)
{
	if (render_pass && active_target_texture == target.color && !clear)
		return true;

	// Ended before the bail-out too, so that a caller which ignores the result can never go on to draw into whatever
	// target the previous pass happened to hold.
	EndRenderPass();

	if (!command_buffer || !target.color)
		return false;

	SDL_GPUColorTargetInfo color_info{};
	color_info.texture = target.color;
	color_info.load_op = clear ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD;
	color_info.store_op = SDL_GPU_STOREOP_STORE;
	// A cleared target is being written in full, so its previous contents need not be preserved for the GPU.
	color_info.cycle = clear;

	SDL_GPUDepthStencilTargetInfo depth_stencil_info{};
	SDL_GPUDepthStencilTargetInfo* depth_stencil_ptr = nullptr;
	if (SDL_GPUTexture* depth_stencil = render_layers.GetDepthStencil())
	{
		depth_stencil_info.texture = depth_stencil;
		depth_stencil_info.load_op = SDL_GPU_LOADOP_DONT_CARE;
		depth_stencil_info.store_op = SDL_GPU_STOREOP_DONT_CARE;
		// The stencil buffer is shared by all layers, so a pass that clears its colour starts the mask afresh too.
		depth_stencil_info.stencil_load_op = clear ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD;
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

	SDL_GPUGraphicsPipeline* pipeline = GetPipeline((texture != 0) ? ProgramId::Texture : ProgramId::Color, Rml::BlendMode::Blend);
	if (!pipeline)
		return;

	if (pipeline != bound_pipeline)
	{
		SDL_BindGPUGraphicsPipeline(render_pass, pipeline);
		bound_pipeline = pipeline;
		// Resource layouts differ between the pipelines, so do not assume the previous binding still applies.
		bound_texture = nullptr;
	}

	if (texture != 0)
	{
		SDL_GPUTexture* sdl_texture = reinterpret_cast<SDL_GPUTexture*>(texture);
		if (sdl_texture != bound_texture)
		{
			SDL_GPUTextureSamplerBinding texture_binding{};
			texture_binding.texture = sdl_texture;
			texture_binding.sampler = linear_sampler;
			SDL_BindGPUFragmentSamplers(render_pass, 0, &texture_binding, 1);
			bound_texture = sdl_texture;
		}
	}

	if (geometry->vertex_buffer->buffer != bound_vertex_buffer)
	{
		SDL_GPUBufferBinding binding{};
		binding.buffer = geometry->vertex_buffer->buffer;
		SDL_BindGPUVertexBuffers(render_pass, 0, &binding, 1);
		bound_vertex_buffer = binding.buffer;
	}

	if (geometry->index_buffer->buffer != bound_index_buffer)
	{
		SDL_GPUBufferBinding binding{};
		binding.buffer = geometry->index_buffer->buffer;
		SDL_BindGPUIndexBuffer(render_pass, &binding, SDL_GPU_INDEXELEMENTSIZE_32BIT);
		bound_index_buffer = binding.buffer;
	}

	if (scissor_dirty)
		ApplyScissor();

	if (transform_dirty)
	{
		SDL_PushGPUVertexUniformData(command_buffer, 0, &transform, sizeof(transform));
		transform_dirty = false;
	}

	if (translation_dirty || translation != pushed_translation)
	{
		SDL_PushGPUVertexUniformData(command_buffer, 1, &translation, sizeof(translation));
		pushed_translation = translation;
		translation_dirty = false;
	}

	SDL_DrawGPUIndexedPrimitives(render_pass, geometry->num_indices, 1, 0, 0, 0);
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
	const Rectanglei target = Rectanglei::FromSize({render_layers.GetWidth(), render_layers.GetHeight()});
	if (!scissor_enabled)
		return target;
	if (!scissor_region.Valid())
		return Rectanglei::FromSize({0, 0});
	return scissor_region;
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

// -- Layers ------------------------------------------------------------------

void RenderInterface_SDL_GPU::ReleaseQuads()
{
	if (composite_quad)
		ReleaseGeometry(composite_quad);
	if (clear_quad)
		ReleaseGeometry(clear_quad);

	composite_quad = {};
	clear_quad = {};
	quad_width = 0;
	quad_height = 0;
}

bool RenderInterface_SDL_GPU::EnsureQuads(int width, int height)
{
	if (composite_quad && clear_quad && quad_width == width && quad_height == height)
		return true;

	ReleaseQuads();

	const int indices[6] = {0, 1, 2, 0, 2, 3};
	const float w = static_cast<float>(width);
	const float h = static_cast<float>(height);

	// White and fully opaque, so the texture shader passes the sampled colour through unchanged.
	const ColourbPremultiplied white(255, 255, 255, 255);
	const Vertex composite_vertices[4] = {
		{{0.f, 0.f}, white, {0.f, 0.f}},
		{{w, 0.f}, white, {1.f, 0.f}},
		{{w, h}, white, {1.f, 1.f}},
		{{0.f, h}, white, {0.f, 1.f}},
	};
	composite_quad = CompileGeometry({composite_vertices, 4}, {indices, 6});

	// Drawn with blending off, this writes transparent black over whatever it covers.
	const ColourbPremultiplied transparent(0, 0, 0, 0);
	const Vertex clear_vertices[4] = {
		{{0.f, 0.f}, transparent, {0.f, 0.f}},
		{{w, 0.f}, transparent, {1.f, 0.f}},
		{{w, h}, transparent, {1.f, 1.f}},
		{{0.f, h}, transparent, {0.f, 1.f}},
	};
	clear_quad = CompileGeometry({clear_vertices, 4}, {indices, 6});

	if (!composite_quad || !clear_quad)
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

	SDL_GPUGraphicsPipeline* pipeline = GetPipeline(ProgramId::Color, Rml::BlendMode::Replace);
	if (!pipeline)
		return;

	GeometryView* geometry = reinterpret_cast<GeometryView*>(clear_quad);

	SDL_BindGPUGraphicsPipeline(render_pass, pipeline);
	bound_pipeline = pipeline;
	bound_texture = nullptr;

	SDL_GPUBufferBinding vertex_binding{};
	vertex_binding.buffer = geometry->vertex_buffer->buffer;
	SDL_BindGPUVertexBuffers(render_pass, 0, &vertex_binding, 1);
	bound_vertex_buffer = vertex_binding.buffer;

	SDL_GPUBufferBinding index_binding{};
	index_binding.buffer = geometry->index_buffer->buffer;
	SDL_BindGPUIndexBuffer(render_pass, &index_binding, SDL_GPU_INDEXELEMENTSIZE_32BIT);
	bound_index_buffer = index_binding.buffer;

	if (scissor_dirty)
		ApplyScissor();

	const Vector2f no_translation(0.f, 0.f);
	SDL_PushGPUVertexUniformData(command_buffer, 0, &projection, sizeof(projection));
	SDL_PushGPUVertexUniformData(command_buffer, 1, &no_translation, sizeof(no_translation));
	transform_dirty = true;
	translation_dirty = true;

	SDL_DrawGPUIndexedPrimitives(render_pass, geometry->num_indices, 1, 0, 0, 0);
}

void RenderInterface_SDL_GPU::DrawTextureToTarget(const RenderTarget& destination, SDL_GPUTexture* source, Rml::BlendMode blend)
{
	if (!source || !composite_quad)
		return;
	if (!FlushGeometryUploads())
		return;
	if (!EnsureRenderPass(destination))
		return;

	SDL_GPUGraphicsPipeline* pipeline = GetPipeline(ProgramId::Texture, blend);
	if (!pipeline)
		return;

	GeometryView* geometry = reinterpret_cast<GeometryView*>(composite_quad);

	SDL_BindGPUGraphicsPipeline(render_pass, pipeline);
	bound_pipeline = pipeline;

	SDL_GPUTextureSamplerBinding texture_binding{};
	texture_binding.texture = source;
	texture_binding.sampler = linear_sampler;
	SDL_BindGPUFragmentSamplers(render_pass, 0, &texture_binding, 1);
	bound_texture = source;

	SDL_GPUBufferBinding vertex_binding{};
	vertex_binding.buffer = geometry->vertex_buffer->buffer;
	SDL_BindGPUVertexBuffers(render_pass, 0, &vertex_binding, 1);
	bound_vertex_buffer = vertex_binding.buffer;

	SDL_GPUBufferBinding index_binding{};
	index_binding.buffer = geometry->index_buffer->buffer;
	SDL_BindGPUIndexBuffer(render_pass, &index_binding, SDL_GPU_INDEXELEMENTSIZE_32BIT);
	bound_index_buffer = index_binding.buffer;

	if (scissor_dirty)
		ApplyScissor();

	// The quad is given in target coordinates, so it must not pick up whatever transform the elements were using.
	const Vector2f no_translation(0.f, 0.f);
	SDL_PushGPUVertexUniformData(command_buffer, 0, &projection, sizeof(projection));
	SDL_PushGPUVertexUniformData(command_buffer, 1, &no_translation, sizeof(no_translation));
	transform_dirty = true;
	translation_dirty = true;

	SDL_DrawGPUIndexedPrimitives(render_pass, geometry->num_indices, 1, 0, 0, 0);
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
		// No quad to draw the cleared region with. Clearing the whole attachment is still correct, only slower; it
		// also resets the stencil, which costs nothing while there is no clip mask to preserve.
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
	// Filters arrive with the filter support of a later phase; until then CompileFilter() returns zero and RmlUi never
	// gets a handle to pass here. Their presence already selects the postprocess path below, which is where they will
	// run once there is something to run.

	// Compositing goes via a postprocess target when the source cannot simply be sampled into the destination. The
	// contract allows both handles to name the same layer, and a texture cannot be a colour attachment and a sampler
	// binding at once; filters, when they arrive, will also read and write that target in turn. Everything else takes
	// the direct path: the detour costs a second full-region draw, which measures at about a quarter of the frame on a
	// filter-heavy document.
	// A layer whose texture could not be created has nothing to composite. Caught here rather than in
	// DrawTextureToTarget(), which would otherwise skip the first hop of the postprocess path and then blend whatever
	// the previous composite left in that target.
	if (!render_layers.GetLayer(source).color)
		return;

	const bool via_postprocess = (source == destination) || !filters.empty();

	if (via_postprocess)
	{
		const RenderTarget& postprocess = render_layers.GetPostprocessPrimary();
		DrawTextureToTarget(postprocess, render_layers.GetLayer(source).color, Rml::BlendMode::Replace);
		DrawTextureToTarget(render_layers.GetLayer(destination), postprocess.color, blend_mode);
	}
	else
	{
		DrawTextureToTarget(render_layers.GetLayer(destination), render_layers.GetLayer(source).color, blend_mode);
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
