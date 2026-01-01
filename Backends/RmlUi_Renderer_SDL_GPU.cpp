#include "RmlUi_Renderer_SDL_GPU.h"
#include "RmlUi_SDL_GPU/ShadersCompiledSPV.h"
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Core/Log.h>
#include <RmlUi/Core/Types.h>
#include <SDL3_image/SDL_image.h>

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

RenderInterface_SDL_GPU::RenderInterface_SDL_GPU(SDL_GPUDevice* device, SDL_Window* window) :
	device(device), window(window), copy_pass(nullptr), render_pass(nullptr)
{
	CreatePipelines();

	SDL_GPUSamplerCreateInfo info{};
	info.min_filter = SDL_GPU_FILTER_LINEAR;
	info.mag_filter = SDL_GPU_FILTER_LINEAR;
	info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
	info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
	info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
	info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
	linear_sampler = SDL_CreateGPUSampler(device, &info);
	if (!linear_sampler)
	{
		Log::Message(Log::LT_ERROR, "Failed to acquire command buffer: %s", SDL_GetError());
		return;
	}
}

void RenderInterface_SDL_GPU::Shutdown()
{
	for (Rml::UniquePtr<Command>& command : commands)
	{
		command->Update(*this);
	}
	for (Rml::UniquePtr<Buffer>& buffer : buffers)
	{
		SDL_ReleaseGPUTransferBuffer(device, buffer->transfer_buffer);
		SDL_ReleaseGPUBuffer(device, buffer->buffer);
	}
	SDL_ReleaseGPUSampler(device, linear_sampler);
	SDL_ReleaseGPUGraphicsPipeline(device, color_pipeline);
	SDL_ReleaseGPUGraphicsPipeline(device, texture_pipeline);
}

void RenderInterface_SDL_GPU::BeginFrame(SDL_GPUCommandBuffer* command_buffer, SDL_GPUTexture* swapchain_texture, uint32_t width, uint32_t height)
{
	this->command_buffer = command_buffer;
	this->swapchain_texture = swapchain_texture;
	swapchain_width = width;
	swapchain_height = height;
	proj = Matrix4f::ProjectOrtho(0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, -10'000.f, 10'000.f);
	SetTransform(nullptr);
	EnableScissorRegion(false);
}

void RenderInterface_SDL_GPU::EndFrame()
{
	for (Rml::UniquePtr<Command>& command : commands)
	{
		command->Update(*this);
	}
	commands.clear();

	if (copy_pass)
	{
		SDL_EndGPUCopyPass(copy_pass);
		copy_pass = nullptr;
	}
	if (render_pass)
	{
		SDL_EndGPURenderPass(render_pass);
		render_pass = nullptr;
	}
}

bool RenderInterface_SDL_GPU::BeginCopyPass()
{
	if (copy_pass)
	{
		return true;
	}
	if (render_pass)
	{
		SDL_EndGPURenderPass(render_pass);
		render_pass = nullptr;
	}
	copy_pass = SDL_BeginGPUCopyPass(command_buffer);
	if (!copy_pass)
	{
		Log::Message(Log::LT_ERROR, "Failed to begin copy pass: %s", SDL_GetError());
		return false;
	}
	return true;
}

bool RenderInterface_SDL_GPU::BeginRenderPass()
{
	if (render_pass)
	{
		return true;
	}
	if (copy_pass)
	{
		SDL_EndGPUCopyPass(copy_pass);
		copy_pass = nullptr;
	}
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
	return true;
}

