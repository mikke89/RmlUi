/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019 The RmlUi Team, and contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "RmlUi_Renderer_Vulkan.h"
#include "RmlUi_Vulkan/ShadersCompiledSPV.h"
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Core/Log.h>
#include <RmlUi/Core/Platform.h>
#include <RmlUi/Core/Profiling.h>
#include <string.h>

// probably a compiler's bug because idk how to fix it
#ifdef __clang__
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif

VkValidationFeaturesEXT debug_validation_features_ext = {};
VkValidationFeatureEnableEXT debug_validation_features_ext_requested[] = {VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
	VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT, VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT};

#ifdef RMLUI_DEBUG
static VKAPI_ATTR VkBool32 VKAPI_CALL MyDebugReportCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT /*objectType*/,
	uint64_t /*object*/, size_t /*location*/, int32_t /*messageCode*/, const char* /*pLayerPrefix*/, const char* pMessage, void* /*pUserData*/)
{
	if (flags & VkDebugReportFlagBitsEXT::VK_DEBUG_REPORT_INFORMATION_BIT_EXT || flags & VkDebugReportFlagBitsEXT::VK_DEBUG_REPORT_DEBUG_BIT_EXT)
	{
		return VK_FALSE;
	}

	#ifdef RMLUI_PLATFORM_WIN32
	if (flags & VkDebugReportFlagBitsEXT::VK_DEBUG_REPORT_ERROR_BIT_EXT)
	{
		// some logs are not passed to our UI, because of early calling for explicity I put native log output
		OutputDebugString(TEXT("\n"));
		OutputDebugStringA(pMessage);
	}
	#endif

	Rml::Log::Message(Rml::Log::LT_ERROR, "[Vulkan][VALIDATION] %s ", pMessage);

	return VK_FALSE;
}
#endif

RenderInterface_Vulkan::RenderInterface_Vulkan() :
	m_is_transform_enabled{false}, m_is_apply_to_regular_geometry_stencil{false}, m_is_use_scissor_specified{false}, m_is_use_stencil_pipeline{false},
	m_is_can_render{true}, m_width{}, m_height{}, m_queue_index_present{}, m_queue_index_graphics{}, m_queue_index_compute{}, m_semaphore_index{},
	m_semaphore_index_previous{}, m_image_index{}, m_current_descriptor_id{}, m_p_instance{}, m_p_device{}, m_p_physical_device_current{},
	m_current_physical_device_properties{}, m_p_surface{}, m_p_swapchain{}, m_p_allocator{}, m_p_current_command_buffer{},
	m_p_descriptor_set_layout_uniform_buffer_dynamic{}, m_p_descriptor_set_layout_for_textures{}, m_p_pipeline_layout{}, m_p_pipeline_with_textures{},
	m_p_pipeline_without_textures{}, m_p_pipeline_stencil_for_region_where_geometry_will_be_drawn{},
	m_p_pipeline_stencil_for_regular_geometry_that_applied_to_region_with_textures{},
	m_p_pipeline_stencil_for_regular_geometry_that_applied_to_region_without_textures{}, m_p_descriptor_set{}, m_p_render_pass{},
	m_p_sampler_linear{}, m_scissor{}, m_scissor_original{}, m_viewport{}, m_p_queue_present{}, m_p_queue_graphics{}, m_p_queue_compute{},
#ifdef RMLUI_DEBUG
	m_debug_report_callback_instance{},
#endif
	m_swapchain_format{}
{}

RenderInterface_Vulkan::~RenderInterface_Vulkan(void) {}

void RenderInterface_Vulkan::RenderGeometry(Rml::Vertex* vertices, int num_vertices, int* indices, int num_indices, Rml::TextureHandle texture,
	const Rml::Vector2f& translation)
{
	Rml::CompiledGeometryHandle handle = this->CompileGeometry(vertices, num_vertices, indices, num_indices, texture);

	if (handle)
	{
		this->RenderCompiledGeometry(handle, translation);
		this->ReleaseCompiledGeometry(handle);
	}
}

Rml::CompiledGeometryHandle RenderInterface_Vulkan::CompileGeometry(Rml::Vertex* vertices, int num_vertices, int* indices, int num_indices,
	Rml::TextureHandle texture)
{
	RMLUI_ZoneScopedN("Vulkan - CompileGeometry");

	texture_data_t* p_texture = reinterpret_cast<texture_data_t*>(texture);

	VkDescriptorSet p_current_descriptor_set = nullptr;
	p_current_descriptor_set = this->m_p_descriptor_set;

	VK_ASSERT(p_current_descriptor_set,
		"you can't have here an invalid pointer of VkDescriptorSet. Two reason might be. 1. - you didn't allocate it "
		"at all or 2. - Somehing is wrong with allocation and somehow it was corrupted by something.");

	auto* p_geometry_handle = new geometry_handle_t();

	VkDescriptorImageInfo info_descriptor_image = {};
	if (p_texture && p_texture->Get_VkDescriptorSet() == nullptr)
	{
		VkDescriptorSet p_texture_set = nullptr;
		this->m_manager_descriptors.Alloc_Descriptor(this->m_p_device, &this->m_p_descriptor_set_layout_for_textures, &p_texture_set);

		info_descriptor_image.imageView = p_texture->Get_VkImageView();
		info_descriptor_image.sampler = p_texture->Get_VkSampler();
		info_descriptor_image.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkWriteDescriptorSet info_write = {};

		info_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		info_write.dstSet = p_texture_set;
		info_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		info_write.dstBinding = 2;
		info_write.pImageInfo = &info_descriptor_image;
		info_write.descriptorCount = 1;

		vkUpdateDescriptorSets(this->m_p_device, 1, &info_write, 0, nullptr);
		p_texture->Set_VkDescriptorSet(p_texture_set);
	}

	uint32_t* pCopyDataToBuffer = nullptr;
	const void* pData = reinterpret_cast<const void*>(vertices);

	bool status = this->m_memory_pool.Alloc_VertexBuffer(num_vertices, sizeof(Rml::Vertex), reinterpret_cast<void**>(&pCopyDataToBuffer),
		&p_geometry_handle->m_p_vertex, &p_geometry_handle->m_p_vertex_allocation);
	VK_ASSERT(status, "failed to AllocVertexBuffer");

	memcpy(pCopyDataToBuffer, pData, sizeof(Rml::Vertex) * num_vertices);

	status = this->m_memory_pool.Alloc_IndexBuffer(num_indices, sizeof(int), reinterpret_cast<void**>(&pCopyDataToBuffer),
		&p_geometry_handle->m_p_index, &p_geometry_handle->m_p_index_allocation);
	VK_ASSERT(status, "failed to AllocIndexBuffer");

	memcpy(pCopyDataToBuffer, indices, sizeof(int) * num_indices);

	p_geometry_handle->m_is_has_texture = !!((texture_data_t*)(texture));
	p_geometry_handle->m_num_indices = num_indices;
	p_geometry_handle->m_descriptor_id = this->Get_CurrentDescriptorID();
	p_geometry_handle->m_is_cached = false;
	p_geometry_handle->m_p_texture = p_texture;

	return Rml::CompiledGeometryHandle(p_geometry_handle);
}

void RenderInterface_Vulkan::RenderCompiledGeometry(Rml::CompiledGeometryHandle geometry, const Rml::Vector2f& translation)
{
	RMLUI_ZoneScopedN("Vulkan - RenderCompiledGeometry");

	if (this->m_p_current_command_buffer == nullptr)
		return;

	VK_ASSERT(this->m_p_current_command_buffer, "must be valid otherwise you can't render now!!! (can't be)");

	geometry_handle_t* p_casted_compiled_geometry = reinterpret_cast<geometry_handle_t*>(geometry);

	this->m_user_data_for_vertex_shader.m_translate = translation;

	VkDescriptorSet p_current_descriptor_set = nullptr;
	p_current_descriptor_set = this->m_p_descriptor_set;

	VK_ASSERT(p_current_descriptor_set,
		"you can't have here an invalid pointer of VkDescriptorSet. Two reason might be. 1. - you didn't allocate it "
		"at all or 2. - Somehing is wrong with allocation and somehow it was corrupted by something.");

	shader_vertex_user_data_t* p_data = nullptr;

	if (p_casted_compiled_geometry->m_p_shader_allocation == nullptr)
	{
		// it means it was freed in ReleaseCompiledGeometry method
		bool status = this->m_memory_pool.Alloc_GeneralBuffer(sizeof(this->m_user_data_for_vertex_shader), reinterpret_cast<void**>(&p_data),
			&p_casted_compiled_geometry->m_p_shader, &p_casted_compiled_geometry->m_p_shader_allocation);
		VK_ASSERT(status, "failed to allocate VkDescriptorBufferInfo for uniform data to shaders");
	}
	else
	{
		// it means our state is dirty and we need to update data, but it is not right in terms of architecture, for real better experience would
		// be great to free all "compiled" geometries and "re-build" them in one general way, but here I got only three callings for
		// font-face-layer textures (loaddocument example) and that shit. So better to think how to make it right, if it is fine okay, if it is
		// not okay and like we really expect that ReleaseCompiledGeometry for all objects that needs to be rebuilt so better to implement that,
		// but still it is a big architectural thing (or at least you need to do something big commits here to implement a such feature), so my
		// implementation doesn't break anything what we had, but still it looks strange. If I get callings for releasing maybe I need to use it
		// for all objects not separately????? Otherwise it is better to provide method for resizing (or some kind of "resizing" callback) for
		// recalculating all geometry IDK, so it means you pass the existed geometry that wasn't pass to ReleaseCompiledGeometry, but from another
		// hand you need to re-build compiled geometry again so we have two kinds of geometry one is compiled and never changes and one is dynamic
		// and it goes through pipeline InitializationOfProgram...->Compile->Render->Release->Compile->Render->Release...

		this->m_memory_pool.Free_GeometryHandle_ShaderDataOnly(p_casted_compiled_geometry);
		bool status = this->m_memory_pool.Alloc_GeneralBuffer(sizeof(this->m_user_data_for_vertex_shader), reinterpret_cast<void**>(&p_data),
			&p_casted_compiled_geometry->m_p_shader, &p_casted_compiled_geometry->m_p_shader_allocation);
		VK_ASSERT(status, "failed to allocate VkDescriptorBufferInfo for uniform data to shaders");
	}

	if (p_data)
	{
		p_data->m_transform = this->m_user_data_for_vertex_shader.m_transform;
		p_data->m_translate = this->m_user_data_for_vertex_shader.m_translate;
	}
	else
	{
		VK_ASSERT(p_data, "you can't reach this zone, it means something bad");
	}

	const uint32_t pDescriptorOffsets = static_cast<uint32_t>(p_casted_compiled_geometry->m_p_shader.offset);

	VkDescriptorSet p_texture_descriptor_set = nullptr;

	if (p_casted_compiled_geometry->m_p_texture)
	{
		p_texture_descriptor_set = p_casted_compiled_geometry->m_p_texture->Get_VkDescriptorSet();
	}

	VkDescriptorSet p_sets[] = {p_current_descriptor_set, p_texture_descriptor_set};
	int real_size_of_sets = 2;

	if (p_casted_compiled_geometry->m_p_texture == nullptr)
		real_size_of_sets = 1;

	vkCmdBindDescriptorSets(this->m_p_current_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->m_p_pipeline_layout, 0, real_size_of_sets,
		p_sets, 1, &pDescriptorOffsets);

	if (this->m_is_use_stencil_pipeline)
	{
		vkCmdBindPipeline(this->m_p_current_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
			this->m_p_pipeline_stencil_for_region_where_geometry_will_be_drawn);
	}
	else
	{
		if (p_casted_compiled_geometry->m_is_has_texture)
		{
			if (this->m_is_apply_to_regular_geometry_stencil)
			{
				vkCmdBindPipeline(this->m_p_current_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
					this->m_p_pipeline_stencil_for_regular_geometry_that_applied_to_region_with_textures);
			}
			else
			{
				vkCmdBindPipeline(this->m_p_current_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->m_p_pipeline_with_textures);
			}
		}
		else
		{
			if (this->m_is_apply_to_regular_geometry_stencil)
			{
				vkCmdBindPipeline(this->m_p_current_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
					this->m_p_pipeline_stencil_for_regular_geometry_that_applied_to_region_without_textures);
			}
			else
			{
				vkCmdBindPipeline(this->m_p_current_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->m_p_pipeline_without_textures);
			}
		}
	}

	vkCmdBindVertexBuffers(this->m_p_current_command_buffer, 0, 1, &p_casted_compiled_geometry->m_p_vertex.buffer,
		&p_casted_compiled_geometry->m_p_vertex.offset);

	vkCmdBindIndexBuffer(this->m_p_current_command_buffer, p_casted_compiled_geometry->m_p_index.buffer, p_casted_compiled_geometry->m_p_index.offset,
		VK_INDEX_TYPE_UINT32);

	vkCmdDrawIndexed(this->m_p_current_command_buffer, p_casted_compiled_geometry->m_num_indices, 1, 0, 0, 0);

	if (p_casted_compiled_geometry->m_is_cached == false)
		p_casted_compiled_geometry->m_is_cached = true;
}

void RenderInterface_Vulkan::ReleaseCompiledGeometry(Rml::CompiledGeometryHandle geometry)
{
	RMLUI_ZoneScopedN("Vulkan - ReleaseCompiledGeometry");

	geometry_handle_t* p_casted_geometry = reinterpret_cast<geometry_handle_t*>(geometry);

	this->m_pending_for_deletion_geometries.push_back(p_casted_geometry);
}

void RenderInterface_Vulkan::EnableScissorRegion(bool enable)
{
	if (this->m_p_current_command_buffer == nullptr)
		return;

	if (this->m_is_transform_enabled)
	{
		this->m_is_apply_to_regular_geometry_stencil = true;
	}

	this->m_is_use_scissor_specified = enable;

	if (this->m_is_use_scissor_specified == false)
	{
		this->m_is_apply_to_regular_geometry_stencil = false;
		vkCmdSetScissor(this->m_p_current_command_buffer, 0, 1, &this->m_scissor_original);
	}
}

void RenderInterface_Vulkan::SetScissorRegion(int x, int y, int width, int height)
{
	if (this->m_is_use_scissor_specified)
	{
		if (this->m_is_transform_enabled)
		{
			float left = static_cast<float>(x);
			float right = static_cast<float>(x + width);
			float top = static_cast<float>(y);
			float bottom = static_cast<float>(y + height);

			Rml::Vertex vertices[4];

			vertices[0].position = {left, top};
			vertices[1].position = {right, top};
			vertices[2].position = {right, bottom};
			vertices[3].position = {left, bottom};

			int indices[6] = {0, 2, 1, 0, 3, 2};

			this->m_is_use_stencil_pipeline = true;

			this->RenderGeometry(vertices, 4, indices, 6, 0, Rml::Vector2f(0.0f, 0.0f));

			this->m_is_use_stencil_pipeline = false;

			this->m_is_apply_to_regular_geometry_stencil = true;
		}
		else
		{
			this->m_scissor.extent.width = width;
			this->m_scissor.extent.height = height;
			this->m_scissor.offset.x = static_cast<int32_t>(std::abs(x));
			this->m_scissor.offset.y = static_cast<int32_t>(std::abs(y));

			vkCmdSetScissor(this->m_p_current_command_buffer, 0, 1, &this->m_scissor);
		}
	}
}

// Set to byte packing, or the compiler will expand our struct, which means it won't read correctly from file
#pragma pack(1)
struct TGAHeader {
	char idLength;
	char colourMapType;
	char dataType;
	short int colourMapOrigin;
	short int colourMapLength;
	char colourMapDepth;
	short int xOrigin;
	short int yOrigin;
	short int width;
	short int height;
	char bitsPerPixel;
	char imageDescriptor;
};
// Restore packing
#pragma pack()

