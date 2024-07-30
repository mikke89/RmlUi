/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019-2023 The RmlUi Team, and contributors
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

#include "RmlUi_Renderer_VK.h"
#include "RmlUi_Vulkan/ShadersCompiledSPV.h"
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Core/Log.h>
#include <RmlUi/Core/Math.h>
#include <RmlUi/Core/Platform.h>
#include <RmlUi/Core/Profiling.h>
#include <algorithm>
#include <string.h>

// AlignUp(314, 256) = 512
template <typename T>
static T AlignUp(T val, T alignment)
{
	return (val + alignment - (T)1) & ~(alignment - (T)1);
}

VkValidationFeaturesEXT debug_validation_features_ext = {};
VkValidationFeatureEnableEXT debug_validation_features_ext_requested[] = {
	VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
	VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT,
	VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
};

#ifdef RMLUI_VK_DEBUG
static Rml::String FormatByteSize(VkDeviceSize size) noexcept
{
	constexpr VkDeviceSize K = VkDeviceSize(1024);
	if (size < K)
		return Rml::CreateString("%zu B", size);
	else if (size < K * K)
		return Rml::CreateString("%g KB", double(size) / double(K));
	return Rml::CreateString("%g MB", double(size) / double(K * K));
}

static VKAPI_ATTR VkBool32 VKAPI_CALL MyDebugReportCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severityFlags,
	VkDebugUtilsMessageTypeFlagsEXT /*messageTypeFlags*/, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* /*pUserData*/)
{
	if (severityFlags & VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
	{
		return VK_FALSE;
	}

	#ifdef RMLUI_PLATFORM_WIN32
	if (severityFlags & VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
	{
		// some logs are not passed to our UI, because of early calling for explicity I put native log output
		OutputDebugString(TEXT("\n"));
		OutputDebugStringA(pCallbackData->pMessage);
	}
	#endif

	Rml::Log::Message(Rml::Log::LT_ERROR, "[Vulkan][VALIDATION] %s ", pCallbackData->pMessage);

	return VK_FALSE;
}
#endif

RenderInterface_VK::RenderInterface_VK() :
	m_is_transform_enabled{false}, m_is_apply_to_regular_geometry_stencil{false}, m_is_use_scissor_specified{false}, m_is_use_stencil_pipeline{false},
	m_width{}, m_height{}, m_queue_index_present{}, m_queue_index_graphics{}, m_queue_index_compute{}, m_semaphore_index{},
	m_semaphore_index_previous{}, m_image_index{}, m_p_instance{}, m_p_device{}, m_p_physical_device{}, m_p_surface{}, m_p_swapchain{},
	m_p_allocator{}, m_p_current_command_buffer{}, m_p_descriptor_set_layout_vertex_transform{}, m_p_descriptor_set_layout_texture{},
	m_p_pipeline_layout{}, m_p_pipeline_with_textures{}, m_p_pipeline_without_textures{},
	m_p_pipeline_stencil_for_region_where_geometry_will_be_drawn{}, m_p_pipeline_stencil_for_regular_geometry_that_applied_to_region_with_textures{},
	m_p_pipeline_stencil_for_regular_geometry_that_applied_to_region_without_textures{}, m_p_descriptor_set{}, m_p_render_pass{},
	m_p_sampler_linear{}, m_scissor{}, m_scissor_original{}, m_viewport{}, m_p_queue_present{}, m_p_queue_graphics{}, m_p_queue_compute{},
#ifdef RMLUI_VK_DEBUG
	m_debug_messenger{},
#endif
	m_swapchain_format{}, m_texture_depthstencil{}, m_pending_for_deletion_textures_by_frames{}
{}

RenderInterface_VK::~RenderInterface_VK() {}

Rml::CompiledGeometryHandle RenderInterface_VK::CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices)
{
	RMLUI_ZoneScopedN("Vulkan - CompileGeometry");

	VkDescriptorSet p_current_descriptor_set = nullptr;
	p_current_descriptor_set = m_p_descriptor_set;

	RMLUI_VK_ASSERTMSG(p_current_descriptor_set,
		"you can't have here an invalid pointer of VkDescriptorSet. Two reason might be. 1. - you didn't allocate it "
		"at all or 2. - Somehing is wrong with allocation and somehow it was corrupted by something.");

	auto* p_geometry_handle = new geometry_handle_t{};

	uint32_t* pCopyDataToBuffer = nullptr;
	const void* pData = reinterpret_cast<const void*>(vertices.data());

	bool status = m_memory_pool.Alloc_VertexBuffer((uint32_t)vertices.size(), sizeof(Rml::Vertex), reinterpret_cast<void**>(&pCopyDataToBuffer),
		&p_geometry_handle->m_p_vertex, &p_geometry_handle->m_p_vertex_allocation);
	RMLUI_VK_ASSERTMSG(status, "failed to AllocVertexBuffer");

	memcpy(pCopyDataToBuffer, pData, sizeof(Rml::Vertex) * vertices.size());

	status = m_memory_pool.Alloc_IndexBuffer((uint32_t)indices.size(), sizeof(int), reinterpret_cast<void**>(&pCopyDataToBuffer),
		&p_geometry_handle->m_p_index, &p_geometry_handle->m_p_index_allocation);
	RMLUI_VK_ASSERTMSG(status, "failed to AllocIndexBuffer");

	memcpy(pCopyDataToBuffer, indices.data(), sizeof(int) * indices.size());

	p_geometry_handle->m_num_indices = (int)indices.size();

	return Rml::CompiledGeometryHandle(p_geometry_handle);
}

void RenderInterface_VK::RenderGeometry(Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation, Rml::TextureHandle texture)
{
	RMLUI_ZoneScopedN("Vulkan - RenderCompiledGeometry");

	if (m_p_current_command_buffer == nullptr)
		return;

	RMLUI_VK_ASSERTMSG(m_p_current_command_buffer, "must be valid otherwise you can't render now!!! (can't be)");

	texture_data_t* p_texture = reinterpret_cast<texture_data_t*>(texture);

	VkDescriptorImageInfo info_descriptor_image = {};
	if (p_texture && p_texture->m_p_vk_descriptor_set == nullptr)
	{
		VkDescriptorSet p_texture_set = nullptr;
		m_manager_descriptors.Alloc_Descriptor(m_p_device, &m_p_descriptor_set_layout_texture, &p_texture_set);

		info_descriptor_image.imageView = p_texture->m_p_vk_image_view;
		info_descriptor_image.sampler = p_texture->m_p_vk_sampler;
		info_descriptor_image.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkWriteDescriptorSet info_write = {};

		info_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		info_write.dstSet = p_texture_set;
		info_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		info_write.dstBinding = 2;
		info_write.pImageInfo = &info_descriptor_image;
		info_write.descriptorCount = 1;

		vkUpdateDescriptorSets(m_p_device, 1, &info_write, 0, nullptr);
		p_texture->m_p_vk_descriptor_set = p_texture_set;
	}

	geometry_handle_t* p_casted_compiled_geometry = reinterpret_cast<geometry_handle_t*>(geometry);

	m_user_data_for_vertex_shader.m_translate = translation;

	VkDescriptorSet p_current_descriptor_set = nullptr;
	p_current_descriptor_set = m_p_descriptor_set;

	RMLUI_VK_ASSERTMSG(p_current_descriptor_set,
		"you can't have here an invalid pointer of VkDescriptorSet. Two reason might be. 1. - you didn't allocate it "
		"at all or 2. - Somehing is wrong with allocation and somehow it was corrupted by something.");

	shader_vertex_user_data_t* p_data = nullptr;

	if (p_casted_compiled_geometry->m_p_shader_allocation == nullptr)
	{
		// it means it was freed in ReleaseCompiledGeometry method
		bool status = m_memory_pool.Alloc_GeneralBuffer(sizeof(m_user_data_for_vertex_shader), reinterpret_cast<void**>(&p_data),
			&p_casted_compiled_geometry->m_p_shader, &p_casted_compiled_geometry->m_p_shader_allocation);
		RMLUI_VK_ASSERTMSG(status, "failed to allocate VkDescriptorBufferInfo for uniform data to shaders");
	}
	else
	{
		// it means our state is dirty and we need to update data, but it is not right in terms of architecture, for real better experience would
		// be great to free all "compiled" geometries and "re-build" them in one general way, but here I got only three callings for
		// font-face-layer textures (load_document example) and that shit. So better to think how to make it right, if it is fine okay, if it is
		// not okay and like we really expect that ReleaseCompiledGeometry for all objects that needs to be rebuilt so better to implement that,
		// but still it is a big architectural thing (or at least you need to do something big commits here to implement a such feature), so my
		// implementation doesn't break anything what we had, but still it looks strange. If I get callings for releasing maybe I need to use it
		// for all objects not separately????? Otherwise it is better to provide method for resizing (or some kind of "resizing" callback) for
		// recalculating all geometry IDK, so it means you pass the existed geometry that wasn't pass to ReleaseCompiledGeometry, but from another
		// hand you need to re-build compiled geometry again so we have two kinds of geometry one is compiled and never changes and one is dynamic
		// and it goes through pipeline InitializationOfProgram...->Compile->Render->Release->Compile->Render->Release...

		m_memory_pool.Free_GeometryHandle_ShaderDataOnly(p_casted_compiled_geometry);
		bool status = m_memory_pool.Alloc_GeneralBuffer(sizeof(m_user_data_for_vertex_shader), reinterpret_cast<void**>(&p_data),
			&p_casted_compiled_geometry->m_p_shader, &p_casted_compiled_geometry->m_p_shader_allocation);
		RMLUI_VK_ASSERTMSG(status, "failed to allocate VkDescriptorBufferInfo for uniform data to shaders");
	}

	if (p_data)
	{
		p_data->m_transform = m_user_data_for_vertex_shader.m_transform;
		p_data->m_translate = m_user_data_for_vertex_shader.m_translate;
	}
	else
	{
		RMLUI_VK_ASSERTMSG(p_data, "you can't reach this zone, it means something bad");
	}

	const uint32_t pDescriptorOffsets = static_cast<uint32_t>(p_casted_compiled_geometry->m_p_shader.offset);

	VkDescriptorSet p_texture_descriptor_set = nullptr;

	if (p_texture)
	{
		p_texture_descriptor_set = p_texture->m_p_vk_descriptor_set;
	}

	VkDescriptorSet p_sets[] = {p_current_descriptor_set, p_texture_descriptor_set};
	int real_size_of_sets = 2;

	if (p_texture == nullptr)
		real_size_of_sets = 1;

	vkCmdBindDescriptorSets(m_p_current_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_p_pipeline_layout, 0, real_size_of_sets, p_sets, 1,
		&pDescriptorOffsets);

	if (m_is_use_stencil_pipeline)
	{
		vkCmdBindPipeline(m_p_current_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_p_pipeline_stencil_for_region_where_geometry_will_be_drawn);
	}
	else
	{
		if (p_texture)
		{
			if (m_is_apply_to_regular_geometry_stencil)
			{
				vkCmdBindPipeline(m_p_current_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
					m_p_pipeline_stencil_for_regular_geometry_that_applied_to_region_with_textures);
			}
			else
			{
				vkCmdBindPipeline(m_p_current_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_p_pipeline_with_textures);
			}
		}
		else
		{
			if (m_is_apply_to_regular_geometry_stencil)
			{
				vkCmdBindPipeline(m_p_current_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
					m_p_pipeline_stencil_for_regular_geometry_that_applied_to_region_without_textures);
			}
			else
			{
				vkCmdBindPipeline(m_p_current_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_p_pipeline_without_textures);
			}
		}
	}

	vkCmdBindVertexBuffers(m_p_current_command_buffer, 0, 1, &p_casted_compiled_geometry->m_p_vertex.buffer,
		&p_casted_compiled_geometry->m_p_vertex.offset);

	vkCmdBindIndexBuffer(m_p_current_command_buffer, p_casted_compiled_geometry->m_p_index.buffer, p_casted_compiled_geometry->m_p_index.offset,
		VK_INDEX_TYPE_UINT32);

	vkCmdDrawIndexed(m_p_current_command_buffer, p_casted_compiled_geometry->m_num_indices, 1, 0, 0, 0);
}

void RenderInterface_VK::ReleaseGeometry(Rml::CompiledGeometryHandle geometry)
{
	RMLUI_ZoneScopedN("Vulkan - ReleaseCompiledGeometry");

	geometry_handle_t* p_casted_geometry = reinterpret_cast<geometry_handle_t*>(geometry);

	m_pending_for_deletion_geometries.push_back(p_casted_geometry);
}

void RenderInterface_VK::EnableScissorRegion(bool enable)
{
	if (m_p_current_command_buffer == nullptr)
		return;

	if (m_is_transform_enabled)
	{
		m_is_apply_to_regular_geometry_stencil = true;
	}

	m_is_use_scissor_specified = enable;

	if (m_is_use_scissor_specified == false)
	{
		m_is_apply_to_regular_geometry_stencil = false;
		vkCmdSetScissor(m_p_current_command_buffer, 0, 1, &m_scissor_original);
	}
}

void RenderInterface_VK::SetScissorRegion(Rml::Rectanglei region)
{
	if (m_is_use_scissor_specified)
	{
		if (m_is_transform_enabled)
		{
			Rml::Vertex vertices[4];

			vertices[0].position = Rml::Vector2f(region.TopLeft());
			vertices[1].position = Rml::Vector2f(region.TopRight());
			vertices[2].position = Rml::Vector2f(region.BottomRight());
			vertices[3].position = Rml::Vector2f(region.BottomLeft());

			int indices[6] = {0, 2, 1, 0, 3, 2};

			m_is_use_stencil_pipeline = true;

#ifdef RMLUI_DEBUG
			VkDebugUtilsLabelEXT info{};
			info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
			info.color[0] = 1.0f;
			info.color[1] = 1.0f;
			info.color[2] = 0.0f;
			info.color[3] = 1.0f;
			info.pLabelName = "SetScissorRegion (generated region)";

			vkCmdInsertDebugUtilsLabelEXT(m_p_current_command_buffer, &info);
#endif

			VkClearDepthStencilValue info_clear_color{};

			info_clear_color.depth = 1.0f;
			info_clear_color.stencil = 0;

			VkClearAttachment clear_attachment = {};
			clear_attachment.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
			clear_attachment.clearValue.depthStencil = info_clear_color;
			clear_attachment.colorAttachment = 1;

			VkClearRect clear_rect = {};
			clear_rect.layerCount = 1;
			clear_rect.rect.extent.width = m_width;
			clear_rect.rect.extent.height = m_height;

			vkCmdClearAttachments(m_p_current_command_buffer, 1, &clear_attachment, 1, &clear_rect);

			if (Rml::CompiledGeometryHandle handle = CompileGeometry({vertices, 4}, {indices, 6}))
			{
				RenderGeometry(handle, {}, {});
				ReleaseGeometry(handle);
			}

			m_is_use_stencil_pipeline = false;

			m_is_apply_to_regular_geometry_stencil = true;
		}
		else
		{
			m_scissor.extent.width = region.Width();
			m_scissor.extent.height = region.Height();
			m_scissor.offset.x = Rml::Math::Clamp(region.Left(), 0, m_width);
			m_scissor.offset.y = Rml::Math::Clamp(region.Top(), 0, m_height);

#ifdef RMLUI_DEBUG
			VkDebugUtilsLabelEXT info{};
			info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
			info.color[0] = 1.0f;
			info.color[1] = 0.0f;
			info.color[2] = 0.0f;
			info.color[3] = 1.0f;
			info.pLabelName = "SetScissorRegion (offset)";

			vkCmdInsertDebugUtilsLabelEXT(m_p_current_command_buffer, &info);
#endif

			vkCmdSetScissor(m_p_current_command_buffer, 0, 1, &m_scissor);
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

Rml::TextureHandle RenderInterface_VK::LoadTexture(Rml::Vector2i& texture_dimensions, const Rml::String& source)
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

	if (buffer_size <= sizeof(TGAHeader))
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "Texture file size is smaller than TGAHeader, file is not a valid TGA image.");
		file_interface->Close(file_handle);
		return false;
	}

	using Rml::byte;
	Rml::UniquePtr<byte[]> buffer(new byte[buffer_size]);
	file_interface->Read(buffer.get(), buffer_size, file_handle);
	file_interface->Close(file_handle);

	TGAHeader header;
	memcpy(&header, buffer.get(), sizeof(TGAHeader));

	int color_mode = header.bitsPerPixel / 8;
	const size_t image_size = header.width * header.height * 4; // We always make 32bit textures

	if (header.dataType != 2)
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "Only 24/32bit uncompressed TGAs are supported.");
		return false;
	}

	// Ensure we have at least 3 colors
	if (color_mode < 3)
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "Only 24 and 32bit textures are supported.");
		return false;
	}

	const byte* image_src = buffer.get() + sizeof(TGAHeader);
	Rml::UniquePtr<byte[]> image_dest_buffer(new byte[image_size]);
	byte* image_dest = image_dest_buffer.get();

	// Targa is BGR, swap to RGB, flip Y axis, and convert to premultiplied alpha.
	for (long y = 0; y < header.height; y++)
	{
		long read_index = y * header.width * color_mode;
		long write_index = ((header.imageDescriptor & 32) != 0) ? read_index : (header.height - y - 1) * header.width * 4;
		for (long x = 0; x < header.width; x++)
		{
			image_dest[write_index] = image_src[read_index + 2];
			image_dest[write_index + 1] = image_src[read_index + 1];
			image_dest[write_index + 2] = image_src[read_index];
			if (color_mode == 4)
			{
				const byte alpha = image_src[read_index + 3];
				for (size_t j = 0; j < 3; j++)
					image_dest[write_index + j] = byte((image_dest[write_index + j] * alpha) / 255);
				image_dest[write_index + 3] = alpha;
			}
			else
				image_dest[write_index + 3] = 255;

			write_index += 4;
			read_index += color_mode;
		}
	}

	texture_dimensions.x = header.width;
	texture_dimensions.y = header.height;

	return GenerateTexture({image_dest, image_size}, texture_dimensions);
}