CompiledGeometryHandle RenderInterface_SDL_GPU::CompileGeometry(Span<const Vertex> vertices, Span<const int> indices)
{
	if (!BeginCopyPass())
	{
		return 0;
	}

	uint32_t vertex_size = static_cast<int>(vertices.size() * sizeof(Vertex));
	uint32_t index_size = static_cast<int>(indices.size() * sizeof(int));

	GeometryView* geometry = new GeometryView();
	geometry->vertex_buffer = RequestBuffer(vertex_size, SDL_GPU_BUFFERUSAGE_VERTEX);
	geometry->index_buffer = RequestBuffer(index_size, SDL_GPU_BUFFERUSAGE_INDEX);
	if (!geometry->vertex_buffer || !geometry->index_buffer)
	{
		Log::Message(Log::LT_ERROR, "Failed to request buffer(s)");
		delete geometry;
		return 0;
	}

	void* vertex_data = SDL_MapGPUTransferBuffer(device, geometry->vertex_buffer->transfer_buffer, true);
	void* index_data = SDL_MapGPUTransferBuffer(device, geometry->index_buffer->transfer_buffer, true);
	if (!vertex_data || !index_data)
	{
		Log::Message(Log::LT_ERROR, "Failed to map transfer buffer(s): %s", SDL_GetError());
		delete geometry;
		return 0;
	}

	std::memcpy(vertex_data, vertices.data(), vertex_size);
	std::memcpy(index_data, indices.data(), index_size);
	SDL_UnmapGPUTransferBuffer(device, geometry->vertex_buffer->transfer_buffer);
	SDL_UnmapGPUTransferBuffer(device, geometry->index_buffer->transfer_buffer);

	SDL_GPUTransferBufferLocation location{};
	SDL_GPUBufferRegion region{};

	location.transfer_buffer = geometry->vertex_buffer->transfer_buffer;
	region.buffer = geometry->vertex_buffer->buffer;
	region.size = vertex_size;
	SDL_UploadToGPUBuffer(copy_pass, &location, &region, false);

	location.transfer_buffer = geometry->index_buffer->transfer_buffer;
	region.buffer = geometry->index_buffer->buffer;
	region.size = index_size;
	SDL_UploadToGPUBuffer(copy_pass, &location, &region, false);

	geometry->num_indices = static_cast<int>(indices.size());
	geometry->vertex_buffer->in_use = true;
	geometry->index_buffer->in_use = true;

	return reinterpret_cast<Rml::CompiledGeometryHandle>(geometry);
}

void RenderInterface_SDL_GPU::ReleaseGeometryCommand::Update(RenderInterface_SDL_GPU& interface)
{
	(void)interface;
	GeometryView* geometry = reinterpret_cast<GeometryView*>(handle);
	geometry->vertex_buffer->in_use = false;
	geometry->index_buffer->in_use = false;
	delete geometry;
}

void RenderInterface_SDL_GPU::ReleaseGeometry(CompiledGeometryHandle handle)
{
	commands.push_back(Rml::MakeUnique<ReleaseGeometryCommand>(handle));
}

void RenderInterface_SDL_GPU::RenderGeometryCommand::Update(RenderInterface_SDL_GPU& interface)
{
	if (!interface.BeginRenderPass())
	{
		return;
	}

	GeometryView* geometry = reinterpret_cast<GeometryView*>(handle);

	if (texture != 0)
	{
		SDL_BindGPUGraphicsPipeline(interface.render_pass, interface.texture_pipeline);

		SDL_GPUTextureSamplerBinding texture_binding{};
		texture_binding.texture = reinterpret_cast<SDL_GPUTexture*>(texture);
		texture_binding.sampler = interface.linear_sampler;
		SDL_BindGPUFragmentSamplers(interface.render_pass, 0, &texture_binding, 1);
	}
	else
	{
		SDL_BindGPUGraphicsPipeline(interface.render_pass, interface.color_pipeline);
	}

	SDL_GPUBufferBinding vertex_buffer_binding{};
	SDL_GPUBufferBinding index_buffer_binding{};
	vertex_buffer_binding.buffer = geometry->vertex_buffer->buffer;
	index_buffer_binding.buffer = geometry->index_buffer->buffer;

	SDL_BindGPUVertexBuffers(interface.render_pass, 0, &vertex_buffer_binding, 1);
	SDL_BindGPUIndexBuffer(interface.render_pass, &index_buffer_binding, SDL_GPU_INDEXELEMENTSIZE_32BIT);

	SDL_SetGPUScissor(interface.render_pass, &interface.scissor);

	SDL_PushGPUVertexUniformData(interface.command_buffer, 0, &interface.transform, sizeof(interface.transform));
	SDL_PushGPUVertexUniformData(interface.command_buffer, 1, &translation, sizeof(translation));

	SDL_DrawGPUIndexedPrimitives(interface.render_pass, geometry->num_indices, 1, 0, 0, 0);
}