bool RenderInterface_Vulkan::LoadTexture(Rml::TextureHandle& texture_handle, Rml::Vector2i& texture_dimensions, const Rml::String& source)
{
	Rml::FileInterface* file_interface = Rml::GetFileInterface();
	Rml::FileHandle file_handle = file_interface->Open(source);
	if (!file_handle)
	{
		return false;
	}

	file_interface->Seek(file_handle, 0, SEEK_END);
	size_t buffer_size = file_interface->Tell(file_handle);
	file_interface->Seek(file_handle, 0, SEEK_SET);

	RMLUI_ASSERTMSG(buffer_size > sizeof(TGAHeader), "Texture file size is smaller than TGAHeader, file must be corrupt or otherwise invalid");
	if (buffer_size <= sizeof(TGAHeader))
	{
		file_interface->Close(file_handle);
		return false;
	}

	char* buffer = new char[buffer_size];
	file_interface->Read(buffer, buffer_size, file_handle);
	file_interface->Close(file_handle);

	TGAHeader header;
	memcpy(&header, buffer, sizeof(TGAHeader));

	int color_mode = header.bitsPerPixel / 8;
	int image_size = header.width * header.height * 4; // We always make 32bit textures

	if (header.dataType != 2)
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "Only 24/32bit uncompressed TGAs are supported.");
		return false;
	}

	// Ensure we have at least 3 colors
	if (color_mode < 3)
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "Only 24 and 32bit textures are supported");
		return false;
	}

	const char* image_src = buffer + sizeof(TGAHeader);
	unsigned char* image_dest = new unsigned char[image_size];

	// Targa is BGR, swap to RGB and flip Y axis
	for (long y = 0; y < header.height; y++)
	{
		long read_index = y * header.width * color_mode;
		long write_index = ((header.imageDescriptor & 32) != 0) ? read_index : (header.height - y - 1) * header.width * color_mode;
		for (long x = 0; x < header.width; x++)
		{
			image_dest[write_index] = image_src[read_index + 2];
			image_dest[write_index + 1] = image_src[read_index + 1];
			image_dest[write_index + 2] = image_src[read_index];
			if (color_mode == 4)
				image_dest[write_index + 3] = image_src[read_index + 3];
			else
				image_dest[write_index + 3] = 255;

			write_index += 4;
			read_index += color_mode;
		}
	}

	texture_dimensions.x = header.width;
	texture_dimensions.y = header.height;

	texture_handle = (Rml::TextureHandle)source.c_str();

	bool status = this->GenerateTexture(texture_handle, image_dest, texture_dimensions);

	delete[] image_dest;
	delete[] buffer;

	return status;
}