Rml::TextureHandle RenderInterface_VK::GenerateTexture(Rml::Span<const Rml::byte> source_data, Rml::Vector2i source_dimensions)
{
	Rml::String source_name = "generated-texture";
	return CreateTexture(source_data, source_dimensions, source_name);
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
Rml::TextureHandle RenderInterface_VK::CreateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i dimensions, const Rml::String& name)
{
	RMLUI_ZoneScopedN("Vulkan - GenerateTexture");

	RMLUI_VK_ASSERTMSG(!source.empty(), "you pushed not valid data for copying to buffer");
	RMLUI_VK_ASSERTMSG(m_p_allocator, "you have to initialize Vma Allocator for this method");
	(void)name;

	int width = dimensions.x;
	int height = dimensions.y;

	RMLUI_VK_ASSERTMSG(width, "invalid width");
	RMLUI_VK_ASSERTMSG(height, "invalid height");

	VkDeviceSize image_size = source.size();
	VkFormat format = VkFormat::VK_FORMAT_R8G8B8A8_UNORM;

	buffer_data_t cpu_buffer = CreateResource_StagingBuffer(image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

	void* data;
	vmaMapMemory(m_p_allocator, cpu_buffer.m_p_vma_allocation, &data);
	memcpy(data, source.data(), static_cast<size_t>(image_size));
	vmaUnmapMemory(m_p_allocator, cpu_buffer.m_p_vma_allocation);

	VkExtent3D extent_image = {};
	extent_image.width = static_cast<uint32_t>(width);
	extent_image.height = static_cast<uint32_t>(height);
	extent_image.depth = 1;

	auto* p_texture = new texture_data_t{};

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
	VkResult status = vmaCreateImage(m_p_allocator, &info, &info_allocation, &p_image, &p_allocation, &info_stats);
	RMLUI_VK_ASSERTMSG(status == VkResult::VK_SUCCESS, "failed to vmaCreateImage");

#ifdef RMLUI_VK_DEBUG
	Rml::Log::Message(Rml::Log::LT_DEBUG, "Created texture '%s' [%dx%d, %s]", name.c_str(), dimensions.x, dimensions.y,
		FormatByteSize(info_stats.size).c_str());
#endif

	p_texture->m_p_vk_image = p_image;
	p_texture->m_p_vma_allocation = p_allocation;

#ifdef RMLUI_VK_DEBUG
	vmaSetAllocationName(m_p_allocator, p_allocation, name.c_str());
#endif

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
	m_upload_manager.UploadToGPU([p_image, extent_image, cpu_buffer](VkCommandBuffer p_cmd) {
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

		vkCmdPipelineBarrier(p_cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &info_barrier);

		VkBufferImageCopy region = {};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;

		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageExtent = extent_image;

		vkCmdCopyBufferToImage(p_cmd, cpu_buffer.m_p_vk_buffer, p_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

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

	DestroyResource_StagingBuffer(cpu_buffer);

	VkImageViewCreateInfo info_image_view = {};
	info_image_view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	info_image_view.pNext = nullptr;
	info_image_view.image = p_texture->m_p_vk_image;
	info_image_view.viewType = VK_IMAGE_VIEW_TYPE_2D;
	info_image_view.format = format;
	info_image_view.subresourceRange.baseMipLevel = 0;
	info_image_view.subresourceRange.levelCount = 1;
	info_image_view.subresourceRange.baseArrayLayer = 0;
	info_image_view.subresourceRange.layerCount = 1;
	info_image_view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

	VkImageView p_image_view = nullptr;
	status = vkCreateImageView(m_p_device, &info_image_view, nullptr, &p_image_view);
	RMLUI_VK_ASSERTMSG(status == VkResult::VK_SUCCESS, "failed to vkCreateImageView");

	p_texture->m_p_vk_image_view = p_image_view;
	p_texture->m_p_vk_sampler = m_p_sampler_linear;

	return reinterpret_cast<Rml::TextureHandle>(p_texture);
}

void RenderInterface_VK::ReleaseTexture(Rml::TextureHandle texture_handle)
{
	texture_data_t* p_texture = reinterpret_cast<texture_data_t*>(texture_handle);

	if (p_texture)
	{
		m_pending_for_deletion_textures_by_frames[m_semaphore_index_previous].push_back(p_texture);
	}
}

void RenderInterface_VK::SetTransform(const Rml::Matrix4f* transform)
{
	m_is_transform_enabled = !!(transform);
	m_user_data_for_vertex_shader.m_transform = m_projection * (transform ? *transform : Rml::Matrix4f::Identity());
}

void RenderInterface_VK::BeginFrame()
{
	Wait();

	Update_PendingForDeletion_Textures_By_Frames();
	Update_PendingForDeletion_Geometries();

	m_command_buffer_ring.OnBeginFrame();
	m_p_current_command_buffer = m_command_buffer_ring.GetCommandBufferForActiveFrame(CommandBufferName::Primary);

	VkCommandBufferBeginInfo info = {};

	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	info.pInheritanceInfo = nullptr;
	info.pNext = nullptr;
	info.flags = VkCommandBufferUsageFlagBits::VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	auto status = vkBeginCommandBuffer(m_p_current_command_buffer, &info);

	RMLUI_VK_ASSERTMSG(status == VkResult::VK_SUCCESS, "failed to vkBeginCommandBuffer");

	VkClearValue for_filling_back_buffer_color;
	VkClearValue for_stencil_depth;

	for_stencil_depth.depthStencil = {1.0f, 0};
	for_filling_back_buffer_color.color = {{0.0f, 0.0f, 0.0f, 1.0f}};

	const VkClearValue p_color_rt[] = {for_filling_back_buffer_color, for_stencil_depth};

	VkRenderPassBeginInfo info_pass = {};

	info_pass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	info_pass.pNext = nullptr;
	info_pass.renderPass = m_p_render_pass;
	info_pass.framebuffer = m_swapchain_frame_buffers[m_image_index];
	info_pass.pClearValues = p_color_rt;
	info_pass.clearValueCount = 2;
	info_pass.renderArea.offset.x = 0;
	info_pass.renderArea.offset.y = 0;
	info_pass.renderArea.extent.width = m_width;
	info_pass.renderArea.extent.height = m_height;

	vkCmdBeginRenderPass(m_p_current_command_buffer, &info_pass, VkSubpassContents::VK_SUBPASS_CONTENTS_INLINE);
	vkCmdSetViewport(m_p_current_command_buffer, 0, 1, &m_viewport);

	m_is_apply_to_regular_geometry_stencil = false;
}

void RenderInterface_VK::EndFrame()
{
	if (m_p_current_command_buffer == nullptr)
		return;

	vkCmdEndRenderPass(m_p_current_command_buffer);

	auto status = vkEndCommandBuffer(m_p_current_command_buffer);

	RMLUI_VK_ASSERTMSG(status == VkResult::VK_SUCCESS, "failed to vkEndCommandBuffer");

	Submit();
	Present();

	m_p_current_command_buffer = nullptr;
}

void RenderInterface_VK::SetViewport(int width, int height)
{
	auto status = vkDeviceWaitIdle(m_p_device);
	RMLUI_VK_ASSERTMSG(status == VkResult::VK_SUCCESS, "failed to vkDeviceWaitIdle");

	if (width > 0 && height > 0)
	{
		m_width = width;
		m_height = height;
	}

	if (m_p_swapchain)
	{
		Destroy_Swapchain();
		DestroyResourcesDependentOnSize();
		m_p_swapchain = {};
	}

	VkExtent2D window_extent = GetValidSurfaceExtent();
	if (window_extent.width == 0 || window_extent.height == 0)
		return;

#ifdef RMLUI_VK_DEBUG
	Rml::Log::Message(Rml::Log::Type::LT_DEBUG, "Rml width: %d height: %d | Vulkan width: %d height: %d", m_width, m_height, window_extent.width,
		window_extent.height);
#endif

	//  we need to sync the data from Vulkan so we can't use native Rml's data about width and height so be careful otherwise we create framebuffer
	//  with Rml's width and height but they're different to what Vulkan determines for our window (e.g. device/swapchain)
	m_width = window_extent.width;
	m_height = window_extent.height;

	Initialize_Swapchain(window_extent);
	CreateResourcesDependentOnSize(window_extent);
}

bool RenderInterface_VK::IsSwapchainValid()
{
	return m_p_swapchain != nullptr;
}

void RenderInterface_VK::RecreateSwapchain()
{
	SetViewport(m_width, m_height);
}

bool RenderInterface_VK::Initialize(Rml::Vector<const char*> required_extensions, CreateSurfaceCallback create_surface_callback)
{
	RMLUI_ZoneScopedN("Vulkan - Initialize");

	int glad_result = 0;
	glad_result = gladLoaderLoadVulkan(VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE);
	RMLUI_VK_ASSERTMSG(glad_result != 0, "Vulkan loader failed - Global functions");

	Initialize_Instance(std::move(required_extensions));

	VkPhysicalDeviceProperties physical_device_properties = {};
	Initialize_PhysicalDevice(physical_device_properties);

	glad_result = gladLoaderLoadVulkan(m_p_instance, m_p_physical_device, VK_NULL_HANDLE);
	RMLUI_VK_ASSERTMSG(glad_result != 0, "Vulkan loader failed - Instance functions");

	Initialize_Surface(create_surface_callback);
	Initialize_QueueIndecies();
	Initialize_Device();

	glad_result = gladLoaderLoadVulkan(m_p_instance, m_p_physical_device, m_p_device);
	RMLUI_VK_ASSERTMSG(glad_result != 0, "Vulkan loader failed - Device functions");

	Initialize_Queues();
	Initialize_SyncPrimitives();
	Initialize_Allocator();
	Initialize_Resources(physical_device_properties);

	return true;
}

void RenderInterface_VK::Shutdown()
{
	RMLUI_ZoneScopedN("Vulkan - Shutdown");

	auto status = vkDeviceWaitIdle(m_p_device);

	RMLUI_VK_ASSERTMSG(status == VkResult::VK_SUCCESS, "you must have a valid status here");

	DestroyResourcesDependentOnSize();
	Destroy_Resources();
	Destroy_Allocator();
	Destroy_SyncPrimitives();
	Destroy_Swapchain();
	Destroy_Surface();
	Destroy_Device();
	Destroy_ReportDebugCallback();
	Destroy_Instance();

	gladLoaderUnloadVulkan();
}

void RenderInterface_VK::Initialize_Instance(Rml::Vector<const char*> required_extensions) noexcept
{
	uint32_t required_version = GetRequiredVersionAndValidateMachine();

	VkApplicationInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	info.pNext = nullptr;
	info.pApplicationName = "RmlUi Shell";
	info.applicationVersion = 50;
	info.pEngineName = "RmlUi";
	info.apiVersion = required_version;

	Rml::Vector<const char*> instance_layer_names;
	Rml::Vector<const char*> instance_extension_names = std::move(required_extensions);
	CreatePropertiesFor_Instance(instance_layer_names, instance_extension_names);

	VkInstanceCreateInfo info_instance = {};
	info_instance.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	info_instance.pNext = &debug_validation_features_ext;
	info_instance.flags = 0;
	info_instance.pApplicationInfo = &info;
	info_instance.enabledExtensionCount = static_cast<uint32_t>(instance_extension_names.size());
	info_instance.ppEnabledExtensionNames = instance_extension_names.data();
	info_instance.enabledLayerCount = static_cast<uint32_t>(instance_layer_names.size());
	info_instance.ppEnabledLayerNames = instance_layer_names.data();

	VkResult status = vkCreateInstance(&info_instance, nullptr, &m_p_instance);
	RMLUI_VK_ASSERTMSG(status == VK_SUCCESS, "failed to vkCreateInstance");

	CreateReportDebugCallback();
}

void RenderInterface_VK::Initialize_Device() noexcept
{
	ExtensionPropertiesList device_extension_properties;
	CreatePropertiesFor_Device(device_extension_properties);

	Rml::Vector<const char*> device_extension_names;
	AddExtensionToDevice(device_extension_names, device_extension_properties, VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	AddExtensionToDevice(device_extension_names, device_extension_properties, VK_EXT_SCALAR_BLOCK_LAYOUT_EXTENSION_NAME);

#ifdef RMLUI_DEBUG
	AddExtensionToDevice(device_extension_names, device_extension_properties, VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

	float queue_priorities[1] = {0.0f};

	VkDeviceQueueCreateInfo info_queue[2] = {};

	info_queue[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	info_queue[0].pNext = nullptr;
	info_queue[0].queueCount = 1;
	info_queue[0].pQueuePriorities = queue_priorities;
	info_queue[0].queueFamilyIndex = m_queue_index_graphics;

	info_queue[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	info_queue[1].pNext = nullptr;
	info_queue[1].queueCount = 1;
	info_queue[1].pQueuePriorities = queue_priorities;
	info_queue[1].queueFamilyIndex = m_queue_index_compute;

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
	info_device.queueCreateInfoCount = m_queue_index_compute != m_queue_index_graphics ? 2 : 1;
	info_device.pQueueCreateInfos = info_queue;
	info_device.enabledExtensionCount = static_cast<uint32_t>(device_extension_names.size());
	info_device.ppEnabledExtensionNames = info_device.enabledExtensionCount ? device_extension_names.data() : nullptr;
	info_device.pEnabledFeatures = nullptr;

	VkResult status = vkCreateDevice(m_p_physical_device, &info_device, nullptr, &m_p_device);

	RMLUI_VK_ASSERTMSG(status == VK_SUCCESS, "failed to vkCreateDevice");
}

void RenderInterface_VK::Initialize_PhysicalDevice(VkPhysicalDeviceProperties& out_physical_device_properties) noexcept
{
	PhysicalDeviceWrapperList physical_devices;
	CollectPhysicalDevices(physical_devices);

	const PhysicalDeviceWrapper* selected_physical_device =
		ChoosePhysicalDevice(physical_devices, VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);

	if (!selected_physical_device)
	{
		Rml::Log::Message(Rml::Log::LT_WARNING, "Failed to pick the discrete gpu, now trying to pick integrated GPU");
		selected_physical_device = ChoosePhysicalDevice(physical_devices, VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU);

		if (!selected_physical_device)
		{
			Rml::Log::Message(Rml::Log::LT_WARNING, "Failed to pick the integrated gpu, now trying to pick the CPU");
			selected_physical_device = ChoosePhysicalDevice(physical_devices, VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_CPU);
		}
	}

	RMLUI_VK_ASSERTMSG(selected_physical_device, "there's no suitable physical device for rendering, abort this application");

	m_p_physical_device = selected_physical_device->m_p_physical_device;
	vkGetPhysicalDeviceProperties(m_p_physical_device, &out_physical_device_properties);

#ifdef RMLUI_VK_DEBUG
	const auto& properties = selected_physical_device->m_physical_device_properties;
	Rml::Log::Message(Rml::Log::LT_DEBUG, "Picked physical device: %s", properties.deviceName);
#endif
}

void RenderInterface_VK::Initialize_Swapchain(VkExtent2D window_extent) noexcept
{
	m_swapchain_format = ChooseSwapchainFormat();

	VkSwapchainCreateInfoKHR info = {};
	info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	info.pNext = nullptr;
	info.surface = m_p_surface;
	info.imageFormat = m_swapchain_format.format;
	info.minImageCount = Choose_SwapchainImageCount();
	info.imageColorSpace = m_swapchain_format.colorSpace;
	info.imageExtent = window_extent;
	info.preTransform = CreatePretransformSwapchain();
	info.compositeAlpha = ChooseSwapchainCompositeAlpha();
	info.imageArrayLayers = 1;
	info.presentMode = GetPresentMode();
	info.oldSwapchain = nullptr;
	info.clipped = true;
	info.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	info.queueFamilyIndexCount = 0;
	info.pQueueFamilyIndices = nullptr;

	uint32_t queue_family_index_present = m_queue_index_present;
	uint32_t queue_family_index_graphics = m_queue_index_graphics;

	if (queue_family_index_graphics != queue_family_index_present)
	{
		uint32_t p_indecies[2] = {queue_family_index_graphics, queue_family_index_present};

		info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		info.queueFamilyIndexCount = sizeof(p_indecies) / sizeof(p_indecies[0]);
		info.pQueueFamilyIndices = p_indecies;
	}

	VkResult status = vkCreateSwapchainKHR(m_p_device, &info, nullptr, &m_p_swapchain);

	RMLUI_VK_ASSERTMSG(status == VK_SUCCESS, "failed to vkCreateSwapchainKHR");
}

void RenderInterface_VK::Initialize_Surface(CreateSurfaceCallback create_surface_callback) noexcept
{
	RMLUI_VK_ASSERTMSG(m_p_instance, "you must initialize your VkInstance");

	bool result = create_surface_callback(m_p_instance, &m_p_surface);
	RMLUI_VK_ASSERTMSG(result && m_p_surface, "failed to call create_surface_callback");
}

void RenderInterface_VK::Initialize_QueueIndecies() noexcept
{
	RMLUI_VK_ASSERTMSG(m_p_physical_device, "you must initialize your physical device");
	RMLUI_VK_ASSERTMSG(m_p_surface, "you must initialize VkSurfaceKHR before calling this method");

	uint32_t queue_family_count = 0;

	vkGetPhysicalDeviceQueueFamilyProperties(m_p_physical_device, &queue_family_count, nullptr);

	RMLUI_VK_ASSERTMSG(queue_family_count >= 1, "failed to vkGetPhysicalDeviceQueueFamilyProperties (getting count)");

	Rml::Vector<VkQueueFamilyProperties> queue_props;
	queue_props.resize(queue_family_count);

	vkGetPhysicalDeviceQueueFamilyProperties(m_p_physical_device, &queue_family_count, queue_props.data());

	RMLUI_VK_ASSERTMSG(queue_family_count >= 1, "failed to vkGetPhysicalDeviceQueueFamilyProperties (filling vector of VkQueueFamilyProperties)");

	constexpr uint32_t kUint32Undefined = uint32_t(-1);

	m_queue_index_compute = kUint32Undefined;
	m_queue_index_graphics = kUint32Undefined;
	m_queue_index_present = kUint32Undefined;

	for (uint32_t i = 0; i < queue_family_count; ++i)
	{
		if ((queue_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
		{
			if (m_queue_index_graphics == kUint32Undefined)
				m_queue_index_graphics = i;

			VkBool32 is_support_present;

			vkGetPhysicalDeviceSurfaceSupportKHR(m_p_physical_device, i, m_p_surface, &is_support_present);

			// User's videocard may have same index for two queues like graphics and present

			if (is_support_present == VK_TRUE)
			{
				m_queue_index_graphics = i;
				m_queue_index_present = m_queue_index_graphics;
				break;
			}
		}
	}

	if (m_queue_index_present == static_cast<uint32_t>(-1))
	{
		Rml::Log::Message(Rml::Log::LT_WARNING, "[Vulkan] User doesn't have one index for two queues, so we need to find for present queue index");

		for (uint32_t i = 0; i < queue_family_count; ++i)
		{
			VkBool32 is_support_present;

			vkGetPhysicalDeviceSurfaceSupportKHR(m_p_physical_device, i, m_p_surface, &is_support_present);

			if (is_support_present == VK_TRUE)
			{
				m_queue_index_present = i;
				break;
			}
		}
	}

	for (uint32_t i = 0; i < queue_family_count; ++i)
	{
		if ((queue_props[i].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0)
		{
			if (m_queue_index_compute == kUint32Undefined)
				m_queue_index_compute = i;

			if (i != m_queue_index_graphics)
			{
				m_queue_index_compute = i;
				break;
			}
		}
	}

#ifdef RMLUI_VK_DEBUG
	Rml::Log::Message(Rml::Log::LT_DEBUG, "[Vulkan] User family queues indecies: Graphics[%d] Present[%d] Compute[%d]", m_queue_index_graphics,
		m_queue_index_present, m_queue_index_compute);
#endif
}

void RenderInterface_VK::Initialize_Queues() noexcept
{
	RMLUI_VK_ASSERTMSG(m_p_device, "you must initialize VkDevice before using this method");

	vkGetDeviceQueue(m_p_device, m_queue_index_graphics, 0, &m_p_queue_graphics);

	if (m_queue_index_graphics == m_queue_index_present)
	{
		m_p_queue_present = m_p_queue_graphics;
	}
	else
	{
		vkGetDeviceQueue(m_p_device, m_queue_index_present, 0, &m_p_queue_present);
	}

	constexpr uint32_t kUint32Undefined = uint32_t(-1);

	if (m_queue_index_compute != kUint32Undefined)
	{
		vkGetDeviceQueue(m_p_device, m_queue_index_compute, 0, &m_p_queue_compute);
	}
}

void RenderInterface_VK::Initialize_SyncPrimitives() noexcept
{
	RMLUI_VK_ASSERTMSG(m_p_device, "you must initialize your device");

	m_executed_fences.resize(kSwapchainBackBufferCount);
	m_semaphores_finished_render.resize(kSwapchainBackBufferCount);
	m_semaphores_image_available.resize(kSwapchainBackBufferCount);

	VkResult status = VK_SUCCESS;

	for (uint32_t i = 0; i < kSwapchainBackBufferCount; ++i)
	{
		VkFenceCreateInfo info_fence = {};

		info_fence.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		info_fence.pNext = nullptr;
		info_fence.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		status = vkCreateFence(m_p_device, &info_fence, nullptr, &m_executed_fences[i]);

		RMLUI_VK_ASSERTMSG(status == VK_SUCCESS, "failed to vkCreateFence");

		VkSemaphoreCreateInfo info_semaphore = {};

		info_semaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		info_semaphore.pNext = nullptr;
		info_semaphore.flags = 0;

		status = vkCreateSemaphore(m_p_device, &info_semaphore, nullptr, &m_semaphores_image_available[i]);

		RMLUI_VK_ASSERTMSG(status == VK_SUCCESS, "failed to vkCreateSemaphore");

		status = vkCreateSemaphore(m_p_device, &info_semaphore, nullptr, &m_semaphores_finished_render[i]);

		RMLUI_VK_ASSERTMSG(status == VK_SUCCESS, "failed to vkCreateSemaphore");
	}
}

void RenderInterface_VK::Initialize_Resources(const VkPhysicalDeviceProperties& physical_device_properties) noexcept
{
	m_command_buffer_ring.Initialize(m_p_device, m_queue_index_graphics);

	const VkDeviceSize min_buffer_alignment = physical_device_properties.limits.minUniformBufferOffsetAlignment;
	m_memory_pool.Initialize(kVideoMemoryForAllocation, min_buffer_alignment, m_p_allocator, m_p_device);

	m_upload_manager.Initialize(m_p_device, m_p_queue_graphics, m_queue_index_graphics);
	m_manager_descriptors.Initialize(m_p_device, 100, 100, 10, 10);

	CreateShaders();
	CreateDescriptorSetLayout();
	CreatePipelineLayout();
	CreateSamplers();
	CreateDescriptorSets();
}

void RenderInterface_VK::Initialize_Allocator() noexcept
{
	RMLUI_VK_ASSERTMSG(m_p_device, "you must have a valid VkDevice here");
	RMLUI_VK_ASSERTMSG(m_p_physical_device, "you must have a valid VkPhysicalDevice here");
	RMLUI_VK_ASSERTMSG(m_p_instance, "you must have a valid VkInstance here");

	VmaVulkanFunctions vulkanFunctions = {};
	vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
	vulkanFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

	VmaAllocatorCreateInfo info = {};

	info.vulkanApiVersion = RMLUI_VK_API_VERSION;
	info.device = m_p_device;
	info.instance = m_p_instance;
	info.physicalDevice = m_p_physical_device;
	info.pVulkanFunctions = &vulkanFunctions;

	auto status = vmaCreateAllocator(&info, &m_p_allocator);

	RMLUI_VK_ASSERTMSG(status == VkResult::VK_SUCCESS, "failed to vmaCreateAllocator");
}

void RenderInterface_VK::Destroy_Instance() noexcept
{
	vkDestroyInstance(m_p_instance, nullptr);
}

void RenderInterface_VK::Destroy_Device() noexcept
{
	vkDestroyDevice(m_p_device, nullptr);
}

void RenderInterface_VK::Destroy_Swapchain() noexcept
{
	RMLUI_VK_ASSERTMSG(m_p_device, "you must initialize device");

	vkDestroySwapchainKHR(m_p_device, m_p_swapchain, nullptr);
}

void RenderInterface_VK::Destroy_Surface() noexcept
{
	vkDestroySurfaceKHR(m_p_instance, m_p_surface, nullptr);
}

void RenderInterface_VK::Destroy_SyncPrimitives() noexcept
{
	for (auto& p_fence : m_executed_fences)
	{
		vkDestroyFence(m_p_device, p_fence, nullptr);
	}

	for (auto& p_semaphore : m_semaphores_image_available)
	{
		vkDestroySemaphore(m_p_device, p_semaphore, nullptr);
	}

	for (auto& p_semaphore : m_semaphores_finished_render)
	{
		vkDestroySemaphore(m_p_device, p_semaphore, nullptr);
	}
}

void RenderInterface_VK::Destroy_Resources() noexcept
{
	m_command_buffer_ring.Shutdown();
	m_upload_manager.Shutdown();

	if (m_p_descriptor_set)
	{
		m_manager_descriptors.Free_Descriptors(m_p_device, &m_p_descriptor_set);
	}

	vkDestroyDescriptorSetLayout(m_p_device, m_p_descriptor_set_layout_vertex_transform, nullptr);
	vkDestroyDescriptorSetLayout(m_p_device, m_p_descriptor_set_layout_texture, nullptr);

	vkDestroyPipelineLayout(m_p_device, m_p_pipeline_layout, nullptr);

	for (const auto& p_module : m_shaders)
	{
		vkDestroyShaderModule(m_p_device, p_module, nullptr);
	}

	DestroySamplers();
	Destroy_Textures();
	Destroy_Geometries();

	m_manager_descriptors.Shutdown(m_p_device);
}

void RenderInterface_VK::Destroy_Allocator() noexcept
{
	RMLUI_VK_ASSERTMSG(m_p_allocator, "you must have an initialized allocator for deleting");

	vmaDestroyAllocator(m_p_allocator);

	m_p_allocator = nullptr;
}

void RenderInterface_VK::QueryInstanceLayers(LayerPropertiesList& result) noexcept
{
	uint32_t instance_layer_properties_count = 0;
	VkResult status = vkEnumerateInstanceLayerProperties(&instance_layer_properties_count, nullptr);
	RMLUI_VK_ASSERTMSG(status == VK_SUCCESS, "failed to vkEnumerateInstanceLayerProperties (getting count)");

	if (instance_layer_properties_count)
	{
		result.resize(instance_layer_properties_count);
		status = vkEnumerateInstanceLayerProperties(&instance_layer_properties_count, result.data());
		RMLUI_VK_ASSERTMSG(status == VK_SUCCESS, "failed to vkEnumerateInstanceLayerProperties (filling vector of VkLayerProperties)");
	}
}

void RenderInterface_VK::QueryInstanceExtensions(ExtensionPropertiesList& result, const LayerPropertiesList& instance_layer_properties) noexcept
{
	uint32_t instance_extension_property_count = 0;
	VkResult status = vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_property_count, nullptr);
	RMLUI_VK_ASSERTMSG(status == VK_SUCCESS, "failed to vkEnumerateInstanceExtensionProperties (getting count)");

	if (instance_extension_property_count)
	{
		result.resize(instance_extension_property_count);
		status = vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_property_count, result.data());

		RMLUI_VK_ASSERTMSG(status == VK_SUCCESS, "failed to vkEnumerateInstanceExtensionProperties (filling vector of VkExtensionProperties)");
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
				ExtensionPropertiesList props;
				props.resize(count);
				status = vkEnumerateInstanceExtensionProperties(layer_property.layerName, &count, props.data());

				if (status == VK_SUCCESS)
				{
#ifdef RMLUI_VK_DEBUG
					Rml::Log::Message(Rml::Log::LT_DEBUG, "[Vulkan] obtained extensions for layer: %s, count: %zu", layer_property.layerName,
						props.size());
#endif

					for (const auto& extension : props)
					{
						if (IsExtensionPresent(result, extension.extensionName) == false)
						{
#ifdef RMLUI_VK_DEBUG
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

bool RenderInterface_VK::AddLayerToInstance(Rml::Vector<const char*>& result, const LayerPropertiesList& instance_layer_properties,
	const char* p_instance_layer_name) noexcept
{
	if (p_instance_layer_name == nullptr)
	{
		RMLUI_VK_ASSERTMSG(p_instance_layer_name, "you have an invalid layer");
		return false;
	}

	if (IsLayerPresent(instance_layer_properties, p_instance_layer_name))
	{
		result.push_back(p_instance_layer_name);
		return true;
	}

	Rml::Log::Message(Rml::Log::LT_WARNING, "[Vulkan] can't add layer %s", p_instance_layer_name);

	return false;
}

bool RenderInterface_VK::AddExtensionToInstance(Rml::Vector<const char*>& result, const ExtensionPropertiesList& instance_extension_properties,
	const char* p_instance_extension_name) noexcept
{
	if (p_instance_extension_name == nullptr)
	{
		RMLUI_VK_ASSERTMSG(p_instance_extension_name, "you have an invalid extension");
		return false;
	}

	if (IsExtensionPresent(instance_extension_properties, p_instance_extension_name))
	{
		result.push_back(p_instance_extension_name);
		return true;
	}

	Rml::Log::Message(Rml::Log::LT_WARNING, "[Vulkan] can't add extension %s", p_instance_extension_name);

	return false;
}

void RenderInterface_VK::CreatePropertiesFor_Instance(Rml::Vector<const char*>& instance_layer_names,
	Rml::Vector<const char*>& instance_extension_names) noexcept
{
	ExtensionPropertiesList instance_extension_properties;
	LayerPropertiesList instance_layer_properties;

	QueryInstanceLayers(instance_layer_properties);
	QueryInstanceExtensions(instance_extension_properties, instance_layer_properties);

	AddExtensionToInstance(instance_extension_names, instance_extension_properties, "VK_EXT_debug_utils");
	AddExtensionToInstance(instance_extension_names, instance_extension_properties, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

#ifdef RMLUI_VK_DEBUG
	AddLayerToInstance(instance_layer_names, instance_layer_properties, "VK_LAYER_LUNARG_monitor");

	bool is_cpu_validation = AddLayerToInstance(instance_layer_names, instance_layer_properties, "VK_LAYER_KHRONOS_validation") &&
		AddExtensionToInstance(instance_extension_names, instance_extension_properties, VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

	if (is_cpu_validation)
	{
		Rml::Log::Message(Rml::Log::LT_DEBUG, "[Vulkan] CPU validation is enabled");

		Rml::Array<const char*, 1> requested_extensions_for_gpu = {VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME};

		for (const auto& extension_name : requested_extensions_for_gpu)
		{
			AddExtensionToInstance(instance_extension_names, instance_extension_properties, extension_name);
		}

		debug_validation_features_ext.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
		debug_validation_features_ext.pNext = nullptr;
		debug_validation_features_ext.enabledValidationFeatureCount =
			sizeof(debug_validation_features_ext_requested) / sizeof(debug_validation_features_ext_requested[0]);
		debug_validation_features_ext.pEnabledValidationFeatures = debug_validation_features_ext_requested;
	}

#else
	(void)instance_layer_names;

#endif
}

bool RenderInterface_VK::IsLayerPresent(const LayerPropertiesList& properties, const char* p_layer_name) noexcept
{
	if (properties.empty())
		return false;

	if (p_layer_name == nullptr)
		return false;

	return std::find_if(properties.cbegin(), properties.cend(),
			   [p_layer_name](const VkLayerProperties& prop) -> bool { return strcmp(prop.layerName, p_layer_name) == 0; }) != properties.cend();
}

bool RenderInterface_VK::IsExtensionPresent(const ExtensionPropertiesList& properties, const char* p_extension_name) noexcept
{
	if (properties.empty())
		return false;

	if (p_extension_name == nullptr)
		return false;

	return std::find_if(properties.cbegin(), properties.cend(), [p_extension_name](const VkExtensionProperties& prop) -> bool {
		return strcmp(prop.extensionName, p_extension_name) == 0;
	}) != properties.cend();
}

bool RenderInterface_VK::AddExtensionToDevice(Rml::Vector<const char*>& result, const ExtensionPropertiesList& device_extension_properties,
	const char* p_device_extension_name) noexcept
{
	if (IsExtensionPresent(device_extension_properties, p_device_extension_name))
	{
		result.push_back(p_device_extension_name);
		return true;
	}

	return false;
}

void RenderInterface_VK::CreatePropertiesFor_Device(ExtensionPropertiesList& result) noexcept
{
	RMLUI_VK_ASSERTMSG(m_p_physical_device, "you must initialize your physical device. Call InitializePhysicalDevice first");

	uint32_t extension_count = 0;
	VkResult status = vkEnumerateDeviceExtensionProperties(m_p_physical_device, nullptr, &extension_count, nullptr);
	RMLUI_VK_ASSERTMSG(status == VK_SUCCESS, "failed to vkEnumerateDeviceExtensionProperties (getting count)");

	result.resize(extension_count);
	status = vkEnumerateDeviceExtensionProperties(m_p_physical_device, nullptr, &extension_count, result.data());
	RMLUI_VK_ASSERTMSG(status == VK_SUCCESS, "failed to vkEnumerateDeviceExtensionProperties (filling vector of VkExtensionProperties)");

	uint32_t instance_layer_property_count = 0;
	status = vkEnumerateInstanceLayerProperties(&instance_layer_property_count, nullptr);
	RMLUI_VK_ASSERTMSG(status == VK_SUCCESS, "failed to vkEnumerateInstanceLayerProperties (getting count)");

	LayerPropertiesList layers;
	layers.resize(instance_layer_property_count);

	// On different OS Vulkan acts strange, so we can't get our extensions to just iterate through default functions
	// We need to deeply analyze our layers and get specified extensions which pass user
	// So we collect all extensions that are presented in physical device
	// And add when they exist to extension_names so we don't pass properties

	if (instance_layer_property_count)
	{
		status = vkEnumerateInstanceLayerProperties(&instance_layer_property_count, layers.data());
		RMLUI_VK_ASSERTMSG(status == VK_SUCCESS, "failed to vkEnumerateInstanceLayerProperties (filling vector of VkLayerProperties)");

		for (const auto& layer : layers)
		{
			extension_count = 0;
			status = vkEnumerateDeviceExtensionProperties(m_p_physical_device, layer.layerName, &extension_count, nullptr);
			RMLUI_VK_ASSERTMSG(status == VK_SUCCESS, "failed to vkEnumerateDeviceExtensionProperties (getting count)");

			if (extension_count)
			{
				ExtensionPropertiesList new_extensions;
				new_extensions.resize(extension_count);

				status = vkEnumerateDeviceExtensionProperties(m_p_physical_device, layer.layerName, &extension_count, new_extensions.data());
				RMLUI_VK_ASSERTMSG(status == VK_SUCCESS, "failed to vkEnumerateDeviceExtensionProperties (filling vector of VkExtensionProperties)");

				for (const auto& extension : new_extensions)
				{
					if (IsExtensionPresent(result, extension.extensionName) == false)
					{
#ifdef RMLUI_VK_DEBUG
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

void RenderInterface_VK::CreateReportDebugCallback() noexcept
{
#ifdef RMLUI_VK_DEBUG
	VkDebugUtilsMessengerCreateInfoEXT info = {};

	info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
	info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	info.pfnUserCallback = MyDebugReportCallback;

	PFN_vkCreateDebugUtilsMessengerEXT p_callback_creation = VK_NULL_HANDLE;

	p_callback_creation = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(m_p_instance, "vkCreateDebugUtilsMessengerEXT"));
	VkResult status = p_callback_creation(m_p_instance, &info, nullptr, &m_debug_messenger);
	RMLUI_VK_ASSERTMSG(status == VK_SUCCESS, "failed to vkCreateDebugUtilsMessengerEXT");
#endif
}

void RenderInterface_VK::Destroy_ReportDebugCallback() noexcept
{
#ifdef RMLUI_VK_DEBUG
	PFN_vkDestroyDebugUtilsMessengerEXT p_destroy_callback =
		reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(m_p_instance, "vkDestroyDebugUtilsMessengerEXT"));

	if (m_debug_messenger)
	{
		p_destroy_callback(m_p_instance, m_debug_messenger, nullptr);
		m_debug_messenger = VK_NULL_HANDLE;
	}
#endif
}

uint32_t RenderInterface_VK::GetUserAPIVersion() const noexcept
{
	uint32_t result = RMLUI_VK_API_VERSION;

#if defined VK_VERSION_1_1
	VkResult status = vkEnumerateInstanceVersion(&result);
	RMLUI_VK_ASSERTMSG(status == VK_SUCCESS, "failed to vkEnumerateInstanceVersion, See Status");
#endif

	return result;
}

uint32_t RenderInterface_VK::GetRequiredVersionAndValidateMachine() noexcept
{
	constexpr uint32_t kRequiredVersion = RMLUI_VK_API_VERSION;
	const uint32_t user_version = GetUserAPIVersion();

	RMLUI_VK_ASSERTMSG(kRequiredVersion <= user_version, "Your machine doesn't support Vulkan");

	return kRequiredVersion;
}

void RenderInterface_VK::CollectPhysicalDevices(PhysicalDeviceWrapperList& out_physical_devices) noexcept
{
	uint32_t gpu_count = 1;
	Rml::Vector<VkPhysicalDevice> temp_devices;

	VkResult status = vkEnumeratePhysicalDevices(m_p_instance, &gpu_count, nullptr);
	RMLUI_VK_ASSERTMSG(status == VK_SUCCESS, "failed to vkEnumeratePhysicalDevices (getting count)");

	temp_devices.resize(gpu_count);
	status = vkEnumeratePhysicalDevices(m_p_instance, &gpu_count, temp_devices.data());

	RMLUI_VK_ASSERTMSG(status == VK_SUCCESS, "failed to vkEnumeratePhysicalDevices (filling the vector of VkPhysicalDevice)");
	RMLUI_VK_ASSERTMSG(temp_devices.empty() == false, "you must have one videocard at least!");

	out_physical_devices.resize(temp_devices.size());
	for (size_t i = 0; i < out_physical_devices.size(); i++)
	{
		out_physical_devices[i].m_p_physical_device = temp_devices[i];
		vkGetPhysicalDeviceProperties(out_physical_devices[i].m_p_physical_device, &out_physical_devices[i].m_physical_device_properties);
	}
}

const RenderInterface_VK::PhysicalDeviceWrapper* RenderInterface_VK::ChoosePhysicalDevice(const PhysicalDeviceWrapperList& physical_devices,
	VkPhysicalDeviceType device_type) noexcept
{
	RMLUI_VK_ASSERTMSG(physical_devices.empty() == false,
		"you must have one videocard at least or early calling of this method, try call this after CollectPhysicalDevices");

	for (const auto& device : physical_devices)
	{
		if (device.m_physical_device_properties.deviceType == device_type)
			return &device;
	}

	return nullptr;
}

VkSurfaceFormatKHR RenderInterface_VK::ChooseSwapchainFormat() noexcept
{
	static constexpr VkFormat UNORM_FORMATS[] = {
		VK_FORMAT_R4G4_UNORM_PACK8,
		VK_FORMAT_R4G4B4A4_UNORM_PACK16,
		VK_FORMAT_B4G4R4A4_UNORM_PACK16,
		VK_FORMAT_R5G6B5_UNORM_PACK16,
		VK_FORMAT_B5G6R5_UNORM_PACK16,
		VK_FORMAT_R5G5B5A1_UNORM_PACK16,
		VK_FORMAT_B5G5R5A1_UNORM_PACK16,
		VK_FORMAT_A1R5G5B5_UNORM_PACK16,
		VK_FORMAT_R8_UNORM,
		VK_FORMAT_R8G8_UNORM,
		VK_FORMAT_R8G8B8_UNORM,
		VK_FORMAT_B8G8R8_UNORM,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_FORMAT_B8G8R8A8_UNORM,
		VK_FORMAT_A8B8G8R8_UNORM_PACK32,
		VK_FORMAT_A2R10G10B10_UNORM_PACK32,
		VK_FORMAT_A2B10G10R10_UNORM_PACK32,
		VK_FORMAT_R16_UNORM,
		VK_FORMAT_R16G16_UNORM,
		VK_FORMAT_R16G16B16_UNORM,
		VK_FORMAT_R16G16B16A16_UNORM,
		VK_FORMAT_D16_UNORM,
		VK_FORMAT_X8_D24_UNORM_PACK32,
		VK_FORMAT_D16_UNORM_S8_UINT,
		VK_FORMAT_D24_UNORM_S8_UINT,
		VK_FORMAT_BC1_RGB_UNORM_BLOCK,
		VK_FORMAT_BC1_RGBA_UNORM_BLOCK,
		VK_FORMAT_BC2_UNORM_BLOCK,
		VK_FORMAT_BC3_UNORM_BLOCK,
		VK_FORMAT_BC4_UNORM_BLOCK,
		VK_FORMAT_BC5_UNORM_BLOCK,
		VK_FORMAT_BC7_UNORM_BLOCK,
		VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK,
		VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK,
		VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK,
		VK_FORMAT_EAC_R11_UNORM_BLOCK,
		VK_FORMAT_EAC_R11G11_UNORM_BLOCK,
		VK_FORMAT_ASTC_4x4_UNORM_BLOCK,
		VK_FORMAT_ASTC_5x4_UNORM_BLOCK,
		VK_FORMAT_ASTC_5x5_UNORM_BLOCK,
		VK_FORMAT_ASTC_6x5_UNORM_BLOCK,
		VK_FORMAT_ASTC_6x6_UNORM_BLOCK,
		VK_FORMAT_ASTC_8x5_UNORM_BLOCK,
		VK_FORMAT_ASTC_8x6_UNORM_BLOCK,
		VK_FORMAT_ASTC_8x8_UNORM_BLOCK,
		VK_FORMAT_ASTC_10x5_UNORM_BLOCK,
		VK_FORMAT_ASTC_10x6_UNORM_BLOCK,
		VK_FORMAT_ASTC_10x8_UNORM_BLOCK,
		VK_FORMAT_ASTC_10x10_UNORM_BLOCK,
		VK_FORMAT_ASTC_12x10_UNORM_BLOCK,
		VK_FORMAT_ASTC_12x12_UNORM_BLOCK,
	};

	RMLUI_VK_ASSERTMSG(m_p_physical_device, "you must initialize your physical device, before calling this method");
	RMLUI_VK_ASSERTMSG(m_p_surface, "you must initialize your surface, before calling this method");

	uint32_t surface_count = 0;
	VkResult status = vkGetPhysicalDeviceSurfaceFormatsKHR(m_p_physical_device, m_p_surface, &surface_count, nullptr);
	RMLUI_VK_ASSERTMSG(status == VK_SUCCESS, "failed to vkGetPhysicalDeviceSurfaceFormatsKHR (getting count)");

	Rml::Vector<VkSurfaceFormatKHR> formats(surface_count);
	status = vkGetPhysicalDeviceSurfaceFormatsKHR(m_p_physical_device, m_p_surface, &surface_count, formats.data());
	RMLUI_VK_ASSERTMSG(status == VK_SUCCESS, "failed to vkGetPhysicalDeviceSurfaceFormatsKHR (filling vector of VkSurfaceFormatKHR)");

	// Prefer UNORM formats
	for (auto& format : formats)
	{
		for (auto ufmt : UNORM_FORMATS)
		{
			if (ufmt == format.format)
				return format;
		}
	}

	return formats.front();
}

VkExtent2D RenderInterface_VK::GetValidSurfaceExtent() noexcept
{
	VkSurfaceCapabilitiesKHR caps = GetSurfaceCapabilities();
	VkExtent2D result = {(uint32_t)m_width, (uint32_t)m_height};

	/*
	    https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkSurfaceCapabilitiesKHR.html
	*/
	if (caps.currentExtent.width == 0xFFFFFFFF)
	{
		result.width = Rml::Math::Clamp(result.width, caps.minImageExtent.width, caps.maxImageExtent.width);
		result.height = Rml::Math::Clamp(result.height, caps.minImageExtent.height, caps.maxImageExtent.height);
	}
	else
	{
		result = caps.currentExtent;
	}

	return result;
}

VkSurfaceTransformFlagBitsKHR RenderInterface_VK::CreatePretransformSwapchain() noexcept
{
	auto caps = GetSurfaceCapabilities();

	VkSurfaceTransformFlagBitsKHR result =
		(caps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) ? VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR : caps.currentTransform;

	return result;
}

VkCompositeAlphaFlagBitsKHR RenderInterface_VK::ChooseSwapchainCompositeAlpha() noexcept
{
	auto caps = GetSurfaceCapabilities();

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

int RenderInterface_VK::Choose_SwapchainImageCount(uint32_t user_swapchain_count_for_creation, bool if_failed_choose_min) noexcept
{
	auto caps = GetSurfaceCapabilities();

	// don't worry if you get this assert just ignore it the method will fix the count ;)
	RMLUI_VK_ASSERTMSG(user_swapchain_count_for_creation >= caps.minImageCount,
		"can't be, you must have a valid count that bounds from minImageCount to maxImageCount! Otherwise you will get a validation error that "
		"specifies that you created a swapchain with invalid image count");
	RMLUI_VK_ASSERTMSG(user_swapchain_count_for_creation <= caps.maxImageCount,
		"can't be, you must have a valid count that bounds from minImageCount to maxImageCount! Otherwise you will get a validation error that "
		"specifies that you created a swapchain with invalid image count");

	int result = 0;

	if (user_swapchain_count_for_creation < caps.minImageCount || user_swapchain_count_for_creation > caps.maxImageCount)
		result = if_failed_choose_min ? caps.minImageCount : caps.maxImageCount;
	else
		result = user_swapchain_count_for_creation;

	return result;
}

// https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkPresentModeKHR.html
// VK_PRESENT_MODE_FIFO_KHR system must support this mode at least so by default we want to use it otherwise user can specify his mode
VkPresentModeKHR RenderInterface_VK::GetPresentMode(VkPresentModeKHR required) noexcept
{
	RMLUI_VK_ASSERTMSG(m_p_device, "[Vulkan] you must initialize your device, before calling this method");
	RMLUI_VK_ASSERTMSG(m_p_physical_device, "[Vulkan] you must initialize your physical device, before calling this method");
	RMLUI_VK_ASSERTMSG(m_p_surface, "[Vulkan] you must initialize your surface, before calling this method");

	VkPresentModeKHR result = required;

	uint32_t present_modes_count = 0;
	VkResult status = vkGetPhysicalDeviceSurfacePresentModesKHR(m_p_physical_device, m_p_surface, &present_modes_count, nullptr);
	RMLUI_VK_ASSERTMSG(status == VK_SUCCESS, "[Vulkan] failed to vkGetPhysicalDeviceSurfacePresentModesKHR (getting count)");

	Rml::Vector<VkPresentModeKHR> present_modes(present_modes_count);
	status = vkGetPhysicalDeviceSurfacePresentModesKHR(m_p_physical_device, m_p_surface, &present_modes_count, present_modes.data());
	RMLUI_VK_ASSERTMSG(status == VK_SUCCESS, "[Vulkan] failed to vkGetPhysicalDeviceSurfacePresentModesKHR (filling vector of VkPresentModeKHR)");

	for (const auto& mode : present_modes)
	{
		if (mode == required)
			return result;
	}

	Rml::Log::Message(Rml::Log::LT_WARNING,
		"[Vulkan] WARNING system can't detect your type of present mode so we choose the first from vector front");

	return present_modes.front();
}

VkSurfaceCapabilitiesKHR RenderInterface_VK::GetSurfaceCapabilities() noexcept
{
	RMLUI_VK_ASSERTMSG(m_p_device, "[Vulkan] you must initialize your device, before calling this method");
	RMLUI_VK_ASSERTMSG(m_p_physical_device, "[Vulkan] you must initialize your physical device, before calling this method");
	RMLUI_VK_ASSERTMSG(m_p_surface, "[Vulkan] you must initialize your surface, before calling this method");

	VkSurfaceCapabilitiesKHR result;
	VkResult status = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_p_physical_device, m_p_surface, &result);
	RMLUI_VK_ASSERTMSG(status == VK_SUCCESS, "[Vulkan] failed to vkGetPhysicalDeviceSurfaceCapabilitiesKHR");

	return result;
}

void RenderInterface_VK::CreateShaders() noexcept
{
	RMLUI_VK_ASSERTMSG(m_p_device, "[Vulkan] you must initialize VkDevice before calling this method");

	struct shader_data_t {
		const uint32_t* m_data;
		size_t m_data_size;
		VkShaderStageFlagBits m_shader_type;
	};

	const Rml::Vector<shader_data_t> shaders = {
		{reinterpret_cast<const uint32_t*>(shader_vert), sizeof(shader_vert), VK_SHADER_STAGE_VERTEX_BIT},
		{reinterpret_cast<const uint32_t*>(shader_frag_color), sizeof(shader_frag_color), VK_SHADER_STAGE_FRAGMENT_BIT},
		{reinterpret_cast<const uint32_t*>(shader_frag_texture), sizeof(shader_frag_texture), VK_SHADER_STAGE_FRAGMENT_BIT},
	};

	for (const shader_data_t& shader_data : shaders)
	{
		VkShaderModuleCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		info.pCode = shader_data.m_data;
		info.codeSize = shader_data.m_data_size;

		VkShaderModule p_module = nullptr;
		VkResult status = vkCreateShaderModule(m_p_device, &info, nullptr, &p_module);

		RMLUI_VK_ASSERTMSG(status == VK_SUCCESS, "[Vulkan] failed to vkCreateShaderModule");

		m_shaders.push_back(p_module);
	}
}

void RenderInterface_VK::CreateDescriptorSetLayout() noexcept
{
	RMLUI_VK_ASSERTMSG(m_p_device, "[Vulkan] you must initialize VkDevice before calling this method");
	RMLUI_VK_ASSERTMSG(!m_p_descriptor_set_layout_vertex_transform && !m_p_descriptor_set_layout_texture, "[Vulkan] Already initialized");

	{
		VkDescriptorSetLayoutBinding binding_for_vertex_transform = {};
		binding_for_vertex_transform.binding = 1;
		binding_for_vertex_transform.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		binding_for_vertex_transform.descriptorCount = 1;
		binding_for_vertex_transform.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		VkDescriptorSetLayoutCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		info.pBindings = &binding_for_vertex_transform;
		info.bindingCount = 1;

		VkResult status = vkCreateDescriptorSetLayout(m_p_device, &info, nullptr, &m_p_descriptor_set_layout_vertex_transform);
		RMLUI_VK_ASSERTMSG(status == VK_SUCCESS, "[Vulkan] failed to vkCreateDescriptorSetLayout");
	}

	{
		VkDescriptorSetLayoutBinding binding_for_fragment_texture = {};
		binding_for_fragment_texture.binding = 2;
		binding_for_fragment_texture.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		binding_for_fragment_texture.descriptorCount = 1;
		binding_for_fragment_texture.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		info.pBindings = &binding_for_fragment_texture;
		info.bindingCount = 1;

		VkResult status = vkCreateDescriptorSetLayout(m_p_device, &info, nullptr, &m_p_descriptor_set_layout_texture);
		RMLUI_VK_ASSERTMSG(status == VK_SUCCESS, "[Vulkan] failed to vkCreateDescriptorSetLayout");
	}
}

void RenderInterface_VK::CreatePipelineLayout() noexcept
{
	RMLUI_VK_ASSERTMSG(m_p_descriptor_set_layout_vertex_transform, "[Vulkan] You must initialize VkDescriptorSetLayout before calling this method");
	RMLUI_VK_ASSERTMSG(m_p_descriptor_set_layout_texture,
		"[Vulkan] you must initialize VkDescriptorSetLayout for textures before calling this method!");
	RMLUI_VK_ASSERTMSG(m_p_device, "[Vulkan] you must initialize VkDevice before calling this method");

	VkDescriptorSetLayout p_layouts[] = {m_p_descriptor_set_layout_vertex_transform, m_p_descriptor_set_layout_texture};

	VkPipelineLayoutCreateInfo info = {};

	info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	info.pNext = nullptr;
	info.pSetLayouts = p_layouts;
	info.setLayoutCount = 2;

	auto status = vkCreatePipelineLayout(m_p_device, &info, nullptr, &m_p_pipeline_layout);

	RMLUI_VK_ASSERTMSG(status == VK_SUCCESS, "[Vulkan] failed to vkCreatePipelineLayout");
}

void RenderInterface_VK::CreateDescriptorSets() noexcept
{
	RMLUI_VK_ASSERTMSG(m_p_device, "[Vulkan] you have to initialize your VkDevice before calling this method");
	RMLUI_VK_ASSERTMSG(m_p_descriptor_set_layout_vertex_transform,
		"[Vulkan] you have to initialize your VkDescriptorSetLayout before calling this method");

	m_manager_descriptors.Alloc_Descriptor(m_p_device, &m_p_descriptor_set_layout_vertex_transform, &m_p_descriptor_set);
	m_memory_pool.SetDescriptorSet(1, sizeof(shader_vertex_user_data_t), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, m_p_descriptor_set);
}

void RenderInterface_VK::CreateSamplers() noexcept
{
	VkSamplerCreateInfo info = {};

	info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	info.pNext = nullptr;
	info.magFilter = VK_FILTER_LINEAR;
	info.minFilter = VK_FILTER_LINEAR;
	info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	vkCreateSampler(m_p_device, &info, nullptr, &m_p_sampler_linear);
}

void RenderInterface_VK::Create_Pipelines() noexcept
{
	RMLUI_VK_ASSERTMSG(m_p_pipeline_layout, "must be initialized");
	RMLUI_VK_ASSERTMSG(m_p_render_pass, "must be initialized");

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
	info_color_blend_att.srcColorBlendFactor = VkBlendFactor::VK_BLEND_FACTOR_ONE;
	info_color_blend_att.dstColorBlendFactor = VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	info_color_blend_att.colorBlendOp = VkBlendOp::VK_BLEND_OP_ADD;
	info_color_blend_att.srcAlphaBlendFactor = VkBlendFactor::VK_BLEND_FACTOR_ONE;
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
	info_shader.module = m_shaders[static_cast<int>(shader_id_t::Vertex)];

	shaders_that_will_be_used_in_pipeline[0] = info_shader;

	info_shader.module = m_shaders[static_cast<int>(shader_id_t::Fragment_WithTextures)];
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
	info.layout = m_p_pipeline_layout;
	info.renderPass = m_p_render_pass;
	info.subpass = 0;

	auto status = vkCreateGraphicsPipelines(m_p_device, nullptr, 1, &info, nullptr, &m_p_pipeline_with_textures);
	RMLUI_VK_ASSERTMSG(status == VkResult::VK_SUCCESS, "failed to vkCreateGraphicsPipelines");

	info_depth.back.passOp = VK_STENCIL_OP_KEEP;
	info_depth.back.failOp = VK_STENCIL_OP_KEEP;
	info_depth.back.depthFailOp = VK_STENCIL_OP_KEEP;
	info_depth.back.compareOp = VK_COMPARE_OP_EQUAL;
	info_depth.back.compareMask = 1;
	info_depth.back.writeMask = 1;
	info_depth.back.reference = 1;
	info_depth.front = info_depth.back;

	status = vkCreateGraphicsPipelines(m_p_device, nullptr, 1, &info, nullptr,
		&m_p_pipeline_stencil_for_regular_geometry_that_applied_to_region_with_textures);
	RMLUI_VK_ASSERTMSG(status == VkResult::VK_SUCCESS, "failed to vkCreateGraphicsPipelines");

	info_shader.module = m_shaders[static_cast<int>(shader_id_t::Fragment_WithoutTextures)];
	shaders_that_will_be_used_in_pipeline[1] = info_shader;
	info_depth.back.compareOp = VK_COMPARE_OP_ALWAYS;
	info_depth.back.failOp = VK_STENCIL_OP_KEEP;
	info_depth.back.depthFailOp = VK_STENCIL_OP_KEEP;
	info_depth.back.passOp = VK_STENCIL_OP_KEEP;
	info_depth.back.compareMask = 1;
	info_depth.back.writeMask = 1;
	info_depth.back.reference = 1;
	info_depth.front = info_depth.back;

	status = vkCreateGraphicsPipelines(m_p_device, nullptr, 1, &info, nullptr, &m_p_pipeline_without_textures);
	RMLUI_VK_ASSERTMSG(status == VkResult::VK_SUCCESS, "failed to vkCreateGraphicsPipelines");

	info_depth.back.passOp = VK_STENCIL_OP_KEEP;
	info_depth.back.failOp = VK_STENCIL_OP_KEEP;
	info_depth.back.depthFailOp = VK_STENCIL_OP_KEEP;
	info_depth.back.compareOp = VK_COMPARE_OP_EQUAL;
	info_depth.back.compareMask = 1;
	info_depth.back.writeMask = 1;
	info_depth.back.reference = 1;
	info_depth.front = info_depth.back;

	status = vkCreateGraphicsPipelines(m_p_device, nullptr, 1, &info, nullptr,
		&m_p_pipeline_stencil_for_regular_geometry_that_applied_to_region_without_textures);
	RMLUI_VK_ASSERTMSG(status == VkResult::VK_SUCCESS, "failed to vkCreateGraphicsPipelines");

	info_color_blend_att.colorWriteMask = 0x0;
	info_depth.back.passOp = VK_STENCIL_OP_REPLACE;
	info_depth.back.failOp = VK_STENCIL_OP_KEEP;
	info_depth.back.depthFailOp = VK_STENCIL_OP_KEEP;
	info_depth.back.compareOp = VK_COMPARE_OP_ALWAYS;
	info_depth.back.compareMask = 1;
	info_depth.back.writeMask = 1;
	info_depth.back.reference = 1;
	info_depth.front = info_depth.back;

	status = vkCreateGraphicsPipelines(m_p_device, nullptr, 1, &info, nullptr, &m_p_pipeline_stencil_for_region_where_geometry_will_be_drawn);
	RMLUI_VK_ASSERTMSG(status == VkResult::VK_SUCCESS, "failed to vkCreateGraphicsPipelines");

#ifdef RMLUI_DEBUG
	VkDebugUtilsObjectNameInfoEXT info_debug = {};

	info_debug.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
	info_debug.pObjectName = "pipeline_stencil for region where geometry will be drawn";
	info_debug.objectType = VkObjectType::VK_OBJECT_TYPE_PIPELINE;
	info_debug.objectHandle = (uint64_t)m_p_pipeline_stencil_for_region_where_geometry_will_be_drawn;

	vkSetDebugUtilsObjectNameEXT(m_p_device, &info_debug);

	info_debug.pObjectName = "pipeline_stencil_for_regular_geometry_that_applied_to_region_without_textures";
	info_debug.objectHandle = (uint64_t)m_p_pipeline_stencil_for_regular_geometry_that_applied_to_region_without_textures;

	vkSetDebugUtilsObjectNameEXT(m_p_device, &info_debug);

	info_debug.pObjectName = "pipeline_without_textures";
	info_debug.objectHandle = (uint64_t)m_p_pipeline_without_textures;

	vkSetDebugUtilsObjectNameEXT(m_p_device, &info_debug);

	info_debug.pObjectName = "pipeline_stencil_for_regular_geometry_that_applied_to_region_with_textures";
	info_debug.objectHandle = (uint64_t)m_p_pipeline_stencil_for_regular_geometry_that_applied_to_region_with_textures;

	vkSetDebugUtilsObjectNameEXT(m_p_device, &info_debug);

	info_debug.pObjectName = "pipeline_with_textures";
	info_debug.objectHandle = (uint64_t)m_p_pipeline_with_textures;

	vkSetDebugUtilsObjectNameEXT(m_p_device, &info_debug);
#endif
}

void RenderInterface_VK::CreateSwapchainFrameBuffers(const VkExtent2D& real_render_image_size) noexcept
{
	RMLUI_VK_ASSERTMSG(m_p_render_pass, "you must create a VkRenderPass before calling this method");
	RMLUI_VK_ASSERTMSG(m_p_device, "you must have a valid VkDevice here");

	CreateSwapchainImageViews();
	Create_DepthStencilImage();
	Create_DepthStencilImageViews();

	m_swapchain_frame_buffers.resize(m_swapchain_image_views.size());

	Rml::Array<VkImageView, 2> attachments;

	VkFramebufferCreateInfo info = {};
	info.sType = VkStructureType::VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	info.pNext = nullptr;
	info.renderPass = m_p_render_pass;
	info.attachmentCount = static_cast<uint32_t>(attachments.size());
	info.pAttachments = attachments.data();
	info.width = real_render_image_size.width;
	info.height = real_render_image_size.height;
	info.layers = 1;

	int index = 0;
	VkResult status = VkResult::VK_SUCCESS;

	attachments[1] = m_texture_depthstencil.m_p_vk_image_view;

	for (auto p_view : m_swapchain_image_views)
	{
		attachments[0] = p_view;

		status = vkCreateFramebuffer(m_p_device, &info, nullptr, &m_swapchain_frame_buffers[index]);

		RMLUI_VK_ASSERTMSG(status == VkResult::VK_SUCCESS, "failed to vkCreateFramebuffer");

		++index;
	}
}

void RenderInterface_VK::CreateSwapchainImages() noexcept
{
	RMLUI_VK_ASSERTMSG(m_p_device, "[Vulkan] you must initialize VkDevice before calling this method");
	RMLUI_VK_ASSERTMSG(m_p_swapchain, "[Vulkan] you must initialize VkSwapchainKHR before calling this method");

	uint32_t count = 0;
	auto status = vkGetSwapchainImagesKHR(m_p_device, m_p_swapchain, &count, nullptr);

	RMLUI_VK_ASSERTMSG(status == VK_SUCCESS, "[Vulkan] failed to vkGetSwapchainImagesKHR (get count)");

	m_swapchain_images.resize(count);

	status = vkGetSwapchainImagesKHR(m_p_device, m_p_swapchain, &count, m_swapchain_images.data());

	RMLUI_VK_ASSERTMSG(status == VK_SUCCESS, "[Vulkan] failed to vkGetSwapchainImagesKHR (filling vector)");
}

void RenderInterface_VK::CreateSwapchainImageViews() noexcept
{
	CreateSwapchainImages();

	m_swapchain_image_views.resize(m_swapchain_images.size());

	uint32_t index = 0;
	VkImageViewCreateInfo info = {};
	VkResult status = VkResult::VK_SUCCESS;

	for (auto p_image : m_swapchain_images)
	{
		info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		info.pNext = nullptr;
		info.format = m_swapchain_format.format;
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

		status = vkCreateImageView(m_p_device, &info, nullptr, &m_swapchain_image_views[index]);
		++index;

		RMLUI_VK_ASSERTMSG(status == VK_SUCCESS, "[Vulkan] failed to vkCreateImageView (creating swapchain views)");
	}
}

void RenderInterface_VK::Create_DepthStencilImage() noexcept
{
	RMLUI_VK_ASSERTMSG(m_p_device, "you must initialize your VkDevice here");
	RMLUI_VK_ASSERTMSG(m_p_allocator, "you must initialize your VMA allcator");
	RMLUI_VK_ASSERTMSG(m_texture_depthstencil.m_p_vk_image == nullptr, "you should delete texture before create it");

	VkImageCreateInfo info = {};

	info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	info.imageType = VK_IMAGE_TYPE_2D;
	info.format = Get_SupportedDepthFormat();
	info.extent = {static_cast<uint32_t>(m_width), static_cast<uint32_t>(m_height), 1};
	info.mipLevels = 1;
	info.arrayLayers = 1;
	info.samples = VK_SAMPLE_COUNT_1_BIT;
	info.tiling = VK_IMAGE_TILING_OPTIMAL;
	info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	VmaAllocation p_allocation = {};
	VkImage p_image = {};

	VmaAllocationCreateInfo info_alloc = {};
	auto p_commentary = "our depth stencil image";

	info_alloc.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	info_alloc.flags = VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
	info_alloc.pUserData = const_cast<char*>(p_commentary);

	VkResult status = vmaCreateImage(m_p_allocator, &info, &info_alloc, &p_image, &p_allocation, nullptr);

	RMLUI_VK_ASSERTMSG(status == VkResult::VK_SUCCESS, "failed to vmaCreateImage");

	m_texture_depthstencil.m_p_vk_image = p_image;
	m_texture_depthstencil.m_p_vma_allocation = p_allocation;
}

void RenderInterface_VK::Create_DepthStencilImageViews() noexcept
{
	RMLUI_VK_ASSERTMSG(m_p_device, "you must initialize your VkDevice here");
	RMLUI_VK_ASSERTMSG(m_texture_depthstencil.m_p_vk_image_view == nullptr, "you should delete it before creating");
	RMLUI_VK_ASSERTMSG(m_texture_depthstencil.m_p_vk_image, "you must initialize VkImage before create this");

	VkImageViewCreateInfo info = {};

	info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	info.image = m_texture_depthstencil.m_p_vk_image;
	info.format = Get_SupportedDepthFormat();
	info.subresourceRange.baseMipLevel = 0;
	info.subresourceRange.levelCount = 1;
	info.subresourceRange.baseArrayLayer = 0;
	info.subresourceRange.layerCount = 1;
	info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

	if (Get_SupportedDepthFormat() >= VK_FORMAT_D16_UNORM_S8_UINT)
	{
		info.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
	}

	VkImageView p_image_view = {};

	VkResult status = vkCreateImageView(m_p_device, &info, nullptr, &p_image_view);

	RMLUI_VK_ASSERTMSG(status == VkResult::VK_SUCCESS, "failed to vkCreateImageView");

	m_texture_depthstencil.m_p_vk_image_view = p_image_view;
}

void RenderInterface_VK::CreateResourcesDependentOnSize(const VkExtent2D& real_render_image_size) noexcept
{
	m_viewport.height = static_cast<float>(real_render_image_size.height);
	m_viewport.width = static_cast<float>(real_render_image_size.width);
	m_viewport.minDepth = 0.0f;
	m_viewport.maxDepth = 1.0f;
	m_viewport.x = 0.0f;
	m_viewport.y = 0.0f;

	m_scissor.extent.width = real_render_image_size.width;
	m_scissor.extent.height = real_render_image_size.height;
	m_scissor.offset.x = 0;
	m_scissor.offset.y = 0;

	m_scissor_original = m_scissor;

	m_projection = Rml::Matrix4f::ProjectOrtho(0.0f, static_cast<float>(m_width), static_cast<float>(m_height), 0.0f, -10000, 10000);

	// https://matthewwellings.com/blog/the-new-vulkan-coordinate-system/
	Rml::Matrix4f correction_matrix;
	correction_matrix.SetColumns(Rml::Vector4f(1.0f, 0.0f, 0.0f, 0.0f), Rml::Vector4f(0.0f, -1.0f, 0.0f, 0.0f), Rml::Vector4f(0.0f, 0.0f, 0.5f, 0.0f),
		Rml::Vector4f(0.0f, 0.0f, 0.5f, 1.0f));

	m_projection = correction_matrix * m_projection;

	SetTransform(nullptr);

	CreateRenderPass();
	CreateSwapchainFrameBuffers(real_render_image_size);
	Create_Pipelines();
}

RenderInterface_VK::buffer_data_t RenderInterface_VK::CreateResource_StagingBuffer(VkDeviceSize size, VkBufferUsageFlags flags) noexcept
{
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

	VkResult status = vmaCreateBuffer(m_p_allocator, &info, &info_allocation, &p_buffer, &p_allocation, &info_stats);
	RMLUI_VK_ASSERTMSG(status == VkResult::VK_SUCCESS, "failed to vmaCreateBuffer");

#ifdef RMLUI_VK_DEBUG
	Rml::Log::Message(Rml::Log::LT_DEBUG, "Allocated buffer [%s]", FormatByteSize(info_stats.size).c_str());
#endif

	buffer_data_t result = {};
	result.m_p_vk_buffer = p_buffer;
	result.m_p_vma_allocation = p_allocation;

	return result;
}

void RenderInterface_VK::DestroyResource_StagingBuffer(const buffer_data_t& data) noexcept
{
	if (m_p_allocator)
	{
		if (data.m_p_vk_buffer && data.m_p_vma_allocation)
		{
			vmaDestroyBuffer(m_p_allocator, data.m_p_vk_buffer, data.m_p_vma_allocation);
		}
	}
}

void RenderInterface_VK::Destroy_Textures() noexcept
{
	for (auto& textures : m_pending_for_deletion_textures_by_frames)
	{
		for (texture_data_t* p_data : textures)
		{
			Destroy_Texture(*p_data);
			delete p_data;
		}

		textures.clear();
	}
}

void RenderInterface_VK::Destroy_Geometries() noexcept
{
	Update_PendingForDeletion_Geometries();
	m_memory_pool.Shutdown();
}

void RenderInterface_VK::Destroy_Texture(const texture_data_t& texture) noexcept
{
	RMLUI_VK_ASSERTMSG(m_p_allocator, "you must have initialized VmaAllocator");
	RMLUI_VK_ASSERTMSG(m_p_device, "you must have initialized VkDevice");

	if (texture.m_p_vma_allocation)
	{
		vmaDestroyImage(m_p_allocator, texture.m_p_vk_image, texture.m_p_vma_allocation);
		vkDestroyImageView(m_p_device, texture.m_p_vk_image_view, nullptr);

		VkDescriptorSet p_set = texture.m_p_vk_descriptor_set;

		if (p_set)
		{
			m_manager_descriptors.Free_Descriptors(m_p_device, &p_set);
		}
	}
}

void RenderInterface_VK::DestroyResourcesDependentOnSize() noexcept
{
	Destroy_Pipelines();
	DestroySwapchainFrameBuffers();
	DestroyRenderPass();
}

void RenderInterface_VK::DestroySwapchainImageViews() noexcept
{
	RMLUI_VK_ASSERTMSG(m_p_device, "[Vulkan] you must initialize VkDevice before calling this method");

	m_swapchain_images.clear();

	for (auto p_view : m_swapchain_image_views)
	{
		vkDestroyImageView(m_p_device, p_view, nullptr);
	}

	m_swapchain_image_views.clear();
}

void RenderInterface_VK::DestroySwapchainFrameBuffers() noexcept
{
	DestroySwapchainImageViews();

	Destroy_Texture(m_texture_depthstencil);
	m_texture_depthstencil.m_p_vk_image = nullptr;
	m_texture_depthstencil.m_p_vk_image_view = nullptr;

	for (auto p_frame_buffer : m_swapchain_frame_buffers)
	{
		vkDestroyFramebuffer(m_p_device, p_frame_buffer, nullptr);
	}

	m_swapchain_frame_buffers.clear();
}

void RenderInterface_VK::DestroyRenderPass() noexcept
{
	RMLUI_VK_ASSERTMSG(m_p_device, "you must have a valid VkDevice here");

	if (m_p_render_pass)
	{
		vkDestroyRenderPass(m_p_device, m_p_render_pass, nullptr);
		m_p_render_pass = nullptr;
	}
}

void RenderInterface_VK::Destroy_Pipelines() noexcept
{
	RMLUI_VK_ASSERTMSG(m_p_device, "must exist here");

	vkDestroyPipeline(m_p_device, m_p_pipeline_with_textures, nullptr);
	vkDestroyPipeline(m_p_device, m_p_pipeline_without_textures, nullptr);
	vkDestroyPipeline(m_p_device, m_p_pipeline_stencil_for_region_where_geometry_will_be_drawn, nullptr);
	vkDestroyPipeline(m_p_device, m_p_pipeline_stencil_for_regular_geometry_that_applied_to_region_with_textures, nullptr);
	vkDestroyPipeline(m_p_device, m_p_pipeline_stencil_for_regular_geometry_that_applied_to_region_without_textures, nullptr);
}

void RenderInterface_VK::DestroyDescriptorSets() noexcept {}

void RenderInterface_VK::DestroyPipelineLayout() noexcept {}

void RenderInterface_VK::DestroySamplers() noexcept
{
	RMLUI_VK_ASSERTMSG(m_p_device, "must exist here");
	vkDestroySampler(m_p_device, m_p_sampler_linear, nullptr);
}

void RenderInterface_VK::CreateRenderPass() noexcept
{
	RMLUI_VK_ASSERTMSG(m_p_device, "you must have a valid VkDevice here");

	Rml::Array<VkAttachmentDescription, 2> attachments = {};

	attachments[0].format = m_swapchain_format.format;
	attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	attachments[1].format = Get_SupportedDepthFormat();
	attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[1].finalLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

	RMLUI_VK_ASSERTMSG(attachments[1].format != VkFormat::VK_FORMAT_UNDEFINED,
		"can't obtain depth format, your device doesn't support depth/stencil operations");

	Rml::Array<VkAttachmentReference, 2> color_references;

	// swapchain
	color_references[0].attachment = 0;
	color_references[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// depth stencil
	color_references[1].attachment = 1;
	color_references[1].layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

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

	VkResult status = vkCreateRenderPass(m_p_device, &info, nullptr, &m_p_render_pass);

	RMLUI_VK_ASSERTMSG(status == VK_SUCCESS, "failed to vkCreateRenderPass");
}

void RenderInterface_VK::Wait() noexcept
{
	RMLUI_VK_ASSERTMSG(m_p_device, "you must initialize device");
	RMLUI_VK_ASSERTMSG(m_p_swapchain, "you must initialize swapchain");

	constexpr uint64_t kMaxUint64 = std::numeric_limits<uint64_t>::max();

	auto status =
		vkAcquireNextImageKHR(m_p_device, m_p_swapchain, kMaxUint64, m_semaphores_image_available[m_semaphore_index], nullptr, &m_image_index);
	RMLUI_VK_ASSERTMSG(status == VkResult::VK_SUCCESS, "failed to vkAcquireNextImageKHR (see status)");

	m_semaphore_index_previous = m_semaphore_index;
	m_semaphore_index = ((m_semaphore_index + 1) % kSwapchainBackBufferCount);

	status = vkWaitForFences(m_p_device, 1, &m_executed_fences[m_semaphore_index_previous], VK_TRUE, kMaxUint64);
	RMLUI_VK_ASSERTMSG(status == VkResult::VK_SUCCESS, "failed to vkWaitForFences (see status)");

	status = vkResetFences(m_p_device, 1, &m_executed_fences[m_semaphore_index_previous]);
	RMLUI_VK_ASSERTMSG(status == VkResult::VK_SUCCESS, "failed to vkResetFences (see status)");
}

void RenderInterface_VK::Update_PendingForDeletion_Textures_By_Frames() noexcept
{
	auto& textures_for_previous_frame = m_pending_for_deletion_textures_by_frames[m_semaphore_index_previous];

	for (texture_data_t* p_data : textures_for_previous_frame)
	{
		Destroy_Texture(*p_data);
		delete p_data;
	}

	textures_for_previous_frame.clear();
}

void RenderInterface_VK::Update_PendingForDeletion_Geometries() noexcept
{
	for (geometry_handle_t* p_geometry_handle : m_pending_for_deletion_geometries)
	{
		m_memory_pool.Free_GeometryHandle(p_geometry_handle);
		delete p_geometry_handle;
	}

	m_pending_for_deletion_geometries.clear();
}

void RenderInterface_VK::Submit() noexcept
{
	const VkSemaphore p_semaphores_wait[] = {m_semaphores_image_available[m_semaphore_index]};
	const VkSemaphore p_semaphores_signal[] = {m_semaphores_finished_render[m_semaphore_index]};

	VkFence p_fence = m_executed_fences[m_semaphore_index];

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
	info.pCommandBuffers = &m_p_current_command_buffer;

	VkResult status = vkQueueSubmit(m_p_queue_graphics, 1, &info, p_fence);

	RMLUI_VK_ASSERTMSG(status == VK_SUCCESS, "failed to vkQueueSubmit");
}

void RenderInterface_VK::Present() noexcept
{
	VkPresentInfoKHR info = {};

	info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	info.pNext = nullptr;
	info.waitSemaphoreCount = 1;
	info.pWaitSemaphores = &(m_semaphores_finished_render[m_semaphore_index]);
	info.swapchainCount = 1;
	info.pSwapchains = &m_p_swapchain;
	info.pImageIndices = &m_image_index;
	info.pResults = nullptr;

	VkResult status = vkQueuePresentKHR(m_p_queue_present, &info);

	if (!(status == VK_SUCCESS))
	{
		if (status == VK_ERROR_OUT_OF_DATE_KHR || status == VK_SUBOPTIMAL_KHR)
		{
			RecreateSwapchain();
		}
		else
		{
			RMLUI_VK_ASSERTMSG(status == VK_SUCCESS, "failed to vkQueuePresentKHR");
		}
	}
}

VkFormat RenderInterface_VK::Get_SupportedDepthFormat()
{
	RMLUI_VK_ASSERTMSG(m_p_physical_device, "you must initialize and pick physical device for your renderer");

	Rml::Array<VkFormat, 5> formats = {VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT,
		VK_FORMAT_D16_UNORM};

	VkFormatProperties properties;
	for (const auto& format : formats)
	{
		vkGetPhysicalDeviceFormatProperties(m_p_physical_device, format, &properties);

		if (properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
		{
			return format;
		}
	}

	return VkFormat::VK_FORMAT_UNDEFINED;
}

RenderInterface_VK::CommandBufferRing::CommandBufferRing() : m_p_device{}, m_frame_index{}, m_p_current_frame{}, m_frames{} {}

void RenderInterface_VK::CommandBufferRing::Initialize(VkDevice p_device, uint32_t queue_index_graphics) noexcept
{
	RMLUI_VK_ASSERTMSG(p_device, "you can't pass an invalid VkDevice here");
	RMLUI_VK_ASSERTMSG(!m_p_device, "already initialized");

	m_p_device = p_device;

	for (CommandBuffersPerFrame& current_buffer : m_frames)
	{
		for (uint32_t command_buffer_index = 0; command_buffer_index < kNumCommandBuffersPerFrame; ++command_buffer_index)
		{
			VkCommandPoolCreateInfo info_pool = {};
			info_pool.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			info_pool.pNext = nullptr;
			info_pool.queueFamilyIndex = queue_index_graphics;
			info_pool.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

			VkCommandPool p_pool = nullptr;
			auto status = vkCreateCommandPool(p_device, &info_pool, nullptr, &p_pool);
			RMLUI_VK_ASSERTMSG(status == VkResult::VK_SUCCESS, "can't create command pool");

			current_buffer.m_command_pools[command_buffer_index] = p_pool;

			VkCommandBufferAllocateInfo info_buffer = {};
			info_buffer.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			info_buffer.pNext = nullptr;
			info_buffer.commandPool = p_pool;
			info_buffer.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			info_buffer.commandBufferCount = 1;

			VkCommandBuffer p_buffer = nullptr;
			status = vkAllocateCommandBuffers(p_device, &info_buffer, &p_buffer);
			RMLUI_VK_ASSERTMSG(status == VkResult::VK_SUCCESS, "failed to fill command buffers");

			current_buffer.m_command_buffers[command_buffer_index] = p_buffer;
		}
	}

	m_frame_index = 0;
	m_p_current_frame = &m_frames[m_frame_index];
}

void RenderInterface_VK::CommandBufferRing::Shutdown()
{
	RMLUI_VK_ASSERTMSG(m_p_device, "you can't have an uninitialized VkDevice");

	for (CommandBuffersPerFrame& current_buffer : m_frames)
	{
		for (uint32_t i = 0; i < kNumCommandBuffersPerFrame; ++i)
		{
			vkFreeCommandBuffers(m_p_device, current_buffer.m_command_pools[i], 1, &current_buffer.m_command_buffers[i]);
			vkDestroyCommandPool(m_p_device, current_buffer.m_command_pools[i], nullptr);
		}
	}
}

void RenderInterface_VK::CommandBufferRing::OnBeginFrame()
{
	m_frame_index = ((m_frame_index + 1) % kNumFramesToBuffer);
	m_p_current_frame = &m_frames[m_frame_index];

	// Reset all command pools of the current frame.
	for (VkCommandPool command_pool : m_p_current_frame->m_command_pools)
	{
		auto status = vkResetCommandPool(m_p_device, command_pool, 0);
		RMLUI_VK_ASSERTMSG(status == VkResult::VK_SUCCESS, "failed to vkResetCommandPool");
	}
}

VkCommandBuffer RenderInterface_VK::CommandBufferRing::GetCommandBufferForActiveFrame(CommandBufferName named_command_buffer)
{
	RMLUI_VK_ASSERTMSG(m_p_current_frame, "must be valid");
	RMLUI_VK_ASSERTMSG(m_p_device, "you must initialize your VkDevice field with valid pointer or it's uninitialized field");
	RMLUI_VK_ASSERTMSG((int)named_command_buffer < (int)CommandBufferName::Count, "overflow, please use one of the named command lists");

	const uint32_t list_index = static_cast<uint32_t>(named_command_buffer);

	VkCommandBuffer result = m_p_current_frame->m_command_buffers[list_index];
	RMLUI_VK_ASSERTMSG(result, "your VkCommandBuffer must be valid otherwise debug your command list class for frame");

	return result;
}

RenderInterface_VK::MemoryPool::MemoryPool() :
	m_memory_total_size{}, m_device_min_uniform_alignment{}, m_p_data{}, m_p_buffer{}, m_p_buffer_alloc{}, m_p_device{}, m_p_vk_allocator{},
	m_p_block{}
{}

RenderInterface_VK::MemoryPool::~MemoryPool() {}

void RenderInterface_VK::MemoryPool::Initialize(VkDeviceSize byte_size, VkDeviceSize device_min_uniform_alignment, VmaAllocator p_allocator,
	VkDevice p_device) noexcept
{
	RMLUI_VK_ASSERTMSG(byte_size > 0, "size must be valid");
	RMLUI_VK_ASSERTMSG(device_min_uniform_alignment > 0, "uniform alignment must be valid");
	RMLUI_VK_ASSERTMSG(p_device, "you must pass a valid VkDevice");
	RMLUI_VK_ASSERTMSG(p_allocator, "you must pass a valid VmaAllocator");

	m_p_device = p_device;
	m_p_vk_allocator = p_allocator;
	m_device_min_uniform_alignment = device_min_uniform_alignment;

#ifdef RMLUI_VK_DEBUG
	Rml::Log::Message(Rml::Log::LT_DEBUG, "[Vulkan][Debug] the alignment for uniform buffer is: %zu", m_device_min_uniform_alignment);
#endif

	m_memory_total_size = AlignUp<VkDeviceSize>(static_cast<VkDeviceSize>(byte_size), m_device_min_uniform_alignment);

	VkBufferCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	info.size = m_memory_total_size;

	VmaAllocationCreateInfo info_alloc = {};

	auto p_commentary = "our pool buffer that manages all memory in vulkan (dynamic)";

	info_alloc.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
	info_alloc.flags = VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
	info_alloc.pUserData = const_cast<char*>(p_commentary);

	VmaAllocationInfo info_stats = {};

	auto status = vmaCreateBuffer(m_p_vk_allocator, &info, &info_alloc, &m_p_buffer, &m_p_buffer_alloc, &info_stats);

	RMLUI_VK_ASSERTMSG(status == VkResult::VK_SUCCESS, "failed to vmaCreateBuffer");

	VmaVirtualBlockCreateInfo info_virtual_block = {};
	info_virtual_block.size = m_memory_total_size;

	status = vmaCreateVirtualBlock(&info_virtual_block, &m_p_block);

	RMLUI_VK_ASSERTMSG(status == VkResult::VK_SUCCESS, "failed to vmaCreateVirtualBlock");

#ifdef RMLUI_VK_DEBUG
	Rml::Log::Message(Rml::Log::LT_DEBUG, "[Vulkan][Debug] Allocated memory pool [%s]", FormatByteSize(info_stats.size).c_str());
#endif

	status = vmaMapMemory(m_p_vk_allocator, m_p_buffer_alloc, (void**)&m_p_data);

	RMLUI_VK_ASSERTMSG(status == VkResult::VK_SUCCESS, "failed to vmaMapMemory");
}

void RenderInterface_VK::MemoryPool::Shutdown() noexcept
{
	RMLUI_VK_ASSERTMSG(m_p_vk_allocator, "you must have a valid VmaAllocator");
	RMLUI_VK_ASSERTMSG(m_p_buffer, "you must allocate VkBuffer for deleting");
	RMLUI_VK_ASSERTMSG(m_p_buffer_alloc, "you must allocate VmaAllocation for deleting");

#ifdef RMLUI_VK_DEBUG
	Rml::Log::Message(Rml::Log::LT_DEBUG, "[Vulkan][Debug] Destroyed memory pool [%s]", FormatByteSize(m_memory_total_size).c_str());
#endif

	vmaUnmapMemory(m_p_vk_allocator, m_p_buffer_alloc);
	vmaDestroyVirtualBlock(m_p_block);
	vmaDestroyBuffer(m_p_vk_allocator, m_p_buffer, m_p_buffer_alloc);
}

bool RenderInterface_VK::MemoryPool::Alloc_GeneralBuffer(VkDeviceSize size, void** p_data, VkDescriptorBufferInfo* p_out,
	VmaVirtualAllocation* p_alloc) noexcept
{
	RMLUI_VK_ASSERTMSG(p_out, "you must pass a valid pointer");
	RMLUI_VK_ASSERTMSG(m_p_buffer, "you must have a valid VkBuffer");

	RMLUI_VK_ASSERTMSG(*p_alloc == nullptr,
		"you can't pass a VALID object, because it is for initialization. So it means you passed the already allocated "
		"VmaVirtualAllocation and it means you did something wrong, like you wanted to allocate into the same object...");

	size = AlignUp<VkDeviceSize>(static_cast<VkDeviceSize>(size), m_device_min_uniform_alignment);

	VkDeviceSize offset_memory{};

	VmaVirtualAllocationCreateInfo info = {};
	info.size = size;
	info.alignment = m_device_min_uniform_alignment;

	auto status = vmaVirtualAllocate(m_p_block, &info, p_alloc, &offset_memory);

	RMLUI_VK_ASSERTMSG(status == VkResult::VK_SUCCESS, "failed to vmaVirtualAllocate");

	*p_data = (void*)(m_p_data + offset_memory);

	p_out->buffer = m_p_buffer;
	p_out->offset = offset_memory;
	p_out->range = size;

	return true;
}

bool RenderInterface_VK::MemoryPool::Alloc_VertexBuffer(uint32_t number_of_elements, uint32_t stride_in_bytes, void** p_data,
	VkDescriptorBufferInfo* p_out, VmaVirtualAllocation* p_alloc) noexcept
{
	return Alloc_GeneralBuffer(number_of_elements * stride_in_bytes, p_data, p_out, p_alloc);
}

bool RenderInterface_VK::MemoryPool::Alloc_IndexBuffer(uint32_t number_of_elements, uint32_t stride_in_bytes, void** p_data,
	VkDescriptorBufferInfo* p_out, VmaVirtualAllocation* p_alloc) noexcept
{
	return Alloc_GeneralBuffer(number_of_elements * stride_in_bytes, p_data, p_out, p_alloc);
}

void RenderInterface_VK::MemoryPool::SetDescriptorSet(uint32_t binding_index, uint32_t size, VkDescriptorType descriptor_type,
	VkDescriptorSet p_set) noexcept
{
	RMLUI_VK_ASSERTMSG(m_p_device, "you must have a valid VkDevice here");
	RMLUI_VK_ASSERTMSG(p_set, "you must have a valid VkDescriptorSet here");
	RMLUI_VK_ASSERTMSG(m_p_buffer, "you must have a valid VkBuffer here");

	VkDescriptorBufferInfo info = {};

	info.buffer = m_p_buffer;
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

	vkUpdateDescriptorSets(m_p_device, 1, &info_write, 0, nullptr);
}

void RenderInterface_VK::MemoryPool::SetDescriptorSet(uint32_t binding_index, VkDescriptorBufferInfo* p_info, VkDescriptorType descriptor_type,
	VkDescriptorSet p_set) noexcept
{
	RMLUI_VK_ASSERTMSG(m_p_device, "you must have a valid VkDevice here");
	RMLUI_VK_ASSERTMSG(p_set, "you must have a valid VkDescriptorSet here");
	RMLUI_VK_ASSERTMSG(m_p_buffer, "you must have a valid VkBuffer here");
	RMLUI_VK_ASSERTMSG(p_info, "must be valid pointer");

	VkWriteDescriptorSet info_write = {};

	info_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	info_write.pNext = nullptr;
	info_write.dstSet = p_set;
	info_write.descriptorCount = 1;
	info_write.descriptorType = descriptor_type;
	info_write.dstArrayElement = 0;
	info_write.dstBinding = binding_index;
	info_write.pBufferInfo = p_info;

	vkUpdateDescriptorSets(m_p_device, 1, &info_write, 0, nullptr);
}

void RenderInterface_VK::MemoryPool::SetDescriptorSet(uint32_t binding_index, VkSampler p_sampler, VkImageLayout layout, VkImageView p_view,
	VkDescriptorType descriptor_type, VkDescriptorSet p_set) noexcept
{
	RMLUI_VK_ASSERTMSG(m_p_device, "you must have a valid VkDevice here");
	RMLUI_VK_ASSERTMSG(p_set, "you must have a valid VkDescriptorSet here");
	RMLUI_VK_ASSERTMSG(m_p_buffer, "you must have a valid VkBuffer here");
	RMLUI_VK_ASSERTMSG(p_view, "you must have a valid VkImageView");
	RMLUI_VK_ASSERTMSG(p_sampler, "you must have a valid VkSampler here");

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

	vkUpdateDescriptorSets(m_p_device, 1, &info_write, 0, nullptr);
}

void RenderInterface_VK::MemoryPool::Free_GeometryHandle(geometry_handle_t* p_valid_geometry_handle) noexcept
{
	RMLUI_VK_ASSERTMSG(p_valid_geometry_handle,
		"you must pass a VALID pointer to geometry_handle_t, otherwise something is wrong and debug your code");
	RMLUI_VK_ASSERTMSG(p_valid_geometry_handle->m_p_vertex_allocation, "you must have a VALID pointer of VmaAllocation for vertex buffer");
	RMLUI_VK_ASSERTMSG(p_valid_geometry_handle->m_p_index_allocation, "you must have a VALID pointer of VmaAllocation for index buffer");

	// TODO: The following assertion is disabled for now. The shader allocation pointer is only set once the geometry
	// handle is rendered with. However, currently the Vulkan renderer does not handle all draw calls from RmlUi, so
	// this pointer may never be set if the geometry was only used in a unsupported draw calls. This can then trigger
	// the following assertion. The free call below gracefully handles zero pointers so this should be safe regardless.
	// RMLUI_VK_ASSERTMSG(p_valid_geometry_handle->m_p_shader_allocation,
	//		"you must have a VALID pointer of VmaAllocation for shader operations (like uniforms and etc)");

	RMLUI_VK_ASSERTMSG(m_p_block, "you have to allocate the virtual block before do this operation...");

	vmaVirtualFree(m_p_block, p_valid_geometry_handle->m_p_vertex_allocation);
	vmaVirtualFree(m_p_block, p_valid_geometry_handle->m_p_index_allocation);
	vmaVirtualFree(m_p_block, p_valid_geometry_handle->m_p_shader_allocation);

	p_valid_geometry_handle->m_p_vertex_allocation = nullptr;
	p_valid_geometry_handle->m_p_shader_allocation = nullptr;
	p_valid_geometry_handle->m_p_index_allocation = nullptr;
	p_valid_geometry_handle->m_num_indices = 0;
}

void RenderInterface_VK::MemoryPool::Free_GeometryHandle_ShaderDataOnly(geometry_handle_t* p_valid_geometry_handle) noexcept
{
	RMLUI_VK_ASSERTMSG(p_valid_geometry_handle,
		"you must pass a VALID pointer to geometry_handle_t, otherwise something is wrong and debug your code");
	RMLUI_VK_ASSERTMSG(p_valid_geometry_handle->m_p_vertex_allocation, "you must have a VALID pointer of VmaAllocation for vertex buffer");
	RMLUI_VK_ASSERTMSG(p_valid_geometry_handle->m_p_index_allocation, "you must have a VALID pointer of VmaAllocation for index buffer");
	RMLUI_VK_ASSERTMSG(p_valid_geometry_handle->m_p_shader_allocation,
		"you must have a VALID pointer of VmaAllocation for shader operations (like uniforms and etc)");
	RMLUI_VK_ASSERTMSG(m_p_block, "you have to allocate the virtual block before do this operation...");

	vmaVirtualFree(m_p_block, p_valid_geometry_handle->m_p_shader_allocation);
	p_valid_geometry_handle->m_p_shader_allocation = nullptr;
}

#define GLAD_VULKAN_IMPLEMENTATION
#define VMA_IMPLEMENTATION
#include "RmlUi_Include_Vulkan.h"