void RenderInterface_SDL_GPU::RenderGeometry(CompiledGeometryHandle handle, Vector2f translation, TextureHandle texture)
{
	commands.push_back(Rml::MakeUnique<RenderGeometryCommand>(handle, translation, texture));
}

void RenderInterface_SDL_GPU::EnableScissorRegionCommand::Update(RenderInterface_SDL_GPU& interface)
{
	if (!enable)
	{
		interface.scissor.x = 0;
		interface.scissor.y = 0;
		interface.scissor.w = interface.swapchain_width;
		interface.scissor.h = interface.swapchain_height;
	}
}

void RenderInterface_SDL_GPU::EnableScissorRegion(bool enable)
{
	commands.push_back(Rml::MakeUnique<EnableScissorRegionCommand>(enable));
}

void RenderInterface_SDL_GPU::SetScissorRegionCommand::Update(RenderInterface_SDL_GPU& interface)
{
	interface.scissor.x = region.Left();
	interface.scissor.w = region.Width();
	interface.scissor.y = region.Top();
	interface.scissor.h = region.Height();
}

void RenderInterface_SDL_GPU::SetScissorRegion(Rectanglei region)
{
	commands.push_back(Rml::MakeUnique<SetScissorRegionCommand>(region));
}

TextureHandle RenderInterface_SDL_GPU::LoadTexture(Vector2i& texture_dimensions, const String& source)
{
	Rml::FileInterface* file_interface = Rml::GetFileInterface();
	Rml::FileHandle file_handle = file_interface->Open(source);
	if (!file_handle)
	{
		return 0;
	}

	file_interface->Seek(file_handle, 0, SEEK_END);
	size_t buffer_size = file_interface->Tell(file_handle);
	file_interface->Seek(file_handle, 0, SEEK_SET);

	using Rml::byte;
	Rml::UniquePtr<byte[]> buffer(new byte[buffer_size]);
	file_interface->Read(buffer.get(), buffer_size, file_handle);
	file_interface->Close(file_handle);

	const size_t i_ext = source.rfind('.');
	Rml::String extension = (i_ext == Rml::String::npos ? Rml::String() : source.substr(i_ext + 1));

	SDL_Surface* surface = IMG_LoadTyped_IO(SDL_IOFromMem(buffer.get(), int(buffer_size)), 1, extension.c_str());
	if (!surface)
	{
		return 0;
	}

	texture_dimensions = {surface->w, surface->h};

	if (surface->format != SDL_PIXELFORMAT_RGBA32)
	{
		SDL_Surface* converted_surface = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
		SDL_DestroySurface(surface);
		if (!converted_surface)
		{
			return 0;
		}

		surface = converted_surface;
	}

	// Convert colors to premultiplied alpha, which is necessary for correct alpha compositing.
	const size_t pixels_byte_size = surface->w * surface->h * 4;
	byte* pixels = static_cast<byte*>(surface->pixels);
	for (size_t i = 0; i < pixels_byte_size; i += 4)
	{
		const byte alpha = pixels[i + 3];
		for (size_t j = 0; j < 3; ++j)
		{
			pixels[i + j] = byte(int(pixels[i + j]) * int(alpha) / 255);
		}
	}

	Span<const byte> data{static_cast<const byte*>(surface->pixels), static_cast<size_t>(surface->pitch * surface->h)};
	texture_dimensions = {surface->w, surface->h};

	TextureHandle handle = GenerateTexture(data, texture_dimensions);

	SDL_DestroySurface(surface);

	return handle;
}