/*
    How vulkan works with textures efficiently?

    You need to create buffer that has CPU memory accessibility it means it uses your RAM memory for storing data and it has only CPU visibility (RAM)
    After you create buffer that has GPU memory accessibility it means it uses by your video hardware and it has only VRAM (Video RAM) visibility

    So you copy data to CPU_buffer and after you copy that thing to GPU_buffer, but delete CPU_buffer

    So it means you "uploaded" data to GPU

    Again, you need to "write" data into CPU buffer after you need to copy that data from buffer to GPU buffer and after that buffer go to GPU.

    RAW_POINTER_DATA_BYTES_LITERALLY->COPY_TO->CPU->COPY_TO->GPU->Releasing_CPU <= that's how works uploading textures in Vulkan if you want to have
   efficient handling otherwise it is cpu_to_gpu visibility and it means you create only ONE buffer that is accessible for CPU and for GPU, but it
   will cause the worst performance...
*/
bool RenderInterface_Vulkan::GenerateTexture(Rml::TextureHandle& texture_handle, const Rml::byte* source, const Rml::Vector2i& source_dimensions)
{
	RMLUI_ZoneScopedN("Vulkan - GenerateTexture");

	VK_ASSERT(source, "you pushed not valid data for copying to buffer");
	VK_ASSERT(this->m_p_allocator, "you have to initialize Vma Allocator for this method");

	int width = source_dimensions.x;
	int height = source_dimensions.y;

	VK_ASSERT(width, "invalid width");
	VK_ASSERT(height, "invalid height");

	const char* file_path = reinterpret_cast<const char*>(texture_handle);

	VkDeviceSize image_size = width * height * 4;
	VkFormat format = VkFormat::VK_FORMAT_R8G8B8A8_UNORM;

	auto cpu_buffer = this->CreateResource_StagingBuffer(image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

	void* data;
	vmaMapMemory(this->m_p_allocator, cpu_buffer.Get_VmaAllocation(), &data);
	memcpy(data, source, static_cast<size_t>(image_size));
	vmaUnmapMemory(this->m_p_allocator, cpu_buffer.Get_VmaAllocation());

	VkExtent3D extent_image = {};
	extent_image.width = static_cast<uint32_t>(width);
	extent_image.height = static_cast<uint32_t>(height);
	extent_image.depth = 1;

	auto* p_texture = new texture_data_t();

	VkImageCreateInfo info = {};

	info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	info.pNext = nullptr;
	info.imageType = VK_IMAGE_TYPE_2D;
	info.format = format;
	info.extent = extent_image;
	info.mipLevels = 1;
	info.arrayLayers = 1;
	info.samples = VK_SAMPLE_COUNT_1_BIT;
	info.tiling = VK_IMAGE_TILING_OPTIMAL;
	info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	VmaAllocationCreateInfo info_allocation = {};
	info_allocation.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	VkImage p_image = nullptr;
	VmaAllocation p_allocation = nullptr;

	VmaAllocationInfo info_stats = {};
	VkResult status = vmaCreateImage(this->m_p_allocator, &info, &info_allocation, &p_image, &p_allocation, &info_stats);
	VK_ASSERT(status == VkResult::VK_SUCCESS, "failed to vmaCreateImage");

#ifdef RMLUI_DEBUG
	Rml::Log::Message(Rml::Log::LT_DEBUG, "Created image %s with size [%zu bytes][%zu Megabytes]", file_path, info_stats.size,
		TranslateBytesToMegaBytes(info_stats.size));
#endif

	p_texture->Set_FileName(file_path);
	p_texture->Set_VkImage(p_image);
	p_texture->Set_VmaAllocation(p_allocation);

#ifdef RMLUI_DEBUG
	vmaSetAllocationName(this->m_p_allocator, p_allocation, file_path);
#endif

	p_texture->Set_Width(width);
	p_texture->Set_Height(height);

	/*
	 * So Vulkan works only through VkCommandBuffer, it is for remembering API commands what you want to call from GPU
	 * So on CPU side you need to create a scope that consists of two things
	 * vkBeginCommandBuffer
	 * ... <= here your commands what you want to place into your command buffer and send it to GPU through vkQueueSubmit function
	 * vkEndCommandBuffer
	 *
	 * So commands start to work ONLY when you called the vkQueueSubmit otherwise you just "place" commands into your command buffer but you
	 * didn't issue any thing in order to start the work on GPU side. ALWAYS remember that just sumbit means execute async mode, so you have to wait
	 * operations before they exeecute fully otherwise you will get some errors or write/read concurrent state and all other stuff, vulkan validation
	 * will notify you :) (in most cases)
	 *
	 * BUT you need always sync what you have done when you called your vkQueueSubmit function, so it is wait method, but generally you can create
	 * another queue and isolate all stuff tbh
	 *
	 * So understing these principles you understand how to work with API and your GPU
	 *
	 * There's nothing hard, but it makes all stuff on programmer side if you remember OpenGL and how it was easy to load texture upload it and create
	 * buffers and it In OpenGL all stuff is handled by driver and other things, not a programmer definitely
	 *
	 * What we do here? We need to change the layout of our image. it means where we want to use it. So in our case we want to see that this image
	 * will be in shaders Because the initial state of create object is VK_IMAGE_LAYOUT_UNDEFINED means you can't just pass that VkImage handle to
	 * your functions and wait that it comes to shaders for exmaple No it doesn't work like that you have to have the explicit states of your resource
	 * and where it goes
	 *
	 * In our case we want to see in our pixel shader so we need to change transfer into this flag VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, because we
	 * want to copy so it means some transfer thing, but after we say it goes to pixel after our copying operation
	 */
	{
		this->m_upload_manager.UploadToGPU([p_image, extent_image, cpu_buffer](VkCommandBuffer p_cmd) {
			VkImageSubresourceRange range = {};
			range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			range.baseMipLevel = 0;
			range.baseArrayLayer = 0;
			range.levelCount = 1;
			range.layerCount = 1;

			VkImageMemoryBarrier info_barrier = {};

			info_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			info_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			info_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			info_barrier.image = p_image;
			info_barrier.subresourceRange = range;
			info_barrier.srcAccessMask = 0;
			info_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			vkCmdPipelineBarrier(p_cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1,
				&info_barrier);

			VkBufferImageCopy region = {};
			region.bufferOffset = 0;
			region.bufferRowLength = 0;
			region.bufferImageHeight = 0;

			region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.imageSubresource.mipLevel = 0;
			region.imageSubresource.baseArrayLayer = 0;
			region.imageSubresource.layerCount = 1;
			region.imageExtent = extent_image;

			vkCmdCopyBufferToImage(p_cmd, cpu_buffer.Get_VkBuffer(), p_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

			VkImageMemoryBarrier info_barrier_shader_read = {};

			info_barrier_shader_read.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			info_barrier_shader_read.pNext = nullptr;
			info_barrier_shader_read.image = p_image;
			info_barrier_shader_read.subresourceRange = range;
			info_barrier_shader_read.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			info_barrier_shader_read.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			info_barrier_shader_read.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			info_barrier_shader_read.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(p_cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
				&info_barrier_shader_read);
		});
	}

	this->DestroyResource_StagingBuffer(cpu_buffer);

	VkImageViewCreateInfo info_image_view = {};

	info_image_view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	info_image_view.pNext = nullptr;
	info_image_view.image = p_texture->Get_VkImage();
	info_image_view.viewType = VK_IMAGE_VIEW_TYPE_2D;
	info_image_view.format = format;
	info_image_view.subresourceRange.baseMipLevel = 0;
	info_image_view.subresourceRange.levelCount = 1;
	info_image_view.subresourceRange.baseArrayLayer = 0;
	info_image_view.subresourceRange.layerCount = 1;
	info_image_view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

	VkImageView p_image_view = nullptr;
	status = vkCreateImageView(this->m_p_device, &info_image_view, nullptr, &p_image_view);
	VK_ASSERT(status == VkResult::VK_SUCCESS, "failed to vkCreateImageView");

	p_texture->Set_VkImageView(p_image_view);
	p_texture->Set_VkSampler(this->m_p_sampler_linear);

	texture_handle = reinterpret_cast<Rml::TextureHandle>(p_texture);

	return true;
}

void RenderInterface_Vulkan::ReleaseTexture(Rml::TextureHandle texture_handle)
{
	texture_data_t* p_texture = reinterpret_cast<texture_data_t*>(texture_handle);

	if (p_texture)
	{
		this->m_pending_for_deletion_textures_by_frames[this->m_semaphore_index_previous].push_back(p_texture);
	}
}

void RenderInterface_Vulkan::SetTransform(const Rml::Matrix4f* transform)
{
	this->m_is_transform_enabled = !!(transform);
	this->m_user_data_for_vertex_shader.m_transform = this->m_projection * (transform ? *transform : Rml::Matrix4f::Identity());
}

void RenderInterface_Vulkan::SetViewport(int viewport_width, int viewport_height)
{
	this->OnResize(viewport_width, viewport_height);
}

void RenderInterface_Vulkan::BeginFrame()
{
	this->m_command_list.OnBeginFrame();
	this->Wait();

	this->Update_PendingForDeletion_Textures_By_Frames();
	this->Update_PendingForDeletion_Geometries();

	this->m_p_current_command_buffer = this->m_command_list.GetNewCommandList();

	VkCommandBufferBeginInfo info = {};

	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	info.pInheritanceInfo = nullptr;
	info.pNext = nullptr;
	info.flags = VkCommandBufferUsageFlagBits::VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	auto status = vkBeginCommandBuffer(this->m_p_current_command_buffer, &info);

	VK_ASSERT(status == VkResult::VK_SUCCESS, "failed to vkBeginCommandBuffer");

	VkClearValue for_filling_back_buffer_color;
	VkClearValue for_stencil_depth;

	for_stencil_depth.depthStencil = {1.0f, 0};
	for_filling_back_buffer_color.color = {{0.0f, 0.0f, 0.0f, 1.0f}};

	const VkClearValue p_color_rt[] = {for_filling_back_buffer_color, for_stencil_depth};

	VkRenderPassBeginInfo info_pass = {};

	info_pass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	info_pass.pNext = nullptr;
	info_pass.renderPass = this->m_p_render_pass;
	info_pass.framebuffer = this->m_swapchain_frame_buffers.at(this->m_image_index);
	info_pass.pClearValues = p_color_rt;
	info_pass.clearValueCount = 2;
	info_pass.renderArea.offset.x = 0;
	info_pass.renderArea.offset.y = 0;
	info_pass.renderArea.extent.width = this->m_width;
	info_pass.renderArea.extent.height = this->m_height;

	vkCmdBeginRenderPass(this->m_p_current_command_buffer, &info_pass, VkSubpassContents::VK_SUBPASS_CONTENTS_INLINE);
	vkCmdSetViewport(this->m_p_current_command_buffer, 0, 1, &this->m_viewport);

	this->m_is_apply_to_regular_geometry_stencil = false;
}

void RenderInterface_Vulkan::EndFrame()
{
	if (this->m_p_current_command_buffer == nullptr)
		return;

	vkCmdEndRenderPass(this->m_p_current_command_buffer);

	auto status = vkEndCommandBuffer(this->m_p_current_command_buffer);

	VK_ASSERT(status == VkResult::VK_SUCCESS, "failed to vkEndCommandBuffer");

	this->Submit();
	this->Present();

	this->m_p_current_command_buffer = nullptr;
}

bool RenderInterface_Vulkan::Initialize(CreateSurfaceCallback create_surface_callback)
{
	RMLUI_ZoneScopedN("Vulkan - Initialize");

	int glad_result = 0;
	glad_result = gladLoaderLoadVulkan(VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE);
	VK_ASSERT(glad_result != 0, "Vulkan loader failed - Global functions");

	this->Initialize_Instance();
	this->Initialize_PhysicalDevice();

	glad_result = gladLoaderLoadVulkan(m_p_instance, m_p_physical_device_current, VK_NULL_HANDLE);
	VK_ASSERT(glad_result != 0, "Vulkan loader failed - Instance functions");

	this->Initialize_Surface(create_surface_callback);
	this->Initialize_QueueIndecies();
	this->Initialize_Device();

	glad_result = gladLoaderLoadVulkan(m_p_instance, m_p_physical_device_current, m_p_device);
	VK_ASSERT(glad_result != 0, "Vulkan loader failed - Device functions");

	this->Initialize_Queues();
	this->Initialize_SyncPrimitives();
	this->Initialize_Allocator();
	this->Initialize_Resources();

	return true;
}

void RenderInterface_Vulkan::Shutdown()
{
	RMLUI_ZoneScopedN("Vulkan - Shutdown");

	auto status = vkDeviceWaitIdle(this->m_p_device);

	VK_ASSERT(status == VkResult::VK_SUCCESS, "you must have a valid status here");

	this->DestroyResourcesDependentOnSize();
	this->Destroy_Resources();
	this->Destroy_Allocator();
	this->Destroy_SyncPrimitives();
	this->Destroy_Swapchain();
	this->Destroy_Surface();
	this->Destroy_Device();
	this->Destroy_ReportDebugCallback();
	this->Destroy_Instance();

	gladLoaderUnloadVulkan();
}

void RenderInterface_Vulkan::OnWindowMinimize(void)
{
	this->m_is_can_render = false;
}

void RenderInterface_Vulkan::OnWindowRestored(void)
{
	// needs to resize when the driver obtained the extent for swapchain it is not right to pass this->m_width & this->m_height, because they could be
	// invalid and can be different with extent, of course it supposed that those m_width and m_height must be the same as extent, but can't be sure
	// that it will be always. Maybe window manager of OS will alter something, or driver will act strangely, so it is better to pass what driver
	// wants otherwise it is just not right.
	auto extent = this->Choose_ValidSwapchainExtent();

#ifdef RMLUI_DEBUG
	VK_ASSERT(extent.width == static_cast<uint32_t>(this->m_width),
		"So logically we just restored the window and we didn't resize at all, possible something is wrong, but just ignore this");
	VK_ASSERT(extent.height == static_cast<uint32_t>(this->m_height),
		"So logically we just restored the window and we didn't resize at all, possible something is wrong, but just ignore this");
#endif

	this->SetViewport(extent.width, extent.height);

	this->m_is_can_render = true;
}

void RenderInterface_Vulkan::OnWindowMaximize(void)
{
	// nothing but well why not...
	// when we maximize resizing is commited and it means it goes through regular pipeline when we just resize the window
	// so it means we don't need to handle something here, but if the user wants to alter or implement something we provide a such callbcack
}

void RenderInterface_Vulkan::OnWindowHidden(void)
{
	this->m_is_can_render = false;
}

void RenderInterface_Vulkan::OnWindowShown(void)
{
	this->m_is_can_render = true;
}

void RenderInterface_Vulkan::OnResize(int width, int height) noexcept
{
	auto status = vkDeviceWaitIdle(this->m_p_device);

	VK_ASSERT(status == VkResult::VK_SUCCESS, "failed to vkDeviceWaitIdle");

	this->m_width = width;
	this->m_height = height;

	auto extent = this->Choose_ValidSwapchainExtent();

	if (extent.width == 0 || extent.height == 0)
	{}
	else
	{}

	if (this->m_p_swapchain)
	{
		this->Destroy_Swapchain();
		this->DestroyResourcesDependentOnSize();
	}

	this->Initialize_Swapchain();
	this->CreateResourcesDependentOnSize();
}

void RenderInterface_Vulkan::Initialize_Instance(void) noexcept
{
	uint32_t required_version = this->GetRequiredVersionAndValidateMachine();

	VkApplicationInfo info = {};

	info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	info.pNext = nullptr;
	info.pApplicationName = "RMLUI Shell";
	info.applicationVersion = 42;
	info.pEngineName = "RmlUi";
	info.apiVersion = required_version;

	Rml::Vector<const char*> instance_layer_names;
	Rml::Vector<const char*> instance_extension_names;
	this->CreatePropertiesFor_Instance(instance_layer_names, instance_extension_names);

	VkInstanceCreateInfo info_instance = {};

	info_instance.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	info_instance.pNext = &debug_validation_features_ext;
	info_instance.flags = 0;
	info_instance.pApplicationInfo = &info;
	info_instance.enabledExtensionCount = static_cast<uint32_t>(instance_extension_names.size());
	info_instance.ppEnabledExtensionNames = instance_extension_names.data();
	info_instance.enabledLayerCount = static_cast<uint32_t>(instance_layer_names.size());
	info_instance.ppEnabledLayerNames = instance_layer_names.data();

	VkResult status = vkCreateInstance(&info_instance, nullptr, &this->m_p_instance);

	VK_ASSERT(status == VK_SUCCESS, "failed to vkCreateInstance");

	this->CreateReportDebugCallback();
}

void RenderInterface_Vulkan::Initialize_Device(void) noexcept
{
	Rml::Vector<VkExtensionProperties> device_extension_properties;
	this->CreatePropertiesFor_Device(device_extension_properties);

	Rml::Vector<const char*> device_extension_names;
	this->AddExtensionToDevice(device_extension_names, device_extension_properties, VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	this->AddExtensionToDevice(device_extension_names, device_extension_properties, VK_EXT_SCALAR_BLOCK_LAYOUT_EXTENSION_NAME);

	float queue_priorities[1] = {0.0f};

	VkDeviceQueueCreateInfo info_queue[2] = {};

	info_queue[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	info_queue[0].pNext = nullptr;
	info_queue[0].queueCount = 1;
	info_queue[0].pQueuePriorities = queue_priorities;
	info_queue[0].queueFamilyIndex = this->m_queue_index_graphics;

	info_queue[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	info_queue[1].pNext = nullptr;
	info_queue[1].queueCount = 1;
	info_queue[1].pQueuePriorities = queue_priorities;
	info_queue[1].queueFamilyIndex = this->m_queue_index_compute;

	VkPhysicalDeviceFeatures features_physical_device = {};

	features_physical_device.fillModeNonSolid = true;
	features_physical_device.pipelineStatisticsQuery = true;
	features_physical_device.fragmentStoresAndAtomics = true;
	features_physical_device.vertexPipelineStoresAndAtomics = true;
	features_physical_device.shaderImageGatherExtended = true;
	features_physical_device.wideLines = true;

	VkPhysicalDeviceShaderSubgroupExtendedTypesFeaturesKHR shader_subgroup_extended_type = {};

	shader_subgroup_extended_type.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SUBGROUP_EXTENDED_TYPES_FEATURES_KHR;
	shader_subgroup_extended_type.pNext = nullptr;
	shader_subgroup_extended_type.shaderSubgroupExtendedTypes = VK_TRUE;

	VkPhysicalDeviceFeatures2 features_physical_device2 = {};

	features_physical_device2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	features_physical_device2.features = features_physical_device;
	features_physical_device2.pNext = &shader_subgroup_extended_type;

	VkDeviceCreateInfo info_device = {};

	info_device.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	info_device.pNext = &features_physical_device2;
	info_device.queueCreateInfoCount = 2;
	info_device.pQueueCreateInfos = info_queue;
	info_device.enabledExtensionCount = static_cast<uint32_t>(device_extension_names.size());
	info_device.ppEnabledExtensionNames = info_device.enabledExtensionCount ? device_extension_names.data() : nullptr;
	info_device.pEnabledFeatures = nullptr;

	VkResult status = vkCreateDevice(this->m_p_physical_device_current, &info_device, nullptr, &this->m_p_device);

	VK_ASSERT(status == VK_SUCCESS, "failed to vkCreateDevice");
}

void RenderInterface_Vulkan::Initialize_PhysicalDevice(void) noexcept
{
	this->CollectPhysicalDevices();
	bool status = this->ChoosePhysicalDevice(VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);

	if (status == false)
	{
		Rml::Log::Message(Rml::Log::LT_WARNING, "Failed to pick the discrete gpu, now trying to pick integrated GPU");
		status = this->ChoosePhysicalDevice(VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU);

		if (status == false)
		{
			Rml::Log::Message(Rml::Log::LT_WARNING, "Failed to pick the integrated gpu, now trying to pick up the CPU");
			status = this->ChoosePhysicalDevice(VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_CPU);

			VK_ASSERT(status, "there's no suitable physical device for rendering, abort this application");
		}
	}

	this->PrintInformationAboutPickedPhysicalDevice(this->m_p_physical_device_current);

	vkGetPhysicalDeviceProperties(this->m_p_physical_device_current, &this->m_current_physical_device_properties);
}

void RenderInterface_Vulkan::Initialize_Swapchain(void) noexcept
{
	VkSwapchainCreateInfoKHR info = {};

	this->m_swapchain_format = this->ChooseSwapchainFormat();

	info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	info.pNext = nullptr;
	info.surface = this->m_p_surface;
	info.imageFormat = this->m_swapchain_format.format;
	info.minImageCount = this->Choose_SwapchainImageCount();
	info.imageColorSpace = this->m_swapchain_format.colorSpace;
	info.imageExtent = this->Choose_ValidSwapchainExtent();
	info.preTransform = this->CreatePretransformSwapchain();
	info.compositeAlpha = this->ChooseSwapchainCompositeAlpha();
	info.imageArrayLayers = 1;
	info.presentMode = this->GetPresentMode();
	info.oldSwapchain = nullptr;
	info.clipped = true;
	info.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	info.queueFamilyIndexCount = 0;
	info.pQueueFamilyIndices = nullptr;

	uint32_t queue_family_index_present = this->m_queue_index_present;
	uint32_t queue_family_index_graphics = this->m_queue_index_graphics;

	if (queue_family_index_graphics != queue_family_index_present)
	{
		uint32_t p_indecies[2] = {queue_family_index_graphics, queue_family_index_present};

		info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		info.queueFamilyIndexCount = sizeof(p_indecies) / sizeof(p_indecies[0]);
		info.pQueueFamilyIndices = p_indecies;
	}

	VkResult status = vkCreateSwapchainKHR(this->m_p_device, &info, nullptr, &this->m_p_swapchain);

	VK_ASSERT(status == VK_SUCCESS, "failed to vkCreateSwapchainKHR");
}

void RenderInterface_Vulkan::Initialize_Surface(CreateSurfaceCallback create_surface_callback) noexcept
{
	VK_ASSERT(this->m_p_instance, "you must initialize your VkInstance");

	bool result = create_surface_callback(this->m_p_instance, &this->m_p_surface);
	VK_ASSERT(result && this->m_p_surface, "failed to vkCreateWin32SurfaceKHR");
}

void RenderInterface_Vulkan::Initialize_QueueIndecies(void) noexcept
{
	VK_ASSERT(this->m_p_physical_device_current, "you must initialize your physical device");
	VK_ASSERT(this->m_p_surface, "you must initialize VkSurfaceKHR before calling this method");

	uint32_t queue_family_count = 0;

	vkGetPhysicalDeviceQueueFamilyProperties(this->m_p_physical_device_current, &queue_family_count, nullptr);

	VK_ASSERT(queue_family_count >= 1, "failed to vkGetPhysicalDeviceQueueFamilyProperties (getting count)");

	Rml::Vector<VkQueueFamilyProperties> queue_props;
	queue_props.resize(queue_family_count);

	vkGetPhysicalDeviceQueueFamilyProperties(this->m_p_physical_device_current, &queue_family_count, queue_props.data());

	VK_ASSERT(queue_family_count >= 1, "failed to vkGetPhysicalDeviceQueueFamilyProperties (filling vector of VkQueueFamilyProperties)");

	constexpr uint32_t kUint32Undefined = uint32_t(-1);

	this->m_queue_index_compute = kUint32Undefined;
	this->m_queue_index_graphics = kUint32Undefined;
	this->m_queue_index_present = kUint32Undefined;

	for (uint32_t i = 0; i < queue_family_count; ++i)
	{
		if ((queue_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
		{
			if (this->m_queue_index_graphics == kUint32Undefined)
				this->m_queue_index_graphics = i;

			VkBool32 is_support_present;

			vkGetPhysicalDeviceSurfaceSupportKHR(this->m_p_physical_device_current, i, this->m_p_surface, &is_support_present);

			// User's videocard may have same index for two queues like graphics and present

			if (is_support_present == VK_TRUE)
			{
				this->m_queue_index_graphics = i;
				this->m_queue_index_present = this->m_queue_index_graphics;
				break;
			}
		}
	}

	if (this->m_queue_index_present == static_cast<uint32_t>(-1))
	{
		Rml::Log::Message(Rml::Log::LT_WARNING, "[Vulkan] User doesn't have one index for two queues, so we need to find for present queue index");

		for (uint32_t i = 0; i < queue_family_count; ++i)
		{
			VkBool32 is_support_present;

			vkGetPhysicalDeviceSurfaceSupportKHR(this->m_p_physical_device_current, i, this->m_p_surface, &is_support_present);

			if (is_support_present == VK_TRUE)
			{
				this->m_queue_index_present = i;
				break;
			}
		}
	}

	for (uint32_t i = 0; i < queue_family_count; ++i)
	{
		if ((queue_props[i].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0)
		{
			if (this->m_queue_index_compute == kUint32Undefined)
				this->m_queue_index_compute = i;

			if (i != this->m_queue_index_graphics)
			{
				this->m_queue_index_compute = i;
				break;
			}
		}
	}

#ifdef RMLUI_DEBUG
	Rml::Log::Message(Rml::Log::LT_DEBUG, "[Vulkan] User family queues indecies: Graphics[%d] Present[%d] Compute[%d]", this->m_queue_index_graphics,
		this->m_queue_index_present, this->m_queue_index_compute);
#endif
}

void RenderInterface_Vulkan::Initialize_Queues(void) noexcept
{
	VK_ASSERT(this->m_p_device, "you must initialize VkDevice before using this method");

	vkGetDeviceQueue(this->m_p_device, this->m_queue_index_graphics, 0, &this->m_p_queue_graphics);

	if (this->m_queue_index_graphics == this->m_queue_index_present)
	{
		this->m_p_queue_present = this->m_p_queue_graphics;
	}
	else
	{
		vkGetDeviceQueue(this->m_p_device, this->m_queue_index_present, 0, &this->m_p_queue_present);
	}

	constexpr uint32_t kUint32Undefined = uint32_t(-1);

	if (this->m_queue_index_compute != kUint32Undefined)
	{
		vkGetDeviceQueue(this->m_p_device, this->m_queue_index_compute, 0, &this->m_p_queue_compute);
	}
}

void RenderInterface_Vulkan::Initialize_SyncPrimitives(void) noexcept
{
	VK_ASSERT(this->m_p_device, "you must initialize your device");

	this->m_executed_fences.resize(kSwapchainBackBufferCount);
	this->m_semaphores_finished_render.resize(kSwapchainBackBufferCount);
	this->m_semaphores_image_available.resize(kSwapchainBackBufferCount);

	VkResult status = VK_SUCCESS;

	for (uint32_t i = 0; i < kSwapchainBackBufferCount; ++i)
	{
		VkFenceCreateInfo info_fence = {};

		info_fence.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		info_fence.pNext = nullptr;
		info_fence.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		status = vkCreateFence(this->m_p_device, &info_fence, nullptr, &this->m_executed_fences[i]);

		VK_ASSERT(status == VK_SUCCESS, "failed to vkCreateFence");

		VkSemaphoreCreateInfo info_semaphore = {};

		info_semaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		info_semaphore.pNext = nullptr;
		info_semaphore.flags = 0;

		status = vkCreateSemaphore(this->m_p_device, &info_semaphore, nullptr, &this->m_semaphores_image_available[i]);

		VK_ASSERT(status == VK_SUCCESS, "failed to vkCreateSemaphore");

		status = vkCreateSemaphore(this->m_p_device, &info_semaphore, nullptr, &this->m_semaphores_finished_render[i]);

		VK_ASSERT(status == VK_SUCCESS, "failed to vkCreateSemaphore");
	}
}

void RenderInterface_Vulkan::Initialize_Resources(void) noexcept
{
	this->m_command_list.Initialize(this->m_p_device, this->m_queue_index_graphics, kSwapchainBackBufferCount, 2);

	MemoryPoolCreateInfo info = {};
	info.m_number_of_back_buffers = kSwapchainBackBufferCount;
	info.m_gpu_data_count = 100;
	info.m_gpu_data_size = sizeof(shader_vertex_user_data_t);
	info.m_memory_total_size = RenderInterface_Vulkan::ConvertMegabytesToBytes(static_cast<VkDeviceSize>(kVideoMemoryForAllocation));

	this->m_memory_pool.Initialize(&this->m_current_physical_device_properties, this->m_p_allocator, this->m_p_device, info);

	this->m_upload_manager.Initialize(this->m_p_device, this->m_p_queue_graphics, this->m_queue_index_graphics);
	this->m_manager_descriptors.Initialize(this->m_p_device, 100, 100, 10, 10);

	auto storage = this->LoadShaders();

	this->CreateShaders(storage);
	this->CreateDescriptorSetLayout(storage);
	this->CreatePipelineLayout();
	this->CreateSamplers();
	this->CreateDescriptorSets();

	this->m_pending_for_deletion_textures_by_frames.resize(kSwapchainBackBufferCount);
}

void RenderInterface_Vulkan::Initialize_Allocator(void) noexcept
{
	VK_ASSERT(this->m_p_device, "you must have a valid VkDevice here");
	VK_ASSERT(this->m_p_physical_device_current, "you must have a valid VkPhysicalDevice here");
	VK_ASSERT(this->m_p_instance, "you must have a valid VkInstance here");

	VmaVulkanFunctions vulkanFunctions = {};
	vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
	vulkanFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

	VmaAllocatorCreateInfo info = {};

	info.vulkanApiVersion = VK_API_VERSION_1_0;
	info.device = this->m_p_device;
	info.instance = this->m_p_instance;
	info.physicalDevice = this->m_p_physical_device_current;
	info.pVulkanFunctions = &vulkanFunctions;

	auto status = vmaCreateAllocator(&info, &this->m_p_allocator);

	VK_ASSERT(status == VkResult::VK_SUCCESS, "failed to vmaCreateAllocator");
}

void RenderInterface_Vulkan::Destroy_Instance(void) noexcept
{
	vkDestroyInstance(this->m_p_instance, nullptr);
}

void RenderInterface_Vulkan::Destroy_Device() noexcept
{
	vkDestroyDevice(this->m_p_device, nullptr);
}

void RenderInterface_Vulkan::Destroy_Swapchain(void) noexcept
{
	VK_ASSERT(this->m_p_device, "you must initialize device");

	vkDestroySwapchainKHR(this->m_p_device, this->m_p_swapchain, nullptr);
}

void RenderInterface_Vulkan::Destroy_Surface(void) noexcept
{
	vkDestroySurfaceKHR(this->m_p_instance, this->m_p_surface, nullptr);
}

void RenderInterface_Vulkan::Destroy_SyncPrimitives(void) noexcept
{
	for (auto& p_fence : this->m_executed_fences)
	{
		vkDestroyFence(this->m_p_device, p_fence, nullptr);
	}

	for (auto& p_semaphore : this->m_semaphores_image_available)
	{
		vkDestroySemaphore(this->m_p_device, p_semaphore, nullptr);
	}

	for (auto& p_semaphore : this->m_semaphores_finished_render)
	{
		vkDestroySemaphore(this->m_p_device, p_semaphore, nullptr);
	}
}

void RenderInterface_Vulkan::Destroy_Resources(void) noexcept
{
	this->m_command_list.Shutdown();
	this->m_upload_manager.Shutdown();

	if (this->m_p_descriptor_set)
	{
		this->m_manager_descriptors.Free_Descriptors(this->m_p_device, &this->m_p_descriptor_set);
	}

	vkDestroyDescriptorSetLayout(this->m_p_device, this->m_p_descriptor_set_layout_uniform_buffer_dynamic, nullptr);
	vkDestroyDescriptorSetLayout(this->m_p_device, this->m_p_descriptor_set_layout_for_textures, nullptr);

	vkDestroyPipelineLayout(this->m_p_device, this->m_p_pipeline_layout, nullptr);

	for (const auto& p_module : this->m_shaders)
	{
		vkDestroyShaderModule(this->m_p_device, p_module, nullptr);
	}

	this->DestroySamplers();
	this->Destroy_Textures();
	this->Destroy_Geometries();

	this->m_manager_descriptors.Shutdown(this->m_p_device);
}

void RenderInterface_Vulkan::Destroy_Allocator(void) noexcept
{
	VK_ASSERT(this->m_p_allocator, "you must have an initialized allocator for deleting");

	vmaDestroyAllocator(this->m_p_allocator);

	this->m_p_allocator = nullptr;
}

void RenderInterface_Vulkan::QueryInstanceLayers(Rml::Vector<VkLayerProperties>& result) noexcept
{
	uint32_t instance_layer_properties_count = 0;

	VkResult status = vkEnumerateInstanceLayerProperties(&instance_layer_properties_count, nullptr);

	VK_ASSERT(status == VK_SUCCESS, "failed to vkEnumerateInstanceLayerProperties (getting count)");

	if (instance_layer_properties_count)
	{
		result.resize(instance_layer_properties_count);

		status = vkEnumerateInstanceLayerProperties(&instance_layer_properties_count, result.data());

		VK_ASSERT(status == VK_SUCCESS, "failed to vkEnumerateInstanceLayerProperties (filling vector of VkLayerProperties)");
	}
}

void RenderInterface_Vulkan::QueryInstanceExtensions(Rml::Vector<VkExtensionProperties>& result,
	const Rml::Vector<VkLayerProperties>& instance_layer_properties) noexcept
{
	uint32_t instance_extension_property_count = 0;

	VkResult status = vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_property_count, nullptr);

	VK_ASSERT(status == VK_SUCCESS, "failed to vkEnumerateInstanceExtensionProperties (getting count)");

	if (instance_extension_property_count)
	{
		result.resize(instance_extension_property_count);
		status = vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_property_count, result.data());

		VK_ASSERT(status == VK_SUCCESS, "failed to vkEnumerateInstanceExtensionProperties (filling vector of VkExtensionProperties)");
	}

	uint32_t count = 0;

	// without first argument in vkEnumerateInstanceExtensionProperties
	// it doesn't collect information well so we need brute-force
	// and pass through everything what use has
	for (const auto& layer_property : instance_layer_properties)
	{
		status = vkEnumerateInstanceExtensionProperties(layer_property.layerName, &count, nullptr);

		if (status == VK_SUCCESS)
		{
			if (count)
			{
				Rml::Vector<VkExtensionProperties> props;

				props.resize(count);

				status = vkEnumerateInstanceExtensionProperties(layer_property.layerName, &count, props.data());

				if (status == VK_SUCCESS)
				{
#ifdef RMLUI_DEBUG
					Rml::Log::Message(Rml::Log::LT_DEBUG, "[Vulkan] obtained extensions for layer: %s, count: %zu", layer_property.layerName,
						props.size());
#endif

					for (const auto& extension : props)
					{
						if (this->IsExtensionPresent(result, extension.extensionName) == false)
						{
#ifdef RMLUI_DEBUG
							Rml::Log::Message(Rml::Log::LT_DEBUG, "[Vulkan] new extension is added: %s", extension.extensionName);
#endif

							result.push_back(extension);
						}
					}
				}
			}
		}
	}
}

bool RenderInterface_Vulkan::AddLayerToInstance(Rml::Vector<const char*>& result, const Rml::Vector<VkLayerProperties>& instance_layer_properties,
	const char* p_instance_layer_name) noexcept
{
	if (p_instance_layer_name == nullptr)
	{
		VK_ASSERT(p_instance_layer_name, "you have an invalid layer");
		return false;
	}

	if (this->IsLayerPresent(instance_layer_properties, p_instance_layer_name))
	{
		result.push_back(p_instance_layer_name);
		return true;
	}

	Rml::Log::Message(Rml::Log::LT_WARNING, "[Vulkan] can't add layer %s", p_instance_layer_name);

	return false;
}

bool RenderInterface_Vulkan::AddExtensionToInstance(Rml::Vector<const char*>& result,
	const Rml::Vector<VkExtensionProperties>& instance_extension_properties, const char* p_instance_extension_name) noexcept
{
	if (p_instance_extension_name == nullptr)
	{
		VK_ASSERT(p_instance_extension_name, "you have an invalid extension");
		return false;
	}

	if (this->IsExtensionPresent(instance_extension_properties, p_instance_extension_name))
	{
		result.push_back(p_instance_extension_name);
		return true;
	}

	Rml::Log::Message(Rml::Log::LT_WARNING, "[Vulkan] can't add extension %s", p_instance_extension_name);

	return false;
}

void RenderInterface_Vulkan::CreatePropertiesFor_Instance(Rml::Vector<const char*>& instance_layer_names,
	Rml::Vector<const char*>& instance_extension_names) noexcept
{
	Rml::Vector<VkExtensionProperties> instance_extension_properties;
	Rml::Vector<VkLayerProperties> instance_layer_properties;

	this->QueryInstanceLayers(instance_layer_properties);
	this->QueryInstanceExtensions(instance_extension_properties, instance_layer_properties);

	this->AddLayerToInstance(instance_layer_names, instance_layer_properties, "VK_LAYER_LUNARG_monitor");

	this->AddExtensionToInstance(instance_extension_names, instance_extension_properties, "VK_EXT_debug_utils");
	this->AddExtensionToInstance(instance_extension_names, instance_extension_properties, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

#if defined(RMLUI_PLATFORM_UNIX)
	// TODO: add x11 headers for linux system this->AddExtensionToInstance(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#elif defined(RMLUI_PLATFORM_WIN32)
	this->AddExtensionToInstance(instance_extension_names, instance_extension_properties, VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif

	this->AddExtensionToInstance(instance_extension_names, instance_extension_properties, VK_KHR_SURFACE_EXTENSION_NAME);

#ifdef RMLUI_DEBUG
	bool is_cpu_validation = this->AddLayerToInstance(instance_layer_names, instance_layer_properties, "VK_LAYER_KHRONOS_validation") &&
		this->AddExtensionToInstance(instance_extension_names, instance_extension_properties, VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

	if (is_cpu_validation)
	{
		Rml::Log::Message(Rml::Log::LT_DEBUG, "[Vulkan] CPU validation is enabled");

		Rml::Array<const char*, 1> requested_extensions_for_gpu = {VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME};

		for (const auto& extension_name : requested_extensions_for_gpu)
		{
			this->AddExtensionToInstance(instance_extension_names, instance_extension_properties, extension_name);
		}

		debug_validation_features_ext.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
		debug_validation_features_ext.pNext = nullptr;
		debug_validation_features_ext.enabledValidationFeatureCount =
			sizeof(debug_validation_features_ext_requested) / sizeof(debug_validation_features_ext_requested[0]);
		debug_validation_features_ext.pEnabledValidationFeatures = debug_validation_features_ext_requested;
	}
#endif
}

bool RenderInterface_Vulkan::IsLayerPresent(const Rml::Vector<VkLayerProperties>& properties, const char* p_layer_name) noexcept
{
	if (properties.empty())
		return false;

	if (p_layer_name == nullptr)
		return false;

	return std::find_if(properties.cbegin(), properties.cend(),
			   [p_layer_name](const VkLayerProperties& prop) -> bool { return strcmp(prop.layerName, p_layer_name) == 0; }) != properties.cend();
}

bool RenderInterface_Vulkan::IsExtensionPresent(const Rml::Vector<VkExtensionProperties>& properties, const char* p_extension_name) noexcept
{
	if (properties.empty())
		return false;

	if (p_extension_name == nullptr)
		return false;

	return std::find_if(properties.cbegin(), properties.cend(), [p_extension_name](const VkExtensionProperties& prop) -> bool {
		return strcmp(prop.extensionName, p_extension_name) == 0;
	}) != properties.cend();
}

bool RenderInterface_Vulkan::AddExtensionToDevice(Rml::Vector<const char*>& result,
	const Rml::Vector<VkExtensionProperties>& device_extension_properties, const char* p_device_extension_name) noexcept
{
	if (this->IsExtensionPresent(device_extension_properties, p_device_extension_name))
	{
		result.push_back(p_device_extension_name);
		return true;
	}

	return false;
}

void RenderInterface_Vulkan::CreatePropertiesFor_Device(Rml::Vector<VkExtensionProperties>& result) noexcept
{
	VK_ASSERT(this->m_p_physical_device_current, "you must initialize your physical device. Call InitializePhysicalDevice first");

	uint32_t extension_count = 0;

	VkResult status = vkEnumerateDeviceExtensionProperties(this->m_p_physical_device_current, nullptr, &extension_count, nullptr);

	VK_ASSERT(status == VK_SUCCESS, "failed to vkEnumerateDeviceExtensionProperties (getting count)");

	result.resize(extension_count);

	status = vkEnumerateDeviceExtensionProperties(this->m_p_physical_device_current, nullptr, &extension_count, result.data());

	VK_ASSERT(status == VK_SUCCESS, "failed to vkEnumerateDeviceExtensionProperties (filling vector of VkExtensionProperties)");

	uint32_t instance_layer_property_count = 0;

	Rml::Vector<VkLayerProperties> layers;

	status = vkEnumerateInstanceLayerProperties(&instance_layer_property_count, nullptr);

	VK_ASSERT(status == VK_SUCCESS, "failed to vkEnumerateInstanceLayerProperties (getting count)");

	layers.resize(instance_layer_property_count);

	// On different OS Vulkan acts strange, so we can't get our extensions to just iterate through default functions
	// We need to deeply analyze our layers and get specified extensions which pass user
	// So we collect all extensions that are presented in physical device
	// And add when they exist to extension_names so we don't pass properties

	if (instance_layer_property_count)
	{
		status = vkEnumerateInstanceLayerProperties(&instance_layer_property_count, layers.data());

		VK_ASSERT(status == VK_SUCCESS, "failed to vkEnumerateInstanceLayerProperties (filling vector of VkLayerProperties)");

		for (const auto& layer : layers)
		{
			extension_count = 0;

			status = vkEnumerateDeviceExtensionProperties(this->m_p_physical_device_current, layer.layerName, &extension_count, nullptr);

			VK_ASSERT(status == VK_SUCCESS, "failed to vkEnumerateDeviceExtensionProperties (getting count)");

			if (extension_count)
			{
				Rml::Vector<VkExtensionProperties> new_extensions;

				new_extensions.resize(extension_count);

				status =
					vkEnumerateDeviceExtensionProperties(this->m_p_physical_device_current, layer.layerName, &extension_count, new_extensions.data());

				VK_ASSERT(status == VK_SUCCESS, "failed to vkEnumerateDeviceExtensionProperties (filling vector of VkExtensionProperties)");

				for (const auto& extension : new_extensions)
				{
					if (this->IsExtensionPresent(result, extension.extensionName) == false)
					{
#ifdef RMLUI_DEBUG
						Rml::Log::Message(Rml::Log::LT_DEBUG, "[Vulkan] obtained new device extension from layer[%s]: %s", layer.layerName,
							extension.extensionName);
#endif

						result.push_back(extension);
					}
				}
			}
		}
	}
}

void RenderInterface_Vulkan::CreateReportDebugCallback(void) noexcept
{
#ifdef RMLUI_DEBUG
	VkDebugReportCallbackCreateInfoEXT info = {};

	info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	info.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
	info.pfnCallback = MyDebugReportCallback;

	PFN_vkCreateDebugReportCallbackEXT p_callback_creation = VK_NULL_HANDLE;

	p_callback_creation =
		reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(vkGetInstanceProcAddr(this->m_p_instance, "vkCreateDebugReportCallbackEXT"));

	VkResult status = p_callback_creation(this->m_p_instance, &info, nullptr, &this->m_debug_report_callback_instance);

	VK_ASSERT(status == VK_SUCCESS, "failed to vkCreateDebugReportCallbackEXT");
#endif
}

void RenderInterface_Vulkan::Destroy_ReportDebugCallback(void) noexcept
{
#ifdef RMLUI_DEBUG
	PFN_vkDestroyDebugReportCallbackEXT p_destroy_callback =
		reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(vkGetInstanceProcAddr(this->m_p_instance, "vkDestroyDebugReportCallbackEXT"));

	if (p_destroy_callback)
	{
		p_destroy_callback(this->m_p_instance, this->m_debug_report_callback_instance, nullptr);
	}
#endif
}

uint32_t RenderInterface_Vulkan::GetUserAPIVersion(void) const noexcept
{
	uint32_t result = VK_API_VERSION_1_0;

#if defined VK_VERSION_1_1
	VkResult status = vkEnumerateInstanceVersion(&result);
	VK_ASSERT(status == VK_SUCCESS, "failed to vkEnumerateInstanceVersion, See Status");
#endif

	return result;
}

uint32_t RenderInterface_Vulkan::GetRequiredVersionAndValidateMachine(void) noexcept
{
	constexpr uint32_t kRequiredVersion = VK_API_VERSION_1_0;
	const uint32_t user_version = this->GetUserAPIVersion();

	VK_ASSERT(kRequiredVersion <= user_version, "Your machine doesn't support Vulkan");

	return kRequiredVersion;
}

void RenderInterface_Vulkan::CollectPhysicalDevices(void) noexcept
{
	uint32_t gpu_count = 1;
	Rml::Vector<VkPhysicalDevice> temp_devices;

	VkResult status = vkEnumeratePhysicalDevices(this->m_p_instance, &gpu_count, nullptr);

	VK_ASSERT(status == VK_SUCCESS, "failed to vkEnumeratePhysicalDevices (getting count)");

	temp_devices.resize(gpu_count);

	status = vkEnumeratePhysicalDevices(this->m_p_instance, &gpu_count, temp_devices.data());

	VK_ASSERT(status == VK_SUCCESS, "failed to vkEnumeratePhysicalDevices (filling the vector of VkPhysicalDevice)");

	VK_ASSERT(temp_devices.empty() == false, "you must have one videocard at least!");

	for (const auto& p_handle : temp_devices)
	{
		this->m_physical_devices.push_back({p_handle});
	}
}

bool RenderInterface_Vulkan::ChoosePhysicalDevice(VkPhysicalDeviceType device_type) noexcept
{
	VK_ASSERT(this->m_physical_devices.empty() == false,
		"you must have one videocard at least or early calling of this method, try call this after CollectPhysicalDevices");

	for (const auto& device : this->m_physical_devices)
	{
		if (device.GetProperties().deviceType == device_type)
		{
			this->m_p_physical_device_current = device.GetHandle();
			return true;
		}
	}

	return false;
}

void RenderInterface_Vulkan::PrintInformationAboutPickedPhysicalDevice(VkPhysicalDevice p_physical_device) noexcept
{
	if (p_physical_device == nullptr)
		return;

	for (const auto& device : this->m_physical_devices)
	{
		if (p_physical_device == device.GetHandle())
		{
#ifdef RMLUI_DEBUG
			const auto& properties = device.GetProperties();
			Rml::Log::Message(Rml::Log::LT_DEBUG, "Picked physical device: %s", properties.deviceName);
#endif

			return;
		}
	}
}

VkSurfaceFormatKHR RenderInterface_Vulkan::ChooseSwapchainFormat(void) noexcept
{
	VK_ASSERT(this->m_p_physical_device_current, "you must initialize your physical device, before calling this method");
	VK_ASSERT(this->m_p_surface, "you must initialize your surface, before calling this method");

	uint32_t surface_count = 0;

	VkResult status = vkGetPhysicalDeviceSurfaceFormatsKHR(this->m_p_physical_device_current, this->m_p_surface, &surface_count, nullptr);

	VK_ASSERT(status == VK_SUCCESS, "failed to vkGetPhysicalDeviceSurfaceFormatsKHR (getting count)");

	Rml::Vector<VkSurfaceFormatKHR> formats(surface_count);

	status = vkGetPhysicalDeviceSurfaceFormatsKHR(this->m_p_physical_device_current, this->m_p_surface, &surface_count, formats.data());

	VK_ASSERT(status == VK_SUCCESS, "failed to vkGetPhysicalDeviceSurfaceFormatsKHR (filling vector of VkSurfaceFormatKHR)");

	return formats.front();
}

VkExtent2D RenderInterface_Vulkan::Choose_ValidSwapchainExtent(void) noexcept
{
	VkSurfaceCapabilitiesKHR caps = this->GetSurfaceCapabilities();

	VkExtent2D result;

	result.width = this->m_width;
	result.height = this->m_height;

	/*
	    https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkSurfaceCapabilitiesKHR.html
	*/

	if (caps.currentExtent.width == 0xFFFFFFFF)
	{
		if (result.width < caps.minImageExtent.width)
		{
			result.width = caps.minImageExtent.width;
		}
		else if (result.width > caps.maxImageExtent.width)
		{
			result.width = caps.maxImageExtent.width;
		}

		if (result.height < caps.minImageExtent.height)
		{
			result.height = caps.minImageExtent.height;
		}
		else if (result.height > caps.maxImageExtent.height)
		{
			result.height = caps.maxImageExtent.height;
		}
	}
	else
	{
		result = caps.currentExtent;
	}

	return result;
}

VkSurfaceTransformFlagBitsKHR RenderInterface_Vulkan::CreatePretransformSwapchain(void) noexcept
{
	auto caps = this->GetSurfaceCapabilities();

	VkSurfaceTransformFlagBitsKHR result =
		(caps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) ? VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR : caps.currentTransform;

	return result;
}

VkCompositeAlphaFlagBitsKHR RenderInterface_Vulkan::ChooseSwapchainCompositeAlpha(void) noexcept
{
	auto caps = this->GetSurfaceCapabilities();

	VkCompositeAlphaFlagBitsKHR result = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	VkCompositeAlphaFlagBitsKHR composite_alpha_flags[4] = {VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
		VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR, VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR};

	for (uint32_t i = 0; i < sizeof(composite_alpha_flags); ++i)
	{
		if (caps.supportedCompositeAlpha & composite_alpha_flags[i])
		{
			result = composite_alpha_flags[i];
			break;
		}
	}

	return result;
}

int RenderInterface_Vulkan::Choose_SwapchainImageCount(uint32_t user_swapchain_count_for_creation, bool if_failed_choose_min) noexcept
{
	auto caps = this->GetSurfaceCapabilities();

// don't worry if you get this assert just ignore it the method will fix the count ;)
#ifdef RMLUI_DEBUG
	VK_ASSERT(user_swapchain_count_for_creation >= caps.minImageCount,
		"can't be, you must have a valid count that bounds from minImageCount to maxImageCount! Otherwise you will get a validation error that "
		"specifies "
		"that you created a swapchain with invalid image count");
	VK_ASSERT(user_swapchain_count_for_creation <= caps.maxImageCount,
		"can't be, you must have a valid count that bounds from minImageCount to maxImageCount! Otherwise you will get a validation error that "
		"specifies "
		"that you created a swapchain with invalid image count");
#endif

	int result = 0;

	if (user_swapchain_count_for_creation < caps.minImageCount || user_swapchain_count_for_creation > caps.maxImageCount)
		result = if_failed_choose_min ? caps.minImageCount : caps.maxImageCount;
	else
		result = user_swapchain_count_for_creation;

	return result;
}

// https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkPresentModeKHR.html
// VK_PRESENT_MODE_FIFO_KHR system must support this mode at least so by default we want to use it otherwise user can specify his mode
VkPresentModeKHR RenderInterface_Vulkan::GetPresentMode(VkPresentModeKHR required) noexcept
{
	VK_ASSERT(this->m_p_device, "[Vulkan] you must initialize your device, before calling this method");
	VK_ASSERT(this->m_p_physical_device_current, "[Vulkan] you must initialize your physical device, before calling this method");
	VK_ASSERT(this->m_p_surface, "[Vulkan] you must initialize your surface, before calling this method");

	VkPresentModeKHR result = required;

	uint32_t present_modes_count = 0;

	VkResult status = vkGetPhysicalDeviceSurfacePresentModesKHR(this->m_p_physical_device_current, this->m_p_surface, &present_modes_count, nullptr);

	VK_ASSERT(status == VK_SUCCESS, "[Vulkan] failed to vkGetPhysicalDeviceSurfacePresentModesKHR (getting count)");

	Rml::Vector<VkPresentModeKHR> present_modes(present_modes_count);

	status =
		vkGetPhysicalDeviceSurfacePresentModesKHR(this->m_p_physical_device_current, this->m_p_surface, &present_modes_count, present_modes.data());

	VK_ASSERT(status == VK_SUCCESS, "[Vulkan] failed to vkGetPhysicalDeviceSurfacePresentModesKHR (filling vector of VkPresentModeKHR)");

	for (const auto& mode : present_modes)
	{
		if (mode == required)
			return result;
	}

	Rml::Log::Message(Rml::Log::LT_WARNING,
		"[Vulkan] WARNING system can't detect your type of present mode so we choose the first from vector front");

	return present_modes.front();
}

VkSurfaceCapabilitiesKHR RenderInterface_Vulkan::GetSurfaceCapabilities(void) noexcept
{
	VK_ASSERT(this->m_p_device, "[Vulkan] you must initialize your device, before calling this method");
	VK_ASSERT(this->m_p_physical_device_current, "[Vulkan] you must initialize your physical device, before calling this method");
	VK_ASSERT(this->m_p_surface, "[Vulkan] you must initialize your surface, before calling this method");

	VkSurfaceCapabilitiesKHR result;

	VkResult status = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(this->m_p_physical_device_current, this->m_p_surface, &result);

	VK_ASSERT(status == VK_SUCCESS, "[Vulkan] failed to vkGetPhysicalDeviceSurfaceCapabilitiesKHR");

	return result;
}

Rml::Vector<RenderInterface_Vulkan::shader_data_t> RenderInterface_Vulkan::LoadShaders(void) noexcept
{
	Rml::Vector<shader_data_t> result = {
		shader_data_t(reinterpret_cast<const uint32_t*>(shader_vert), sizeof(shader_vert)),
		shader_data_t(reinterpret_cast<const uint32_t*>(shader_frag_color), sizeof(shader_frag_color)),
		shader_data_t(reinterpret_cast<const uint32_t*>(shader_frag_texture), sizeof(shader_frag_texture)),
	};

	return result;
}

bool RenderInterface_Vulkan::Check_IfSwapchainExtentValid(void) noexcept
{
	auto swapchain_extent = this->Choose_ValidSwapchainExtent();
	return (swapchain_extent.width != 0 && swapchain_extent.height != 0);
}

void RenderInterface_Vulkan::CreateShaders(const Rml::Vector<shader_data_t>& storage) noexcept
{
	VK_ASSERT(storage.empty() == false, "[Vulkan] you must load shaders before creating resources");
	VK_ASSERT(this->m_p_device, "[Vulkan] you must initialize VkDevice before calling this method");

	VkShaderModuleCreateInfo info = {};

	for (const auto& shader_data : storage)
	{
		VkShaderModule p_module = nullptr;

		info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		info.pCode = shader_data.Get_Data();
		info.codeSize = shader_data.Get_DataSize();

		VkResult status = vkCreateShaderModule(this->m_p_device, &info, nullptr, &p_module);

		VK_ASSERT(status == VK_SUCCESS, "[Vulkan] failed to vkCreateShaderModule");

		this->m_shaders.push_back(p_module);
	}
}

void RenderInterface_Vulkan::CreateDescriptorSetLayout(Rml::Vector<shader_data_t>& storage) noexcept
{
	VK_ASSERT(storage.empty() == false, "[Vulkan] you must load shaders before creating resources");
	VK_ASSERT(this->m_p_device, "[Vulkan] you must initialize VkDevice before calling this method");

	Rml::Vector<VkDescriptorSetLayoutBinding> all_bindings;

	for (auto& shader_data : storage)
	{
		const auto& current_bindings = this->CreateDescriptorSetLayoutBindings(shader_data);

		all_bindings.insert(all_bindings.end(), current_bindings.begin(), current_bindings.end());
	}

	// yeah... it is hardcoded, but idk how it is better to create two different layouts, because you can't update one dynamic descriptor set
	// which is for uniform buffers... we have to have at least TWO different descriptor set, one is global and single and it is for uniform buffer
	// with offsets, but the second one is for per texture descriptor... it will be hardcoded in anyway, but spirv just automize the process in order
	// to not write the really long and big code for initializing information about our bindings in shaders
	VK_ASSERT(all_bindings.size() > 1, "[Vulkan] something is wrong with SPIRV parsing and obtaining the true amount of bindings");

	auto& binding_for_geometry = all_bindings[0];
	auto& binding_for_textures = all_bindings[1];

	VK_ASSERT(binding_for_geometry.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
		"this binding must be uniform_buffer_dynamic don't change shaders!");
	VK_ASSERT(binding_for_textures.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		"this binding must be for textures don't change shaders!");

	VkDescriptorSetLayoutCreateInfo info = {};

	info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	info.pBindings = &binding_for_geometry;
	info.bindingCount = 1;

	VkDescriptorSetLayout p_layout = nullptr;

	VkResult status = vkCreateDescriptorSetLayout(this->m_p_device, &info, nullptr, &p_layout);

	VK_ASSERT(status == VK_SUCCESS, "[Vulkan] failed to vkCreateDescriptorSetLayout");

	this->m_p_descriptor_set_layout_uniform_buffer_dynamic = p_layout;

	p_layout = nullptr;

	info.pBindings = &binding_for_textures;
	info.bindingCount = 1;

	status = vkCreateDescriptorSetLayout(this->m_p_device, &info, nullptr, &p_layout);

	VK_ASSERT(status == VK_SUCCESS, "[Vulkan] failed to vkCreateDescriptorSetLayout");

	this->m_p_descriptor_set_layout_for_textures = p_layout;
}

Rml::Vector<VkDescriptorSetLayoutBinding> RenderInterface_Vulkan::CreateDescriptorSetLayoutBindings(shader_data_t& data) noexcept
{
	Rml::Vector<VkDescriptorSetLayoutBinding> result;

	VK_ASSERT(data.Get_DataSize() == 0, "[Vulkan] can't be empty data of shader");

	SpvReflectShaderModule spv_module = {};

	SpvReflectResult status = spvReflectCreateShaderModule(data.Get_DataSize(), data.Get_Data(), &spv_module);

	VK_ASSERT(status == SPV_REFLECT_RESULT_SUCCESS, "[Vulkan] SPIRV-Reflect failed to spvReflectCreateShaderModule");

	data.Set_Type(static_cast<VkShaderStageFlagBits>(spv_module.shader_stage));

	uint32_t count = 0;
	status = spvReflectEnumerateDescriptorSets(&spv_module, &count, nullptr);

	VK_ASSERT(status == SPV_REFLECT_RESULT_SUCCESS, "[Vulkan] SPIRV-Reflect failed to spvReflectEnumerateDescriptorSets");

	Rml::Vector<SpvReflectDescriptorSet*> sets(count);
	status = spvReflectEnumerateDescriptorSets(&spv_module, &count, sets.data());

	VK_ASSERT(status == SPV_REFLECT_RESULT_SUCCESS, "[Vulkan] SPIRV-Reflect failed to spvReflectEnumerateDescriptorSets");

	Rml::Vector<VkDescriptorSetLayoutBinding> temp;
	for (const auto* p_descriptor_set : sets)
	{
		temp.resize(p_descriptor_set->binding_count);

		for (uint32_t binding_index = 0; binding_index < p_descriptor_set->binding_count; ++binding_index)
		{
			const SpvReflectDescriptorBinding* p_binding = p_descriptor_set->bindings[binding_index];

			VkDescriptorSetLayoutBinding& info = temp[binding_index];

			info.binding = p_binding->binding;
			info.descriptorCount = 1;

			if (p_binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
				info.descriptorType = static_cast<VkDescriptorType>(SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
			else
				info.descriptorType = static_cast<VkDescriptorType>(p_binding->descriptor_type);

			info.stageFlags = static_cast<VkShaderStageFlagBits>(spv_module.shader_stage);

			for (uint32_t index = 0; index < p_binding->array.dims_count; ++index)
			{
				info.descriptorCount *= p_binding->array.dims[index];
			}
		}

		result.insert(result.end(), temp.begin(), temp.end());
		temp.clear();
	}

	spvReflectDestroyShaderModule(&spv_module);

	return result;
}

void RenderInterface_Vulkan::CreatePipelineLayout(void) noexcept
{
	VK_ASSERT(this->m_p_descriptor_set_layout_uniform_buffer_dynamic,
		"[Vulkan] You must initialize VkDescriptorSetLayout before calling this method");
	VK_ASSERT(this->m_p_descriptor_set_layout_for_textures,
		"[Vulkan] you must initialize VkDescriptorSetLayout for textures before calling this method!");
	VK_ASSERT(this->m_p_device, "[Vulkan] you must initialize VkDevice before calling this method");

	VkDescriptorSetLayout p_layouts[] = {this->m_p_descriptor_set_layout_uniform_buffer_dynamic, this->m_p_descriptor_set_layout_for_textures};

	VkPipelineLayoutCreateInfo info = {};

	info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	info.pNext = nullptr;
	info.pSetLayouts = p_layouts;
	info.setLayoutCount = 2;

	auto status = vkCreatePipelineLayout(this->m_p_device, &info, nullptr, &this->m_p_pipeline_layout);

	VK_ASSERT(status == VK_SUCCESS, "[Vulkan] failed to vkCreatePipelineLayout");
}

void RenderInterface_Vulkan::CreateDescriptorSets(void) noexcept
{
	VK_ASSERT(this->m_p_device, "[Vulkan] you have to initialize your VkDevice before calling this method");
	VK_ASSERT(this->m_p_descriptor_set_layout_uniform_buffer_dynamic,
		"[Vulkan] you have to initialize your VkDescriptorSetLayout before calling this method");

	this->m_manager_descriptors.Alloc_Descriptor(this->m_p_device, &this->m_p_descriptor_set_layout_uniform_buffer_dynamic,
		&this->m_p_descriptor_set);
	this->m_memory_pool.SetDescriptorSet(1, sizeof(shader_vertex_user_data_t), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, this->m_p_descriptor_set);
}

void RenderInterface_Vulkan::CreateSamplers(void) noexcept
{
	VkSamplerCreateInfo info = {};

	info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	info.pNext = nullptr;
	info.magFilter = VK_FILTER_LINEAR;
	info.minFilter = VK_FILTER_LINEAR;
	info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	vkCreateSampler(this->m_p_device, &info, nullptr, &this->m_p_sampler_linear);
}

void RenderInterface_Vulkan::Create_Pipelines(void) noexcept
{
	VK_ASSERT(this->m_p_pipeline_layout, "must be initialized");
	VK_ASSERT(this->m_p_render_pass, "must be initialized");

	VkPipelineInputAssemblyStateCreateInfo info_assembly_state = {};

	info_assembly_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	info_assembly_state.pNext = nullptr;
	info_assembly_state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	info_assembly_state.primitiveRestartEnable = VK_FALSE;
	info_assembly_state.flags = 0;

	VkPipelineRasterizationStateCreateInfo info_raster_state = {};

	info_raster_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	info_raster_state.pNext = nullptr;
	info_raster_state.polygonMode = VK_POLYGON_MODE_FILL;
	info_raster_state.cullMode = VK_CULL_MODE_NONE;
	info_raster_state.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	info_raster_state.rasterizerDiscardEnable = VK_FALSE;
	info_raster_state.depthBiasEnable = VK_FALSE;
	info_raster_state.lineWidth = 1.0f;

	VkPipelineColorBlendAttachmentState info_color_blend_att = {};

	info_color_blend_att.colorWriteMask = 0xf;
	info_color_blend_att.blendEnable = VK_TRUE;
	info_color_blend_att.srcColorBlendFactor = VkBlendFactor::VK_BLEND_FACTOR_SRC_ALPHA;
	info_color_blend_att.dstColorBlendFactor = VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	info_color_blend_att.colorBlendOp = VkBlendOp::VK_BLEND_OP_ADD;
	info_color_blend_att.srcAlphaBlendFactor = VkBlendFactor::VK_BLEND_FACTOR_SRC_ALPHA;
	info_color_blend_att.dstAlphaBlendFactor = VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	info_color_blend_att.alphaBlendOp = VkBlendOp::VK_BLEND_OP_SUBTRACT;

	VkPipelineColorBlendStateCreateInfo info_color_blend_state = {};

	info_color_blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	info_color_blend_state.pNext = nullptr;
	info_color_blend_state.attachmentCount = 1;
	info_color_blend_state.pAttachments = &info_color_blend_att;

	VkPipelineDepthStencilStateCreateInfo info_depth = {};

	info_depth.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	info_depth.pNext = nullptr;
	info_depth.depthTestEnable = VK_FALSE;
	info_depth.depthWriteEnable = VK_TRUE;
	info_depth.depthBoundsTestEnable = VK_FALSE;
	info_depth.maxDepthBounds = 1.0f;

	info_depth.depthCompareOp = VK_COMPARE_OP_ALWAYS;

	info_depth.stencilTestEnable = VK_TRUE;
	info_depth.back.compareOp = VK_COMPARE_OP_ALWAYS;
	info_depth.back.failOp = VK_STENCIL_OP_KEEP;
	info_depth.back.depthFailOp = VK_STENCIL_OP_KEEP;
	info_depth.back.passOp = VK_STENCIL_OP_KEEP;
	info_depth.back.compareMask = 1;
	info_depth.back.writeMask = 1;
	info_depth.back.reference = 1;
	info_depth.front = info_depth.back;
	VkPipelineViewportStateCreateInfo info_viewport = {};

	info_viewport.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	info_viewport.pNext = nullptr;
	info_viewport.viewportCount = 1;
	info_viewport.scissorCount = 1;
	info_viewport.flags = 0;

	VkPipelineMultisampleStateCreateInfo info_multisample = {};

	info_multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	info_multisample.pNext = nullptr;
	info_multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	info_multisample.flags = 0;

	Rml::Array<VkDynamicState, 2> dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

	VkPipelineDynamicStateCreateInfo info_dynamic_state = {};

	info_dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	info_dynamic_state.pNext = nullptr;
	info_dynamic_state.pDynamicStates = dynamicStateEnables.data();
	info_dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
	info_dynamic_state.flags = 0;

	Rml::Array<VkPipelineShaderStageCreateInfo, 2> shaders_that_will_be_used_in_pipeline;

	VkPipelineShaderStageCreateInfo info_shader = {};

	info_shader.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	info_shader.pNext = nullptr;
	info_shader.pName = "main";
	info_shader.stage = VK_SHADER_STAGE_VERTEX_BIT;
	info_shader.module = this->m_shaders[static_cast<int>(shader_id_t::kShaderID_Vertex)];

	shaders_that_will_be_used_in_pipeline[0] = info_shader;

	info_shader.module = this->m_shaders[static_cast<int>(shader_id_t::kShaderID_Pixel_WithTextures)];
	info_shader.stage = VK_SHADER_STAGE_FRAGMENT_BIT;

	shaders_that_will_be_used_in_pipeline[1] = info_shader;

	VkPipelineVertexInputStateCreateInfo info_vertex = {};

	info_vertex.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	info_vertex.pNext = nullptr;
	info_vertex.flags = 0;

	Rml::Array<VkVertexInputAttributeDescription, 3> info_shader_vertex_attributes;
	// describe info about our vertex and what is used in vertex shader as "layout(location = X) in"

	VkVertexInputBindingDescription info_vertex_input_binding = {};
	info_vertex_input_binding.binding = 0;
	info_vertex_input_binding.stride = sizeof(Rml::Vertex);
	info_vertex_input_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	info_shader_vertex_attributes[0].binding = 0;
	info_shader_vertex_attributes[0].location = 0;
	info_shader_vertex_attributes[0].format = VK_FORMAT_R32G32_SFLOAT;
	info_shader_vertex_attributes[0].offset = offsetof(Rml::Vertex, position);

	info_shader_vertex_attributes[1].binding = 0;
	info_shader_vertex_attributes[1].location = 1;
	info_shader_vertex_attributes[1].format = VK_FORMAT_R8G8B8A8_UNORM;
	info_shader_vertex_attributes[1].offset = offsetof(Rml::Vertex, colour);

	info_shader_vertex_attributes[2].binding = 0;
	info_shader_vertex_attributes[2].location = 2;
	info_shader_vertex_attributes[2].format = VK_FORMAT_R32G32_SFLOAT;
	info_shader_vertex_attributes[2].offset = offsetof(Rml::Vertex, tex_coord);

	info_vertex.pVertexAttributeDescriptions = info_shader_vertex_attributes.data();
	info_vertex.vertexAttributeDescriptionCount = static_cast<uint32_t>(info_shader_vertex_attributes.size());
	info_vertex.pVertexBindingDescriptions = &info_vertex_input_binding;
	info_vertex.vertexBindingDescriptionCount = 1;

	VkGraphicsPipelineCreateInfo info = {};

	info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	info.pNext = nullptr;
	info.pInputAssemblyState = &info_assembly_state;
	info.pRasterizationState = &info_raster_state;
	info.pColorBlendState = &info_color_blend_state;
	info.pMultisampleState = &info_multisample;
	info.pViewportState = &info_viewport;
	info.pDepthStencilState = &info_depth;
	info.pDynamicState = &info_dynamic_state;
	info.stageCount = static_cast<uint32_t>(shaders_that_will_be_used_in_pipeline.size());
	info.pStages = shaders_that_will_be_used_in_pipeline.data();
	info.pVertexInputState = &info_vertex;
	info.layout = this->m_p_pipeline_layout;
	info.renderPass = this->m_p_render_pass;
	info.subpass = 0;

	auto status = vkCreateGraphicsPipelines(this->m_p_device, nullptr, 1, &info, nullptr, &this->m_p_pipeline_with_textures);

	VK_ASSERT(status == VkResult::VK_SUCCESS, "failed to vkCreateGraphicsPipelines");

	info_depth.back.passOp = VK_STENCIL_OP_KEEP;
	info_depth.back.failOp = VK_STENCIL_OP_KEEP;
	info_depth.back.depthFailOp = VK_STENCIL_OP_KEEP;
	info_depth.back.compareOp = VK_COMPARE_OP_EQUAL;
	info_depth.back.compareMask = 1;
	info_depth.back.writeMask = 1;
	info_depth.back.reference = 1;
	info_depth.front = info_depth.back;

	status = vkCreateGraphicsPipelines(this->m_p_device, nullptr, 1, &info, nullptr,
		&this->m_p_pipeline_stencil_for_regular_geometry_that_applied_to_region_with_textures);

	VK_ASSERT(status == VkResult::VK_SUCCESS, "failed to vkCreateGraphicsPipelines");

	info_shader.module = this->m_shaders[static_cast<int>(shader_id_t::kShaderID_Pixel_WithoutTextures)];
	shaders_that_will_be_used_in_pipeline[1] = info_shader;
	info_depth.back.compareOp = VK_COMPARE_OP_ALWAYS;
	info_depth.back.failOp = VK_STENCIL_OP_KEEP;
	info_depth.back.depthFailOp = VK_STENCIL_OP_KEEP;
	info_depth.back.passOp = VK_STENCIL_OP_KEEP;
	info_depth.back.compareMask = 1;
	info_depth.back.writeMask = 1;
	info_depth.back.reference = 1;
	info_depth.front = info_depth.back;

	status = vkCreateGraphicsPipelines(this->m_p_device, nullptr, 1, &info, nullptr, &this->m_p_pipeline_without_textures);

	VK_ASSERT(status == VkResult::VK_SUCCESS, "failed to vkCreateGraphicsPipelines");

	info_depth.back.passOp = VK_STENCIL_OP_KEEP;
	info_depth.back.failOp = VK_STENCIL_OP_KEEP;
	info_depth.back.depthFailOp = VK_STENCIL_OP_KEEP;
	info_depth.back.compareOp = VK_COMPARE_OP_EQUAL;
	info_depth.back.compareMask = 1;
	info_depth.back.writeMask = 1;
	info_depth.back.reference = 1;
	info_depth.front = info_depth.back;

	status = vkCreateGraphicsPipelines(this->m_p_device, nullptr, 1, &info, nullptr,
		&this->m_p_pipeline_stencil_for_regular_geometry_that_applied_to_region_without_textures);

	VK_ASSERT(status == VkResult::VK_SUCCESS, "failed to vkCreateGraphicsPipelines");

	info_color_blend_att.colorWriteMask = 0x0;
	info_depth.back.passOp = VK_STENCIL_OP_REPLACE;
	info_depth.back.failOp = VK_STENCIL_OP_KEEP;
	info_depth.back.depthFailOp = VK_STENCIL_OP_KEEP;
	info_depth.back.compareOp = VK_COMPARE_OP_ALWAYS;
	info_depth.back.compareMask = 1;
	info_depth.back.writeMask = 1;
	info_depth.back.reference = 1;
	info_depth.front = info_depth.back;

	status =
		vkCreateGraphicsPipelines(this->m_p_device, nullptr, 1, &info, nullptr, &this->m_p_pipeline_stencil_for_region_where_geometry_will_be_drawn);

	VK_ASSERT(status == VkResult::VK_SUCCESS, "failed to vkCreateGraphicsPipelines");
}

void RenderInterface_Vulkan::CreateSwapchainFrameBuffers(void) noexcept
{
	VK_ASSERT(this->m_p_render_pass, "you must create a VkRenderPass before calling this method");
	VK_ASSERT(this->m_p_device, "you must have a valid VkDevice here");

	this->CreateSwapchainImageViews();
	this->Create_DepthStencilImage();
	this->Create_DepthStencilImageViews();

	this->m_swapchain_frame_buffers.resize(this->m_swapchain_image_views.size());

	Rml::Array<VkImageView, 2> attachments;

	VkFramebufferCreateInfo info = {};

	info.sType = VkStructureType::VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	info.pNext = nullptr;
	info.renderPass = this->m_p_render_pass;
	info.attachmentCount = static_cast<uint32_t>(attachments.size());
	info.pAttachments = attachments.data();
	info.width = this->m_width;
	info.height = this->m_height;
	info.layers = 1;

	int index = 0;
	VkResult status = VkResult::VK_SUCCESS;

	attachments[1] = this->m_texture_depthstencil.Get_VkImageView();

	for (auto p_view : this->m_swapchain_image_views)
	{
		attachments[0] = p_view;

		status = vkCreateFramebuffer(this->m_p_device, &info, nullptr, &this->m_swapchain_frame_buffers[index]);

		VK_ASSERT(status == VkResult::VK_SUCCESS, "failed to vkCreateFramebuffer");

		++index;
	}
}

void RenderInterface_Vulkan::CreateSwapchainImages(void) noexcept
{
	VK_ASSERT(this->m_p_device, "[Vulkan] you must initialize VkDevice before calling this method");
	VK_ASSERT(this->m_p_swapchain, "[Vulkan] you must initialize VkSwapchainKHR before calling this method");

	uint32_t count = 0;
	auto status = vkGetSwapchainImagesKHR(this->m_p_device, this->m_p_swapchain, &count, nullptr);

	VK_ASSERT(status == VK_SUCCESS, "[Vulkan] failed to vkGetSwapchainImagesKHR (get count)");

	this->m_swapchain_images.resize(count);

	status = vkGetSwapchainImagesKHR(this->m_p_device, this->m_p_swapchain, &count, this->m_swapchain_images.data());

	VK_ASSERT(status == VK_SUCCESS, "[Vulkan] failed to vkGetSwapchainImagesKHR (filling vector)");
}

void RenderInterface_Vulkan::CreateSwapchainImageViews(void) noexcept
{
	this->CreateSwapchainImages();

	this->m_swapchain_image_views.resize(this->m_swapchain_images.size());

	uint32_t index = 0;
	VkImageViewCreateInfo info = {};
	VkResult status = VkResult::VK_SUCCESS;

	for (auto p_image : this->m_swapchain_images)
	{
		info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		info.pNext = nullptr;
		info.format = this->m_swapchain_format.format;
		info.components.r = VK_COMPONENT_SWIZZLE_R;
		info.components.g = VK_COMPONENT_SWIZZLE_G;
		info.components.b = VK_COMPONENT_SWIZZLE_B;
		info.components.a = VK_COMPONENT_SWIZZLE_A;
		info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		info.subresourceRange.baseMipLevel = 0;
		info.subresourceRange.levelCount = 1;
		info.subresourceRange.baseArrayLayer = 0;
		info.subresourceRange.layerCount = 1;
		info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		info.flags = 0;
		info.image = p_image;

		status = vkCreateImageView(this->m_p_device, &info, nullptr, &this->m_swapchain_image_views.at(index));
		++index;

		VK_ASSERT(status == VK_SUCCESS, "[Vulkan] failed to vkCreateImageView (creating swapchain views)");
	}
}

void RenderInterface_Vulkan::Create_DepthStencilImage(void) noexcept
{
	VK_ASSERT(this->m_p_device, "you must initialize your VkDevice here");
	VK_ASSERT(this->m_p_allocator, "you must initialize your VMA allcator");
	VK_ASSERT(this->m_texture_depthstencil.Get_VkImage() == nullptr, "you should delete texture before create it");

	VkImageCreateInfo info = {};

	info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	info.imageType = VK_IMAGE_TYPE_2D;
	info.format = this->Get_SupportedDepthFormat();
	info.extent = {static_cast<uint32_t>(this->m_width), static_cast<uint32_t>(this->m_height), 1};
	info.mipLevels = 1;
	info.arrayLayers = 1;
	info.samples = VK_SAMPLE_COUNT_1_BIT;
	info.tiling = VK_IMAGE_TILING_OPTIMAL;
	info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

	VmaAllocation p_allocation = {};
	VkImage p_image = {};

	VmaAllocationCreateInfo info_alloc = {};
	auto p_commentary = "our depth stencil image";

	info_alloc.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	info_alloc.flags = VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
	info_alloc.pUserData = const_cast<char*>(p_commentary);

	VkResult status = vmaCreateImage(this->m_p_allocator, &info, &info_alloc, &p_image, &p_allocation, nullptr);

	VK_ASSERT(status == VkResult::VK_SUCCESS, "failed to vmaCreateImage");

	this->m_texture_depthstencil.Set_VkImage(p_image);
	this->m_texture_depthstencil.Set_VmaAllocation(p_allocation);
}

void RenderInterface_Vulkan::Create_DepthStencilImageViews(void) noexcept
{
	VK_ASSERT(this->m_p_device, "you must initialize your VkDevice here");
	VK_ASSERT(this->m_texture_depthstencil.Get_VkImageView() == nullptr, "you should delete it before creating");
	VK_ASSERT(this->m_texture_depthstencil.Get_VkImage(), "you must initialize VkImage before create this");

	VkImageViewCreateInfo info = {};

	info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	info.image = this->m_texture_depthstencil.Get_VkImage();
	info.format = this->Get_SupportedDepthFormat();
	info.subresourceRange.baseMipLevel = 0;
	info.subresourceRange.levelCount = 1;
	info.subresourceRange.baseArrayLayer = 0;
	info.subresourceRange.layerCount = 1;
	info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

	if (this->Get_SupportedDepthFormat() >= VK_FORMAT_D16_UNORM_S8_UINT)
	{
		info.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
	}

	VkImageView p_image_view = {};

	VkResult status = vkCreateImageView(this->m_p_device, &info, nullptr, &p_image_view);

	VK_ASSERT(status == VkResult::VK_SUCCESS, "failed to vkCreateImageView");

	this->m_texture_depthstencil.Set_VkImageView(p_image_view);
}

void RenderInterface_Vulkan::CreateResourcesDependentOnSize(void) noexcept
{
	this->m_viewport.height = static_cast<float>(this->m_height);
	this->m_viewport.width = static_cast<float>(this->m_width);
	this->m_viewport.minDepth = 0.0f;
	this->m_viewport.maxDepth = 1.0f;
	this->m_viewport.x = 0.0f;
	this->m_viewport.y = 0.0f;

	this->m_scissor.extent.width = this->m_width;
	this->m_scissor.extent.height = this->m_height;
	this->m_scissor.offset.x = 0;
	this->m_scissor.offset.y = 0;

	this->m_scissor_original = this->m_scissor;

	this->m_projection =
		Rml::Matrix4f::ProjectOrtho(0.0f, static_cast<float>(this->m_width), static_cast<float>(this->m_height), 0.0f, -10000, 10000);

	// https://matthewwellings.com/blog/the-new-vulkan-coordinate-system/
	Rml::Matrix4f correction_matrix;
	correction_matrix.SetColumns(Rml::Vector4f(1.0f, 0.0f, 0.0f, 0.0f), Rml::Vector4f(0.0f, -1.0f, 0.0f, 0.0f), Rml::Vector4f(0.0f, 0.0f, 0.5f, 0.0f),
		Rml::Vector4f(0.0f, 0.0f, 0.5f, 1.0f));

	this->m_projection = correction_matrix * this->m_projection;

	this->SetTransform(nullptr);

	this->CreateRenderPass();
	this->CreateSwapchainFrameBuffers();
	this->Create_Pipelines();
}

RenderInterface_Vulkan::buffer_data_t RenderInterface_Vulkan::CreateResource_StagingBuffer(VkDeviceSize size, VkBufferUsageFlags flags) noexcept
{
	buffer_data_t result;

	VkBufferCreateInfo info = {};

	info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	info.pNext = nullptr;
	info.size = size;
	info.usage = flags;

	VmaAllocationCreateInfo info_allocation = {};
	info_allocation.usage = VMA_MEMORY_USAGE_CPU_ONLY;

	VkBuffer p_buffer = nullptr;
	VmaAllocation p_allocation = nullptr;

	VmaAllocationInfo info_stats = {};

	VkResult status = vmaCreateBuffer(this->m_p_allocator, &info, &info_allocation, &p_buffer, &p_allocation, &info_stats);

	VK_ASSERT(status == VkResult::VK_SUCCESS, "failed to vmaCreateBuffer");

#ifdef RMLUI_DEBUG
	Rml::Log::Message(Rml::Log::LT_DEBUG, "allocated buffer with size %zu in bytes", info_stats.size);
#endif

	result.Set_VkBuffer(p_buffer);
	result.Set_VmaAllocation(p_allocation);

	return result;
}

void RenderInterface_Vulkan::DestroyResource_StagingBuffer(const buffer_data_t& data) noexcept
{
	if (this->m_p_allocator)
	{
		if (data.Get_VkBuffer() && data.Get_VmaAllocation())
		{
			vmaDestroyBuffer(this->m_p_allocator, data.Get_VkBuffer(), data.Get_VmaAllocation());
		}
	}
}

void RenderInterface_Vulkan::Destroy_Textures(void) noexcept
{
	for (auto& textures : this->m_pending_for_deletion_textures_by_frames)
	{
		for (auto* p_data : textures)
		{
			this->Destroy_Texture(*p_data);

			p_data->Set_VmaAllocation(nullptr);
			p_data->Set_VkDescriptorSet(nullptr);
			p_data->Set_VkImage(nullptr);
			p_data->Set_VkImageView(nullptr);
			p_data->Set_VkSampler(nullptr);
			p_data->Set_Width(-1);
			p_data->Set_Height(-1);
			p_data->Set_ID(-1);

			delete p_data;
		}

		textures.clear();
	}
}

void RenderInterface_Vulkan::Destroy_Geometries(void) noexcept
{
	this->Update_PendingForDeletion_Geometries();
	this->m_memory_pool.Shutdown();
}

void RenderInterface_Vulkan::Destroy_Texture(const texture_data_t& texture) noexcept
{
	VK_ASSERT(this->m_p_allocator, "you must have initialized VmaAllocator");
	VK_ASSERT(this->m_p_device, "you must have initialized VkDevice");

	if (texture.Get_VmaAllocation())
	{
		vmaDestroyImage(this->m_p_allocator, texture.Get_VkImage(), texture.Get_VmaAllocation());
		vkDestroyImageView(this->m_p_device, texture.Get_VkImageView(), nullptr);

		VkDescriptorSet p_set = texture.Get_VkDescriptorSet();

		if (p_set)
		{
			this->m_manager_descriptors.Free_Descriptors(this->m_p_device, &p_set);
		}
	}
}

void RenderInterface_Vulkan::DestroyResourcesDependentOnSize(void) noexcept
{
	this->Destroy_Pipelines();
	this->DestroySwapchainFrameBuffers();
	this->DestroyRenderPass();
}

void RenderInterface_Vulkan::DestroySwapchainImageViews(void) noexcept
{
	VK_ASSERT(this->m_p_device, "[Vulkan] you must initialize VkDevice before calling this method");

	this->m_swapchain_images.clear();

	for (auto p_view : this->m_swapchain_image_views)
	{
		vkDestroyImageView(this->m_p_device, p_view, nullptr);
	}

	this->m_swapchain_image_views.clear();
}

void RenderInterface_Vulkan::DestroySwapchainFrameBuffers(void) noexcept
{
	this->DestroySwapchainImageViews();

	this->Destroy_Texture(this->m_texture_depthstencil);
	this->m_texture_depthstencil.Set_VkImage(nullptr);
	this->m_texture_depthstencil.Set_VkImageView(nullptr);

	for (auto p_frame_buffer : this->m_swapchain_frame_buffers)
	{
		vkDestroyFramebuffer(this->m_p_device, p_frame_buffer, nullptr);
	}

	this->m_swapchain_frame_buffers.clear();
}

void RenderInterface_Vulkan::DestroyRenderPass(void) noexcept
{
	VK_ASSERT(this->m_p_device, "you must have a valid VkDevice here");

	if (this->m_p_render_pass)
	{
		vkDestroyRenderPass(this->m_p_device, this->m_p_render_pass, nullptr);
		this->m_p_render_pass = nullptr;
	}
}

void RenderInterface_Vulkan::Destroy_Pipelines(void) noexcept
{
	VK_ASSERT(this->m_p_device, "must exist here");

	vkDestroyPipeline(this->m_p_device, this->m_p_pipeline_with_textures, nullptr);
	vkDestroyPipeline(this->m_p_device, this->m_p_pipeline_without_textures, nullptr);
	vkDestroyPipeline(this->m_p_device, this->m_p_pipeline_stencil_for_region_where_geometry_will_be_drawn, nullptr);
	vkDestroyPipeline(this->m_p_device, this->m_p_pipeline_stencil_for_regular_geometry_that_applied_to_region_with_textures, nullptr);
	vkDestroyPipeline(this->m_p_device, this->m_p_pipeline_stencil_for_regular_geometry_that_applied_to_region_without_textures, nullptr);
}

void RenderInterface_Vulkan::DestroyDescriptorSets(void) noexcept {}

void RenderInterface_Vulkan::DestroyPipelineLayout(void) noexcept {}

void RenderInterface_Vulkan::DestroySamplers(void) noexcept
{
	VK_ASSERT(this->m_p_device, "must exist here");
	vkDestroySampler(this->m_p_device, this->m_p_sampler_linear, nullptr);
}

void RenderInterface_Vulkan::CreateRenderPass(void) noexcept
{
	VK_ASSERT(this->m_p_device, "you must have a valid VkDevice here");

	Rml::Array<VkAttachmentDescription, 2> attachments = {};

	attachments[0].format = this->m_swapchain_format.format;
	attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;

// @ in order to be sure that pClearValues and rendering are working properly
#ifdef RMLUI_DEBUG
	attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
#else
	attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
#endif

	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	attachments[1].format = this->Get_SupportedDepthFormat();
	attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VK_ASSERT(attachments[1].format != VkFormat::VK_FORMAT_UNDEFINED,
		"can't obtain depth format, your device doesn't support depth/stencil operations");

	Rml::Array<VkAttachmentReference, 2> color_references;

	// swapchain
	color_references[0].attachment = 0;
	color_references[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// depth stencil
	color_references[1].attachment = 1;
	color_references[1].layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};

	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.flags = 0;
	subpass.inputAttachmentCount = 0;
	subpass.pInputAttachments = nullptr;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_references[0];
	subpass.pResolveAttachments = nullptr;
	subpass.pDepthStencilAttachment = &color_references[1];
	subpass.preserveAttachmentCount = 0;
	subpass.pPreserveAttachments = nullptr;

	Rml::Array<VkSubpassDependency, 2> dependencies = {};

	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcAccessMask = 0;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].dstSubpass = 0;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependencies[1].srcAccessMask = 0;
	dependencies[1].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkRenderPassCreateInfo info = {};

	info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	info.pNext = nullptr;
	info.attachmentCount = static_cast<uint32_t>(attachments.size());
	info.pAttachments = attachments.data();
	info.subpassCount = 1;
	info.pSubpasses = &subpass;
	info.dependencyCount = static_cast<uint32_t>(dependencies.size());
	info.pDependencies = dependencies.data();

	VkResult status = vkCreateRenderPass(this->m_p_device, &info, nullptr, &this->m_p_render_pass);

	VK_ASSERT(status == VK_SUCCESS, "failed to vkCreateRenderPass");
}

void RenderInterface_Vulkan::Wait(void) noexcept
{
	VK_ASSERT(this->m_p_device, "you must initialize device");
	VK_ASSERT(this->m_p_swapchain, "you must initialize swapchain");

	constexpr uint64_t kMaxUint64 = std::numeric_limits<uint64_t>::max();

	auto status = vkAcquireNextImageKHR(this->m_p_device, this->m_p_swapchain, kMaxUint64,
		this->m_semaphores_image_available.at(this->m_semaphore_index), nullptr, &this->m_image_index);

	VK_ASSERT(status == VkResult::VK_SUCCESS, "failed to vkAcquireNextImageKHR (see status)");

	this->m_semaphore_index_previous = this->m_semaphore_index;
	++this->m_semaphore_index;

	if (this->m_semaphore_index >= kSwapchainBackBufferCount)
		this->m_semaphore_index = 0;

	status = vkWaitForFences(this->m_p_device, 1, &this->m_executed_fences[this->m_semaphore_index_previous], VK_TRUE, kMaxUint64);

	VK_ASSERT(status == VkResult::VK_SUCCESS, "failed to vkWaitForFences (see status)");

	status = vkResetFences(this->m_p_device, 1, &this->m_executed_fences[this->m_semaphore_index_previous]);

	VK_ASSERT(status == VkResult::VK_SUCCESS, "failed to vkResetFences (see status)");
}

void RenderInterface_Vulkan::Update_PendingForDeletion_Textures_By_Frames(void) noexcept
{
	auto& textures_for_previous_frame = this->m_pending_for_deletion_textures_by_frames.at(this->m_semaphore_index_previous);

	for (auto* p_data : textures_for_previous_frame)
	{
		this->Destroy_Texture(*p_data);

		p_data->Set_VmaAllocation(nullptr);
		p_data->Set_VkDescriptorSet(nullptr);
		p_data->Set_VkImage(nullptr);
		p_data->Set_VkImageView(nullptr);
		p_data->Set_VkSampler(nullptr);
		p_data->Set_Width(-1);
		p_data->Set_Height(-1);
		p_data->Set_ID(-1);

		delete p_data;
	}

	textures_for_previous_frame.clear();
}

void RenderInterface_Vulkan::Update_PendingForDeletion_Geometries(void) noexcept
{
	for (auto* p_geometry_handle : this->m_pending_for_deletion_geometries)
	{
		this->m_memory_pool.Free_GeometryHandle(p_geometry_handle);
		delete p_geometry_handle;
	}

	this->m_pending_for_deletion_geometries.clear();
}

void RenderInterface_Vulkan::Submit(void) noexcept
{
	const VkSemaphore p_semaphores_wait[] = {this->m_semaphores_image_available.at(this->m_semaphore_index_previous)};

	const VkSemaphore p_semaphores_signal[] = {this->m_semaphores_finished_render.at(this->m_semaphore_index)};

	VkFence p_fence = this->m_executed_fences.at(this->m_semaphore_index_previous);

	VkPipelineStageFlags submit_wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	VkSubmitInfo info = {};

	info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	info.pNext = nullptr;
	info.waitSemaphoreCount = 1;
	info.pWaitSemaphores = p_semaphores_wait;
	info.pWaitDstStageMask = &submit_wait_stage;
	info.signalSemaphoreCount = 1;
	info.pSignalSemaphores = p_semaphores_signal;
	info.commandBufferCount = 1;
	info.pCommandBuffers = &this->m_p_current_command_buffer;

	VkResult status = vkQueueSubmit(this->m_p_queue_graphics, 1, &info, p_fence);

	VK_ASSERT(status == VK_SUCCESS, "failed to vkQueueSubmit");
}

void RenderInterface_Vulkan::Present(void) noexcept
{
	VkPresentInfoKHR info = {};

	info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	info.pNext = nullptr;
	info.waitSemaphoreCount = 1;
	info.pWaitSemaphores = &(this->m_semaphores_finished_render.at(this->m_semaphore_index));
	info.swapchainCount = 1;
	info.pSwapchains = &this->m_p_swapchain;
	info.pImageIndices = &this->m_image_index;
	info.pResults = nullptr;

	VkResult status = vkQueuePresentKHR(this->m_p_queue_present, &info);

	if (!(status == VK_SUCCESS))
	{
		if (status == VK_ERROR_OUT_OF_DATE_KHR)
		{
			this->OnResize(this->m_width, this->m_height);
		}
		else
		{
			VK_ASSERT(status == VK_SUCCESS, "failed to vkQueuePresentKHR");
		}
	}
}

VkFormat RenderInterface_Vulkan::Get_SupportedDepthFormat(void)
{
	VK_ASSERT(this->m_p_physical_device_current, "you must initialize and pick physical device for your renderer");

	Rml::Array<VkFormat, 5> formats = {VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT,
		VK_FORMAT_D16_UNORM};

	VkFormatProperties properties;
	for (const auto& format : formats)
	{
		vkGetPhysicalDeviceFormatProperties(this->m_p_physical_device_current, format, &properties);

		if (properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
		{
			return format;
		}
	}

	return VkFormat::VK_FORMAT_UNDEFINED;
}

RenderInterface_Vulkan::PhysicalDeviceWrapper::PhysicalDeviceWrapper(VkPhysicalDevice p_physical_device) : m_p_physical_device(p_physical_device)
{
	VK_ASSERT(this->m_p_physical_device, "you passed an invalid pointer of VkPhysicalDevice");

	vkGetPhysicalDeviceProperties(this->m_p_physical_device, &this->m_physical_device_properties);

	this->m_physical_device_limits = this->m_physical_device_properties.limits;
}

RenderInterface_Vulkan::PhysicalDeviceWrapper::PhysicalDeviceWrapper(void) :
	m_p_physical_device{}, m_physical_device_properties{}, m_physical_device_limits{}
{}

RenderInterface_Vulkan::PhysicalDeviceWrapper::~PhysicalDeviceWrapper(void) {}

VkPhysicalDevice RenderInterface_Vulkan::PhysicalDeviceWrapper::GetHandle(void) const noexcept
{
	return this->m_p_physical_device;
}

const VkPhysicalDeviceProperties& RenderInterface_Vulkan::PhysicalDeviceWrapper::GetProperties(void) const noexcept
{
	return this->m_physical_device_properties;
}

const VkPhysicalDeviceLimits& RenderInterface_Vulkan::PhysicalDeviceWrapper::GetLimits(void) const noexcept
{
	return this->m_physical_device_limits;
}

RenderInterface_Vulkan::CommandListRing::CommandListRing(void) :
	m_frame_index{}, m_number_of_frames{}, m_command_lists_per_frame{}, m_p_device{}, m_p_current_frame{}
{}

RenderInterface_Vulkan::CommandListRing::~CommandListRing(void) {}

void RenderInterface_Vulkan::CommandListRing::Initialize(VkDevice p_device, uint32_t queue_index_graphics, uint32_t number_of_back_buffers,
	uint32_t command_list_per_frame) noexcept
{
	VK_ASSERT(p_device, "you can't pass an invalid VkDevice here");
	VK_ASSERT(number_of_back_buffers, "you must pass a positive value or just greater than 0");
	VK_ASSERT(command_list_per_frame, "you must pass a positive value or just greater than 0");

	this->m_number_of_frames = number_of_back_buffers;
	this->m_command_lists_per_frame = command_list_per_frame;
	this->m_p_device = p_device;
	this->m_frames.resize(this->m_number_of_frames);

	for (uint32_t current_frame_index = 0; current_frame_index < this->m_number_of_frames; ++current_frame_index)
	{
		CommandBufferPerFrame* p_current_buffer = &this->m_frames[current_frame_index];

		p_current_buffer->SetCountCommandListsAndPools(command_list_per_frame);

		for (uint32_t current_command_buffer_index = 0; current_command_buffer_index < this->m_command_lists_per_frame;
			 ++current_command_buffer_index)
		{
			VkCommandPoolCreateInfo info_pool = {};

			info_pool.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			info_pool.pNext = nullptr;

			info_pool.queueFamilyIndex = queue_index_graphics;

			info_pool.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

			VkCommandPool p_pool = nullptr;

			auto status = vkCreateCommandPool(p_device, &info_pool, nullptr, &p_pool);

			VK_ASSERT(status == VkResult::VK_SUCCESS, "can't create command pool");

			p_current_buffer->AddCommandPools(p_pool);

			VkCommandBufferAllocateInfo info_buffer = {};

			info_buffer.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			info_buffer.pNext = nullptr;
			info_buffer.commandPool = p_current_buffer->GetCommandPools()[current_command_buffer_index];
			info_buffer.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			info_buffer.commandBufferCount = 1;

			VkCommandBuffer p_buffer = nullptr;

			status = vkAllocateCommandBuffers(p_device, &info_buffer, &p_buffer);

			VK_ASSERT(status == VkResult::VK_SUCCESS, "failed to fill command buffers");

			p_current_buffer->AddCommandBuffers(p_buffer);
		}
	}

	this->m_frame_index = 0;
	this->m_p_current_frame = &this->m_frames[this->m_frame_index % this->m_number_of_frames];
	this->m_p_current_frame->SetUsedCalls(0);
}

void RenderInterface_Vulkan::CommandListRing::Shutdown(void)
{
	VK_ASSERT(this->m_p_device, "you can't have an uninitialized VkDevice");

	for (uint32_t i = 0; i < this->m_number_of_frames; ++i)
	{
		for (uint32_t j = 0; j < this->m_command_lists_per_frame; ++j)
		{
			vkFreeCommandBuffers(this->m_p_device, this->m_frames.at(i).GetCommandPools().at(j), 1, &this->m_frames.at(i).GetCommandBuffers().at(j));
			vkDestroyCommandPool(this->m_p_device, this->m_frames.at(i).GetCommandPools().at(j), nullptr);
		}
	}
}

void RenderInterface_Vulkan::CommandListRing::OnBeginFrame(void)
{
	this->m_p_current_frame = &this->m_frames.at(this->m_frame_index % this->m_number_of_frames);

	this->m_p_current_frame->SetUsedCalls(0);

	++this->m_frame_index;
}

VkCommandBuffer RenderInterface_Vulkan::CommandListRing::GetNewCommandList(void)
{
	VK_ASSERT(this->m_p_current_frame, "must be valid");
	VK_ASSERT(this->m_p_device, "you must initialize your VkDevice field with valid pointer or it's uninitialized field");

	uint32_t current_call = this->m_p_current_frame->GetUsedCalls();

	auto status = vkResetCommandPool(this->m_p_device, this->m_p_current_frame->GetCommandPools().at(current_call), 0);

	VK_ASSERT(status == VkResult::VK_SUCCESS, "failed to vkResetCommandPool");

	VkCommandBuffer result = this->m_p_current_frame->GetCommandBuffers().at(current_call);

	VK_ASSERT(result, "your VkCommandBuffer must be valid otherwise debug your command list class for frame");

	VK_ASSERT(this->m_p_current_frame->GetUsedCalls() < this->m_command_lists_per_frame,
		"overflow, you must call GetNewCommandList only for specified count. So it means if you set m_command_lists_per_frame=2 you can call "
		"GetNewCommandList twice in your function scope.");

	++current_call;
	this->m_p_current_frame->SetUsedCalls(current_call);

	return result;
}

const Rml::Vector<VkCommandBuffer>& RenderInterface_Vulkan::CommandListRing::GetAllCommandBuffersForCurrentFrame(void) const noexcept
{
	VK_ASSERT(this->m_p_current_frame, "you must have a valid pointer here");

	return this->m_p_current_frame->GetCommandBuffers();
}

uint32_t RenderInterface_Vulkan::CommandListRing::GetCountOfCommandBuffersPerFrame(void) const noexcept
{
	return this->m_command_lists_per_frame;
}

RenderInterface_Vulkan::CommandListRing::CommandBufferPerFrame::CommandBufferPerFrame(void) : m_used_calls{}, m_number_per_frame_command_lists{} {}

RenderInterface_Vulkan::CommandListRing::CommandBufferPerFrame::~CommandBufferPerFrame(void) {}

uint32_t RenderInterface_Vulkan::CommandListRing::CommandBufferPerFrame::GetUsedCalls(void) const noexcept
{
	return this->m_used_calls;
}

const Rml::Vector<VkCommandPool>& RenderInterface_Vulkan::CommandListRing::CommandBufferPerFrame::GetCommandPools(void) const noexcept
{
	return this->m_command_pools;
}

const Rml::Vector<VkCommandBuffer>& RenderInterface_Vulkan::CommandListRing::CommandBufferPerFrame::GetCommandBuffers(void) const noexcept
{
	return this->m_command_buffers;
}

void RenderInterface_Vulkan::CommandListRing::CommandBufferPerFrame::AddCommandPools(VkCommandPool p_command_pool) noexcept
{
	VK_ASSERT(p_command_pool, "you must pass a valid pointer of VkCommandPool");

	this->m_command_pools.push_back(p_command_pool);
}

void RenderInterface_Vulkan::CommandListRing::CommandBufferPerFrame::AddCommandBuffers(VkCommandBuffer p_buffer) noexcept
{
	VK_ASSERT(p_buffer, "you must pass a valid pointer of VkCommandBuffer");

	this->m_command_buffers.push_back(p_buffer);
}

void RenderInterface_Vulkan::CommandListRing::CommandBufferPerFrame::SetUsedCalls(uint32_t number) noexcept
{
	this->m_used_calls = number;
}

void RenderInterface_Vulkan::CommandListRing::CommandBufferPerFrame::SetCountCommandListsAndPools(uint32_t number) noexcept
{
	this->m_number_per_frame_command_lists = number;
}

RenderInterface_Vulkan::MemoryPool::MemoryPool(void) :
	m_memory_gpu_data_total{}, m_memory_total_size{}, m_memory_gpu_data_one_object{}, m_min_alignment_for_uniform_buffer{}, m_p_data{}, m_p_buffer{},
	m_p_physical_device_current_properties{}, m_p_buffer_alloc{}, m_p_device{}, m_p_vk_allocator{}, m_p_block{}
{}

RenderInterface_Vulkan::MemoryPool::~MemoryPool(void) {}

void RenderInterface_Vulkan::MemoryPool::Initialize(VkPhysicalDeviceProperties* p_props, VmaAllocator p_allocator, VkDevice p_device,
	const MemoryPoolCreateInfo& info_creation) noexcept
{
	VK_ASSERT(p_device, "you must pass a valid VkDevice");
	VK_ASSERT(p_allocator, "you must pass a valid VmaAllocator");
	VK_ASSERT(p_props, "must be valid!");

	VK_ASSERT(info_creation.m_memory_total_size > 0, "size must be valid");
	VK_ASSERT(info_creation.m_gpu_data_size > 0, "size must be valid");
	VK_ASSERT(info_creation.m_gpu_data_count > 0, "count must be valid");
	VK_ASSERT(info_creation.m_number_of_back_buffers > 0, "can't be!");

	this->m_p_device = p_device;
	this->m_p_vk_allocator = p_allocator;
	this->m_p_physical_device_current_properties = p_props;
	this->m_min_alignment_for_uniform_buffer = this->m_p_physical_device_current_properties->limits.minUniformBufferOffsetAlignment;

#ifdef RMLUI_DEBUG
	Rml::Log::Message(Rml::Log::LT_DEBUG, "[Vulkan][Debug] the alignment for uniform buffer is: %zu", this->m_min_alignment_for_uniform_buffer);
#endif

	this->m_memory_total_size = RenderInterface_Vulkan::AlignUp<VkDeviceSize>(static_cast<VkDeviceSize>(info_creation.m_memory_total_size),
		this->m_min_alignment_for_uniform_buffer);

	this->m_memory_gpu_data_one_object = RenderInterface_Vulkan::AlignUp<VkDeviceSize>(static_cast<VkDeviceSize>(info_creation.m_gpu_data_size),
		this->m_min_alignment_for_uniform_buffer);

	this->m_memory_gpu_data_total = this->m_memory_gpu_data_one_object * info_creation.m_gpu_data_count;

	VkBufferCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	info.size = this->m_memory_total_size;

	VmaAllocationCreateInfo info_alloc = {};

	auto p_commentary = "our pool buffer that manages all memory in vulkan (dynamic)";

	info_alloc.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
	info_alloc.flags = VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
	info_alloc.pUserData = const_cast<char*>(p_commentary);

	VmaAllocationInfo info_stats = {};

	auto status = vmaCreateBuffer(this->m_p_vk_allocator, &info, &info_alloc, &this->m_p_buffer, &this->m_p_buffer_alloc, &info_stats);

	VK_ASSERT(status == VkResult::VK_SUCCESS, "failed to vmaCreateBuffer");

	VmaVirtualBlockCreateInfo info_virtual_block = {};
	info_virtual_block.size = this->m_memory_total_size;

	status = vmaCreateVirtualBlock(&info_virtual_block, &this->m_p_block);

	VK_ASSERT(status == VkResult::VK_SUCCESS, "failed to vmaCreateVirtualBlock");

#ifdef RMLUI_DEBUG
	Rml::Log::Message(Rml::Log::LT_DEBUG, "[Vulkan][Debug] allocated memory for pool: %zu Mbs",
		RenderInterface_Vulkan::TranslateBytesToMegaBytes(info_stats.size));
#endif

	status = vmaMapMemory(this->m_p_vk_allocator, this->m_p_buffer_alloc, (void**)&this->m_p_data);

	VK_ASSERT(status == VkResult::VK_SUCCESS, "failed to vmaMapMemory");
}

void RenderInterface_Vulkan::MemoryPool::Shutdown(void) noexcept
{
	VK_ASSERT(this->m_p_vk_allocator, "you must have a valid VmaAllocator");
	VK_ASSERT(this->m_p_buffer, "you must allocate VkBuffer for deleting");
	VK_ASSERT(this->m_p_buffer_alloc, "you must allocate VmaAllocation for deleting");

#ifdef RMLUI_DEBUG
	Rml::Log::Message(Rml::Log::LT_DEBUG, "[Vulkan][Debug] Destroyed buffer with memory [%zu] Mbs",
		RenderInterface_Vulkan::TranslateBytesToMegaBytes(this->m_memory_total_size));
#endif

	vmaUnmapMemory(this->m_p_vk_allocator, this->m_p_buffer_alloc);
	vmaDestroyVirtualBlock(this->m_p_block);
	vmaDestroyBuffer(this->m_p_vk_allocator, this->m_p_buffer, this->m_p_buffer_alloc);
}

bool RenderInterface_Vulkan::MemoryPool::Alloc_GeneralBuffer(VkDeviceSize size, void** p_data, VkDescriptorBufferInfo* p_out,
	VmaVirtualAllocation* p_alloc) noexcept
{
	VK_ASSERT(p_out, "you must pass a valid pointer");
	VK_ASSERT(this->m_p_buffer, "you must have a valid VkBuffer");

	VK_ASSERT(*p_alloc == nullptr,
		"you can't pass a VALID object, because it is for initialization. So it means you passed the already allocated "
		"VmaVirtualAllocation and it means you did something wrong, like you wanted to allocate into the same object...");

	size = RenderInterface_Vulkan::AlignUp<VkDeviceSize>(static_cast<VkDeviceSize>(size), this->m_min_alignment_for_uniform_buffer);

	VkDeviceSize offset_memory{};

	VmaVirtualAllocationCreateInfo info = {};
	info.size = size;
	info.alignment = this->m_min_alignment_for_uniform_buffer;

	auto status = vmaVirtualAllocate(this->m_p_block, &info, p_alloc, &offset_memory);

	VK_ASSERT(status == VkResult::VK_SUCCESS, "failed to vmaVirtualAllocate");

	*p_data = (void*)(this->m_p_data + offset_memory);

	p_out->buffer = this->m_p_buffer;
	p_out->offset = offset_memory;
	p_out->range = size;

	return true;
}

bool RenderInterface_Vulkan::MemoryPool::Alloc_VertexBuffer(uint32_t number_of_elements, uint32_t stride_in_bytes, void** p_data,
	VkDescriptorBufferInfo* p_out, VmaVirtualAllocation* p_alloc) noexcept
{
	return this->Alloc_GeneralBuffer(number_of_elements * stride_in_bytes, p_data, p_out, p_alloc);
}

bool RenderInterface_Vulkan::MemoryPool::Alloc_IndexBuffer(uint32_t number_of_elements, uint32_t stride_in_bytes, void** p_data,
	VkDescriptorBufferInfo* p_out, VmaVirtualAllocation* p_alloc) noexcept
{
	return this->Alloc_GeneralBuffer(number_of_elements * stride_in_bytes, p_data, p_out, p_alloc);
}

void RenderInterface_Vulkan::MemoryPool::SetDescriptorSet(uint32_t binding_index, uint32_t size, VkDescriptorType descriptor_type,
	VkDescriptorSet p_set) noexcept
{
	VK_ASSERT(this->m_p_device, "you must have a valid VkDevice here");
	VK_ASSERT(p_set, "you must have a valid VkDescriptorSet here");
	VK_ASSERT(this->m_p_buffer, "you must have a valid VkBuffer here");

	VkDescriptorBufferInfo info = {};

	info.buffer = this->m_p_buffer;
	info.offset = 0;
	info.range = size;

	VkWriteDescriptorSet info_write = {};

	info_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	info_write.pNext = nullptr;
	info_write.dstSet = p_set;
	info_write.descriptorCount = 1;
	info_write.descriptorType = descriptor_type;
	info_write.dstArrayElement = 0;
	info_write.dstBinding = binding_index;
	info_write.pBufferInfo = &info;

	vkUpdateDescriptorSets(this->m_p_device, 1, &info_write, 0, nullptr);
}

void RenderInterface_Vulkan::MemoryPool::SetDescriptorSet(uint32_t binding_index, VkDescriptorBufferInfo* p_info, VkDescriptorType descriptor_type,
	VkDescriptorSet p_set) noexcept
{
	VK_ASSERT(this->m_p_device, "you must have a valid VkDevice here");
	VK_ASSERT(p_set, "you must have a valid VkDescriptorSet here");
	VK_ASSERT(this->m_p_buffer, "you must have a valid VkBuffer here");
	VK_ASSERT(p_info, "must be valid pointer");

	VkWriteDescriptorSet info_write = {};

	info_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	info_write.pNext = nullptr;
	info_write.dstSet = p_set;
	info_write.descriptorCount = 1;
	info_write.descriptorType = descriptor_type;
	info_write.dstArrayElement = 0;
	info_write.dstBinding = binding_index;
	info_write.pBufferInfo = p_info;

	vkUpdateDescriptorSets(this->m_p_device, 1, &info_write, 0, nullptr);
}

void RenderInterface_Vulkan::MemoryPool::SetDescriptorSet(uint32_t binding_index, VkSampler p_sampler, VkImageLayout layout, VkImageView p_view,
	VkDescriptorType descriptor_type, VkDescriptorSet p_set) noexcept
{
	VK_ASSERT(this->m_p_device, "you must have a valid VkDevice here");
	VK_ASSERT(p_set, "you must have a valid VkDescriptorSet here");
	VK_ASSERT(this->m_p_buffer, "you must have a valid VkBuffer here");
	VK_ASSERT(p_view, "you must have a valid VkImageView");
	VK_ASSERT(p_sampler, "you must have a valid VkSampler here");

	VkDescriptorImageInfo info = {};

	info.imageLayout = layout;
	info.imageView = p_view;
	info.sampler = p_sampler;

	VkWriteDescriptorSet info_write = {};

	info_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	info_write.pNext = nullptr;
	info_write.dstSet = p_set;
	info_write.descriptorCount = 1;
	info_write.descriptorType = descriptor_type;
	info_write.dstArrayElement = 0;
	info_write.dstBinding = binding_index;
	info_write.pImageInfo = &info;

	vkUpdateDescriptorSets(this->m_p_device, 1, &info_write, 0, nullptr);
}

void RenderInterface_Vulkan::MemoryPool::Free_GeometryHandle(geometry_handle_t* p_valid_geometry_handle) noexcept
{
	VK_ASSERT(p_valid_geometry_handle, "you must pass a VALID pointer to geometry_handle_t, otherwise something is wrong and debug your code");
	VK_ASSERT(p_valid_geometry_handle->m_p_vertex_allocation, "you must have a VALID pointer of VmaAllocation for vertex buffer");
	VK_ASSERT(p_valid_geometry_handle->m_p_index_allocation, "you must have a VALID pointer of VmaAllocation for index buffer");
	VK_ASSERT(p_valid_geometry_handle->m_p_shader_allocation,
		"you must have a VALID pointer of VmaAllocation for shader operations (like uniforms and etc)");
	VK_ASSERT(this->m_p_block, "you have to allocate the virtual block before do this operation...");

	vmaVirtualFree(this->m_p_block, p_valid_geometry_handle->m_p_vertex_allocation);
	vmaVirtualFree(this->m_p_block, p_valid_geometry_handle->m_p_index_allocation);
	vmaVirtualFree(this->m_p_block, p_valid_geometry_handle->m_p_shader_allocation);

	p_valid_geometry_handle->m_p_vertex_allocation = nullptr;
	p_valid_geometry_handle->m_p_shader_allocation = nullptr;
	p_valid_geometry_handle->m_p_index_allocation = nullptr;
	p_valid_geometry_handle->m_p_texture = nullptr;
	p_valid_geometry_handle->m_descriptor_id = 0;
	p_valid_geometry_handle->m_is_cached = false;
	p_valid_geometry_handle->m_is_has_texture = false;
	p_valid_geometry_handle->m_num_indices = 0;
}

void RenderInterface_Vulkan::MemoryPool::Free_GeometryHandle_ShaderDataOnly(geometry_handle_t* p_valid_geometry_handle) noexcept
{
	VK_ASSERT(p_valid_geometry_handle, "you must pass a VALID pointer to geometry_handle_t, otherwise something is wrong and debug your code");
	VK_ASSERT(p_valid_geometry_handle->m_p_vertex_allocation, "you must have a VALID pointer of VmaAllocation for vertex buffer");
	VK_ASSERT(p_valid_geometry_handle->m_p_index_allocation, "you must have a VALID pointer of VmaAllocation for index buffer");
	VK_ASSERT(p_valid_geometry_handle->m_p_shader_allocation,
		"you must have a VALID pointer of VmaAllocation for shader operations (like uniforms and etc)");
	VK_ASSERT(this->m_p_block, "you have to allocate the virtual block before do this operation...");

	vmaVirtualFree(this->m_p_block, p_valid_geometry_handle->m_p_shader_allocation);
	p_valid_geometry_handle->m_p_shader_allocation = nullptr;
}

#ifdef __clang__
	#pragma clang diagnostic pop
#endif

RMLUI_DISABLE_ALL_COMPILER_WARNINGS_PUSH

#define GLAD_VULKAN_IMPLEMENTATION
#include "RmlUi_Vulkan/vulkan.h"

#define VMA_IMPLEMENTATION
#include "RmlUi_Vulkan/spirv_reflect.cpp"
#include "RmlUi_Vulkan/vk_mem_alloc.h"

RMLUI_DISABLE_ALL_COMPILER_WARNINGS_POP
