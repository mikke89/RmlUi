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

// -- Setup -------------------------------------------------------------------

void RenderInterface_SDL_GPU::CreatePipelines()
{
	SDL_GPUShader* color_shader = CreateShaderFromMemory(device, ShaderTypeColor);
	SDL_GPUShader* texture_shader = CreateShaderFromMemory(device, ShaderTypeTexture);
	SDL_GPUShader* vert_shader = CreateShaderFromMemory(device, ShaderTypeVert);

	SDL_GPUColorTargetDescription target{};
	target.format = SDL_GetGPUSwapchainTextureFormat(device, window);
	target.blend_state.enable_blend = true;
	target.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
	target.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
	target.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
	target.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
	target.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	target.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

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
	info.vertex_shader = vert_shader;

	info.target_info.num_color_targets = 1;
	info.target_info.color_target_descriptions = &target;

	info.vertex_input_state.num_vertex_attributes = 3;
	info.vertex_input_state.num_vertex_buffers = 1;
	info.vertex_input_state.vertex_attributes = attrib;
	info.vertex_input_state.vertex_buffer_descriptions = &buffer;

	info.fragment_shader = color_shader;
	color_pipeline = SDL_CreateGPUGraphicsPipeline(device, &info);
	if (!color_pipeline)
	{
		Log::Message(Log::LT_ERROR, "Failed to create color pipeline: %s", SDL_GetError());
		RMLUI_ERROR;
	}

	info.fragment_shader = texture_shader;
	texture_pipeline = SDL_CreateGPUGraphicsPipeline(device, &info);
	if (!texture_pipeline)
	{
		Log::Message(Log::LT_ERROR, "Failed to create texture pipeline: %s", SDL_GetError());
		RMLUI_ERROR;
	}

	SDL_ReleaseGPUShader(device, color_shader);
	SDL_ReleaseGPUShader(device, texture_shader);
	SDL_ReleaseGPUShader(device, vert_shader);
}

RenderInterface_SDL_GPU::RenderInterface_SDL_GPU(SDL_GPUDevice* device, SDL_Window* window) : device(device), window(window)
{
	CreatePipelines();

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

	// The GPU may still be reading from resources submitted in earlier frames. Releasing them while in flight is
	// allowed, but waiting here keeps shutdown ordering simple to reason about.
	SDL_WaitForGPUIdle(device);

	pending_uploads.clear();
	vertex_buffers.ReleaseAll();
	index_buffers.ReleaseAll();

	if (linear_sampler)
		SDL_ReleaseGPUSampler(device, linear_sampler);
	if (color_pipeline)
		SDL_ReleaseGPUGraphicsPipeline(device, color_pipeline);
	if (texture_pipeline)
		SDL_ReleaseGPUGraphicsPipeline(device, texture_pipeline);

	linear_sampler = nullptr;
	color_pipeline = nullptr;
	texture_pipeline = nullptr;

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

	projection = Matrix4f::ProjectOrtho(0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, -10'000.f, 10'000.f);
	transform = projection;

	scissor_enabled = false;
	scissor_region = Rectanglei::FromSize({static_cast<int>(width), static_cast<int>(height)});

	InvalidateRenderPassState();

#if RMLUI_BACKEND_SDL_GPU_DEBUG
	SDL_PushGPUDebugGroup(command_buffer, "RmlUi frame");
#endif
}

void RenderInterface_SDL_GPU::EndFrame()
{
	EndRenderPass();

#if RMLUI_BACKEND_SDL_GPU_DEBUG
	if (command_buffer)
		SDL_PopGPUDebugGroup(command_buffer);
#endif

	// The caller submits the frame's command buffer right after this returns, so this is the last moment at which the
	// transfers this frame depends on can still be placed ahead of it. It also drains anything queued while the window
	// was minimized, since then no frame is ever begun.
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

bool RenderInterface_SDL_GPU::EnsureRenderPass()
{
	if (render_pass)
		return true;
	if (!command_buffer || !swapchain_texture)
		return false;

	SDL_GPUColorTargetInfo color_info{};
	color_info.texture = swapchain_texture;
	color_info.load_op = SDL_GPU_LOADOP_LOAD;
	color_info.store_op = SDL_GPU_STOREOP_STORE;
	render_pass = SDL_BeginGPURenderPass(command_buffer, &color_info, 1, nullptr);
	if (!render_pass)
	{
		Log::Message(Log::LT_ERROR, "Failed to begin render pass: %s", SDL_GetError());
		return false;
	}

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
	if (!EnsureRenderPass())
		return;

	SDL_GPUGraphicsPipeline* pipeline = (texture != 0) ? texture_pipeline : color_pipeline;
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

	SDL_GPUTexture* texture = nullptr;
	{
		SDL_GPUTextureCreateInfo info{};
		info.type = SDL_GPU_TEXTURETYPE_2D;
		info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
		info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
		info.width = source_dimensions.x;
		info.height = source_dimensions.y;
		info.layer_count_or_depth = 1;
		info.num_levels = 1;
		texture = SDL_CreateGPUTexture(device, &info);
		if (!texture)
		{
			Log::Message(Log::LT_ERROR, "Failed to create texture: %s", SDL_GetError());
			SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
			return 0;
		}
	}

	SDL_SetGPUTextureName(device, texture, "RmlUi texture");

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

void RenderInterface_SDL_GPU::ApplyScissor()
{
	const Rectanglei target = Rectanglei::FromSize({static_cast<int>(swapchain_width), static_cast<int>(swapchain_height)});
	// RmlUi can submit regions reaching outside the render target; SDL requires the scissor to stay within it.
	const Rectanglei region = scissor_enabled ? scissor_region.IntersectIfValid(target) : target;

	SDL_Rect rect;
	rect.x = Math::Clamp(region.Left(), 0, target.Right());
	rect.y = Math::Clamp(region.Top(), 0, target.Bottom());
	rect.w = Math::Max(Math::Min(region.Right(), target.Right()) - rect.x, 0);
	rect.h = Math::Max(Math::Min(region.Bottom(), target.Bottom()) - rect.y, 0);

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