TextureHandle RenderInterface_SDL_GPU::GenerateTexture(Span<const byte> source, Vector2i source_dimensions)
{
	SDL_GPUTransferBuffer* transfer_buffer;
	{
		SDL_GPUTransferBufferCreateInfo info{};
		info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
		info.size = source_dimensions.x * source_dimensions.y * 4;
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
		SDL_Log("Failed to map transfer buffer: %s", SDL_GetError());
		return 0;
	}

	std::memcpy(dst, source.data(), source_dimensions.x * source_dimensions.y * 4);
	SDL_UnmapGPUTransferBuffer(device, transfer_buffer);

	SDL_GPUTexture* texture;
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
			return 0;
		}
	}

	SDL_GPUTextureTransferInfo transfer_info{};
	SDL_GPUTextureRegion region{};
	transfer_info.transfer_buffer = transfer_buffer;
	region.texture = texture;
	region.w = source_dimensions.x;
	region.h = source_dimensions.y;
	region.d = 1;

	// We can get calls out outside of Begin/End Frame so always acquire a command buffer here
	SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(device);
	if (!command_buffer)
	{
		Log::Message(Log::LT_ERROR, "Failed to acquire command buffer: %s", SDL_GetError());
		SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
		SDL_ReleaseGPUTexture(device, texture);
		return 0;
	}

	SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(command_buffer);
	if (!copy_pass)
	{
		Log::Message(Log::LT_ERROR, "Failed to begin copy pass: %s", SDL_GetError());
		SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
		SDL_ReleaseGPUTexture(device, texture);
		SDL_CancelGPUCommandBuffer(command_buffer);
		return 0;
	}

	SDL_UploadToGPUTexture(copy_pass, &transfer_info, &region, false);
	SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
	SDL_EndGPUCopyPass(copy_pass);
	SDL_SubmitGPUCommandBuffer(command_buffer);

	return reinterpret_cast<TextureHandle>(texture);
}

void RenderInterface_SDL_GPU::ReleaseTextureCommand::Update(RenderInterface_SDL_GPU& interface)
{
	SDL_GPUTexture* texture = reinterpret_cast<SDL_GPUTexture*>(handle);
	SDL_ReleaseGPUTexture(interface.device, texture);
}

void RenderInterface_SDL_GPU::ReleaseTexture(TextureHandle texture_handle)
{
	commands.push_back(Rml::MakeUnique<ReleaseTextureCommand>(texture_handle));
}

RenderInterface_SDL_GPU::SetTransformCommand::SetTransformCommand(const Rml::Matrix4f* new_transform)
{
	if (new_transform)
	{
		has_transform = true;
		transform = *new_transform;
	}
	else
	{
		has_transform = false;
	}
}

void RenderInterface_SDL_GPU::SetTransformCommand::Update(RenderInterface_SDL_GPU& interface)
{
	if (has_transform)
	{
		interface.transform = interface.proj * transform;
	}
	else
	{
		interface.transform = interface.proj;
	}
}

void RenderInterface_SDL_GPU::SetTransform(const Rml::Matrix4f* new_transform)
{
	commands.push_back(Rml::MakeUnique<SetTransformCommand>(new_transform));
}

RenderInterface_SDL_GPU::Buffer* RenderInterface_SDL_GPU::RequestBuffer(int capacity, SDL_GPUBufferUsageFlags usage)
{
	auto it = std::lower_bound(buffers.begin(), buffers.end(), capacity,
		[](const Rml::UniquePtr<Buffer>& lhs, int capacity) { return lhs->capacity < capacity; });

	for (auto tmp_it = it; tmp_it != buffers.end(); ++tmp_it)
	{
		const auto& buffer = *tmp_it;
		if (!buffer->in_use && buffer->usage == usage)
		{
			// set in_use as false and expect the caller to set it to true themselves
			buffer->in_use = false;
			return buffer.get();
		}
	}

	Rml::UniquePtr<Buffer> buffer = Rml::MakeUnique<Buffer>();
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
		return {};
	}
	buffer->usage = usage;
	buffer->in_use = false;
	buffer->capacity = capacity;
	auto inserted_it = buffers.insert(it, std::move(buffer));
	return inserted_it->get();
}
