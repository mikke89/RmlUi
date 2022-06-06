#include "ShellRenderInterfaceVulkan.h"
#include "ShellFileInterface.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#define VK_ASSERT(statement, msg, ...)             \
	{                                              \
		RMLUI_ASSERT(statement);                   \
		if (!!(statement) == false)                \
			Shell::DisplayError(msg, __VA_ARGS__); \
	}

VkValidationFeaturesEXT debug_validation_features_ext = {};
VkValidationFeatureEnableEXT debug_validation_features_ext_requested[] = {VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
	VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT, VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT};

static VKAPI_ATTR VkBool32 VKAPI_CALL MyDebugReportCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object,
	size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData)
{
	if (messageCode & VkDebugReportFlagBitsEXT::VK_DEBUG_REPORT_INFORMATION_BIT_EXT ||
		messageCode & VkDebugReportFlagBitsEXT::VK_DEBUG_REPORT_DEBUG_BIT_EXT)
	{
		return VK_FALSE;
	}

	Shell::Log("[Vulkan][VALIDATION] %s ", pMessage);

	return VK_FALSE;
}

ShellRenderInterfaceVulkan::ShellRenderInterfaceVulkan() :
	m_is_transform_enabled(false), m_width{}, m_height{}, m_queue_index_present{}, m_queue_index_graphics{}, m_queue_index_compute{},
	m_semaphore_index{}, m_semaphore_index_previous{}, m_image_index{}, m_current_descriptor_id{}, m_p_instance{}, m_p_device{},
	m_p_physical_device_current{}, m_p_surface{}, m_p_swapchain{}, m_p_window_handle{}, m_p_queue_present{}, m_p_queue_graphics{},
	m_p_queue_compute{}, m_p_descriptor_set_layout{}, m_p_pipeline_layout{}, m_p_pipeline_with_textures{}, m_p_pipeline_without_textures{},
	m_p_descriptor_set{}, m_p_render_pass{}, m_p_sampler_nearest{}, m_p_allocator{}, m_p_current_command_buffer{}
{}

ShellRenderInterfaceVulkan::~ShellRenderInterfaceVulkan(void) {}

void ShellRenderInterfaceVulkan::RenderGeometry(
	Rml::Vertex* vertices, int num_vertices, int* indices, int num_indices, Rml::TextureHandle texture, const Rml::Vector2f& translation)
{
	Rml::CompiledGeometryHandle handle = this->CompileGeometry(vertices, num_vertices, indices, num_indices, texture);

	if (handle)
	{
		this->RenderCompiledGeometry(handle, translation);
		this->ReleaseCompiledGeometry(handle);
	}
}

// TODO: RMLUI team it is important because it affects the architecture, when we do resize WE HAVE TO UPDATE ONLY THOSE geometry_handle_ts that need
// to rebuild!!! So when it comes to "rebuild" our geometry on resize we need change already existed instances of geometry_handle_t otherwise we just
// add new and old doesn't update at all. I won't change anything with resize so it is technically doesn't work. (like maximize window and vice versa)
Rml::CompiledGeometryHandle ShellRenderInterfaceVulkan::CompileGeometry(
	Rml::Vertex* vertices, int num_vertices, int* indices, int num_indices, Rml::TextureHandle texture)
{
	texture_data_t* p_texture = reinterpret_cast<texture_data_t*>(texture);

	VkDescriptorSet p_current_descriptor_set = nullptr;

	if (this->m_descriptor_sets.empty() == false)
	{
		p_current_descriptor_set = this->Get_DescriptorSet(this->Get_CurrentDescriptorID());
	}
	else
	{
		p_current_descriptor_set = this->m_p_descriptor_set;
	}

	VK_ASSERT(p_current_descriptor_set, "you can't have here an invalid pointer of VkDescriptorSet. Two reason might be. 1. - you didn't allocate it "
										"at all or 2. - Somehing is wrong with allocation and somehow it was corrupted by something.");

	VK_ASSERT(this->m_compiled_geometries.size() < kGeometryForReserve,
		"if it is greater than the constant (like assert is triggered), it means that the added element and all operations with other elements "
		"becomes INVALID, because the "
		"erase operation will cause invalidation for all references and pointers that used from that map. (like pointer to value of the map). So "
		"just set the value higher than it was before...");

	VK_ASSERT(this->m_compiled_geometries.find(this->Get_CurrentDescriptorID()) == this->m_compiled_geometries.end(),
		"you must delete the element before construct it");

	this->m_compiled_geometries[this->Get_CurrentDescriptorID()];

	auto& current_geometry_handle = this->m_compiled_geometries.at(this->Get_CurrentDescriptorID());

	VkDescriptorImageInfo info_descriptor_image = {};
	if (p_texture)
	{
		info_descriptor_image.imageView = p_texture->Get_VkImageView();
		info_descriptor_image.sampler = p_texture->Get_VkSampler();
		info_descriptor_image.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkWriteDescriptorSet info_write = {};

		info_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		info_write.dstSet = p_current_descriptor_set;
		info_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		info_write.dstBinding = 2;
		info_write.pImageInfo = &info_descriptor_image;
		info_write.descriptorCount = 1;

		vkUpdateDescriptorSets(this->m_p_device, 1, &info_write, 0, nullptr);
	}

	uint32_t* pCopyDataToBuffer = nullptr;
	const void* pData = reinterpret_cast<const void*>(vertices);

	bool status = this->m_memory_pool.Alloc_VertexBuffer(num_vertices, sizeof(Rml::Vertex), reinterpret_cast<void**>(&pCopyDataToBuffer),
		&current_geometry_handle.m_p_vertex, &current_geometry_handle.m_p_vertex_allocation);
	VK_ASSERT(status, "failed to AllocVertexBuffer");

	memcpy(pCopyDataToBuffer, pData, sizeof(Rml::Vertex) * num_vertices);

	status = this->m_memory_pool.Alloc_IndexBuffer(num_indices, sizeof(int), reinterpret_cast<void**>(&pCopyDataToBuffer),
		&current_geometry_handle.m_p_index, &current_geometry_handle.m_p_index_allocation);
	VK_ASSERT(status, "failed to AllocIndexBuffer");

	memcpy(pCopyDataToBuffer, indices, sizeof(int) * num_indices);

	// TODO: RmlUI team checks if it is right logic that if we don't have texture the handle is NULL/0 otherwise provide in places where a such
	// (Rml::TextureHandle)(nullptr) is more obvious and strict, because it is probably important thing. Because I am not sure if it is always valid
	// if you have callings from different places not only from this class like LoadTexture function, but GenerateTexture can be called somewhere.
	// Keep this in mind;
	current_geometry_handle.m_is_has_texture = !!((texture_data_t*)(texture));
	current_geometry_handle.m_num_indices = num_indices;
	current_geometry_handle.m_descriptor_id = this->Get_CurrentDescriptorID();
	current_geometry_handle.m_is_cached = false;

#ifdef RMLUI_DEBUG
	Shell::Log("[Vulkan][Debug] created descriptor id:[%d]", current_geometry_handle.m_descriptor_id);
#endif

	this->NextDescriptorID();

	return Rml::CompiledGeometryHandle(&current_geometry_handle);
}

void ShellRenderInterfaceVulkan::RenderCompiledGeometry(Rml::CompiledGeometryHandle geometry, const Rml::Vector2f& translation)
{
	VK_ASSERT(this->m_p_current_command_buffer, "must be valid otherwise you can't render now!!! (can't be)");

	geometry_handle_t* p_casted_compiled_geometry = reinterpret_cast<geometry_handle_t*>(geometry);

	this->m_user_data_for_vertex_shader.m_translate = translation;

	// TODO: RmlUI team somehow but on resize I got invalid value here...
	VkDescriptorSet p_current_descriptor_set = this->Get_DescriptorSet(p_casted_compiled_geometry->m_descriptor_id);

	VK_ASSERT(p_current_descriptor_set, "you can't have here an invalid pointer of VkDescriptorSet. Two reason might be. 1. - you didn't allocate it "
										"at all or 2. - Somehing is wrong with allocation and somehow it was corrupted by something.");

	// we don't need to write the same commands to buffer or update descritor set somehow because it was already passed to it!!!!!
	// don't do repetetive and pointless callings!!!!
	if (!p_casted_compiled_geometry->m_is_cached)
	{
		uint32_t* pCopyDataToBuffer = nullptr;

		shader_vertex_user_data_t* p_data = nullptr;

		bool status = this->m_memory_pool.Alloc_GeneralBuffer(sizeof(this->m_user_data_for_vertex_shader), reinterpret_cast<void**>(&p_data),
			&p_casted_compiled_geometry->m_p_shader, &p_casted_compiled_geometry->m_p_shader_allocation);
		VK_ASSERT(status, "failed to allocate VkDescriptorBufferInfo for uniform data to shaders");

		p_data->m_transform = this->m_user_data_for_vertex_shader.m_transform;
		p_data->m_translate = this->m_user_data_for_vertex_shader.m_translate;

		if (this->m_descriptor_sets.empty() == false)
			this->m_memory_pool.SetDescriptorSet(
				1, &p_casted_compiled_geometry->m_p_shader, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, p_current_descriptor_set);

		// that's a final destination where the descriptor will be not available for updating...
		if (this->m_descriptor_sets.empty() == false)
			this->m_descriptor_sets.at(p_casted_compiled_geometry->m_descriptor_id).Set_Available(false);
	}

	uint32_t casted_offset = 0;
	int num_uniform_offset = 0;

	if (p_casted_compiled_geometry->m_p_shader.buffer)
	{
		num_uniform_offset = 1;
		casted_offset = (uint32_t)p_casted_compiled_geometry->m_p_shader.offset;
	}

	const uint32_t pDescriptorOffsets = static_cast<uint32_t>(p_casted_compiled_geometry->m_p_shader.offset);
	vkCmdBindDescriptorSets(this->m_p_current_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->m_p_pipeline_layout, 0, 1,
		&p_current_descriptor_set, 1, &pDescriptorOffsets);

	if (p_casted_compiled_geometry->m_is_has_texture)
	{
		vkCmdBindPipeline(this->m_p_current_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->m_p_pipeline_with_textures);
	}
	else
	{
		vkCmdBindPipeline(this->m_p_current_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->m_p_pipeline_without_textures);
	}

	// means the size of array is one (I say about []), but it is not about indexing like my_std_vector[real_positioning_from_language_perspective]
	// like it is intuitive 0, but it is 1
	VkDeviceSize offsets[1] = {0};

	vkCmdBindVertexBuffers(
		this->m_p_current_command_buffer, 0, 1, &p_casted_compiled_geometry->m_p_vertex.buffer, &p_casted_compiled_geometry->m_p_vertex.offset);

	vkCmdBindIndexBuffer(this->m_p_current_command_buffer, p_casted_compiled_geometry->m_p_index.buffer, p_casted_compiled_geometry->m_p_index.offset,
		VK_INDEX_TYPE_UINT32);

	vkCmdDrawIndexed(this->m_p_current_command_buffer, p_casted_compiled_geometry->m_num_indices, 1, 0, 0, 0);

	if (p_casted_compiled_geometry->m_is_cached == false)
		p_casted_compiled_geometry->m_is_cached = true;
}

void ShellRenderInterfaceVulkan::ReleaseCompiledGeometry(Rml::CompiledGeometryHandle geometry)
{
	geometry_handle_t* p_casted_geometry = reinterpret_cast<geometry_handle_t*>(geometry);
	this->m_memory_pool.Free_GeometryHandle(p_casted_geometry);

	if (this->m_descriptor_sets.empty() == false) 
	{
		this->m_compiled_geometries.erase(p_casted_geometry->m_descriptor_id);
		this->NextDescriptorID();
	}
}

void ShellRenderInterfaceVulkan::EnableScissorRegion(bool enable) {}

void ShellRenderInterfaceVulkan::SetScissorRegion(int x, int y, int width, int height) {}

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

bool ShellRenderInterfaceVulkan::LoadTexture(Rml::TextureHandle& texture_handle, Rml::Vector2i& texture_dimensions, const Rml::String& source)
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

	// TODO: RmlUI team, possibly better to pass source into generate texture too
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
bool ShellRenderInterfaceVulkan::GenerateTexture(Rml::TextureHandle& texture_handle, const Rml::byte* source, const Rml::Vector2i& source_dimensions)
{
	VK_ASSERT(source, "you pushed not valid data for copying to buffer");
	VK_ASSERT(this->m_p_allocator, "you have to initialize Vma Allocator for this method");

	int width = source_dimensions.x;
	int height = source_dimensions.y;

	VK_ASSERT(width, "invalid width");
	VK_ASSERT(height, "invalid height");

	const char* file_path = reinterpret_cast<const char*>(texture_handle);

	this->m_textures.push_back(texture_data_t());

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

	auto& texture = this->m_textures.back();

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

	Shell::Log("Created image %s with size [%d bytes][%d Megabytes]", file_path, info_stats.size, TranslateBytesToMegaBytes(info_stats.size));

	texture.Set_FileName(file_path);
	texture.Set_VkImage(p_image);
	texture.Set_VmaAllocation(p_allocation);
	texture.Set_Width(width);
	texture.Set_Height(height);

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

			vkCmdPipelineBarrier(
				p_cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &info_barrier);

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
	info_image_view.image = texture.Get_VkImage();
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

	texture.Set_VkImageView(p_image_view);
	texture.Set_VkSampler(this->m_p_sampler_nearest);

	texture_handle = (Rml::TextureHandle)(&texture);

	return true;
}

void ShellRenderInterfaceVulkan::ReleaseTexture(Rml::TextureHandle texture_handle)
{
	// TODO: implement deleting textures
}

void ShellRenderInterfaceVulkan::SetTransform(const Rml::Matrix4f* transform)
{
	this->m_user_data_for_vertex_shader.m_transform = this->m_projection * (transform ? *transform : Rml::Matrix4f::Identity());
}

void ShellRenderInterfaceVulkan::SetViewport(int width, int height)
{
	this->OnResize(width, height);
}

bool ShellRenderInterfaceVulkan::AttachToNative(void* nativeWindow)
{
	this->m_p_window_handle = reinterpret_cast<HWND>(nativeWindow);

	this->Initialize();

	return true;
}

void ShellRenderInterfaceVulkan::DetachFromNative(void)
{
	this->Shutdown();
}

void ShellRenderInterfaceVulkan::PrepareRenderBuffer(void)
{
	this->m_memory_pool.OnBeginFrame();
	this->m_command_list.OnBeginFrame();
	this->Wait();

	this->m_p_current_command_buffer = this->m_command_list.GetNewCommandList();

	VkCommandBufferBeginInfo info = {};

	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	info.pInheritanceInfo = nullptr;
	info.pNext = nullptr;
	info.flags = VkCommandBufferUsageFlagBits::VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	auto status = vkBeginCommandBuffer(this->m_p_current_command_buffer, &info);

	VK_ASSERT(status == VkResult::VK_SUCCESS, "failed to vkBeginCommandBuffer");

	VkClearValue for_filling_back_buffer_color;

	for_filling_back_buffer_color.color = {0.0f, 0.0f, 0.0f, 1.0f};

	const VkClearValue p_color_rt[] = {for_filling_back_buffer_color};

	VkRenderPassBeginInfo info_pass = {};

	info_pass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	info_pass.pNext = nullptr;
	info_pass.renderPass = this->m_p_render_pass;
	info_pass.framebuffer = this->m_swapchain_frame_buffers.at(this->m_image_index);
	info_pass.pClearValues = p_color_rt;
	info_pass.clearValueCount = 1;
	info_pass.renderArea.offset.x = 0;
	info_pass.renderArea.offset.y = 0;
	info_pass.renderArea.extent.width = this->m_width;
	info_pass.renderArea.extent.height = this->m_height;

	vkCmdBeginRenderPass(this->m_p_current_command_buffer, &info_pass, VkSubpassContents::VK_SUBPASS_CONTENTS_INLINE);

	vkCmdSetScissor(this->m_p_current_command_buffer, 0, 1, &this->m_scissor);
	vkCmdSetViewport(this->m_p_current_command_buffer, 0, 1, &this->m_viewport);
}

void ShellRenderInterfaceVulkan::PresentRenderBuffer(void)
{
	vkCmdEndRenderPass(this->m_p_current_command_buffer);

	auto status = vkEndCommandBuffer(this->m_p_current_command_buffer);

	VK_ASSERT(status == VkResult::VK_SUCCESS, "failed to vkEndCommandBuffer");

	this->Submit();
	this->Present();

	this->m_p_current_command_buffer = nullptr;
}

void ShellRenderInterfaceVulkan::Initialize(void) noexcept
{
	this->Initialize_Instance();
	this->Initialize_PhysicalDevice();
	this->Initialize_Surface();
	this->Initialize_QueueIndecies();
	this->Initialize_Device();
	this->Initialize_Queues();
	this->Initialize_SyncPrimitives();
	this->Initialize_Allocator();
	this->Initialize_Resources();
}

void ShellRenderInterfaceVulkan::Shutdown(void) noexcept
{
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
}

void ShellRenderInterfaceVulkan::OnResize(int width, int height) noexcept
{
	auto status = vkDeviceWaitIdle(this->m_p_device);

	VK_ASSERT(status == VkResult::VK_SUCCESS, "failed to vkDeviceWaitIdle");

	this->m_width = width;
	this->m_height = height;

#ifdef RMLUI_PLATFORM_WIN32
	// TODO: rmlui team try to call OnResize method only in case when the windows is shown, all other iterations (you can try to delete this if block
	// and see the results of validation) cause validation error with invalid extent it means height with zero value and it is not acceptable
	if (!IsWindowVisible(this->m_p_window_handle))
	{
		return;
	}
#endif

	if (this->m_p_swapchain)
	{
		this->Destroy_Swapchain();
		this->DestroyResourcesDependentOnSize();
	}

	this->Initialize_Swapchain();
	this->CreateResourcesDependentOnSize();
}

void ShellRenderInterfaceVulkan::Initialize_Instance(void) noexcept
{
	uint32_t required_version = this->GetRequiredVersionAndValidateMachine();

	VkApplicationInfo info = {};

	info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	info.pNext = nullptr;
	info.pApplicationName = "RMLUI Shell";
	info.applicationVersion = 42;
	info.pEngineName = "RmlUi";
	info.apiVersion = required_version;

	this->CreatePropertiesFor_Instance();

	VkInstanceCreateInfo info_instance = {};

	info_instance.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	info_instance.pNext = &debug_validation_features_ext;
	info_instance.flags = 0;
	info_instance.pApplicationInfo = &info;
	info_instance.enabledExtensionCount = this->m_instance_extension_names.size();
	info_instance.ppEnabledExtensionNames = this->m_instance_extension_names.data();
	info_instance.enabledLayerCount = this->m_instance_layer_names.size();
	info_instance.ppEnabledLayerNames = this->m_instance_layer_names.data();

	VkResult status = vkCreateInstance(&info_instance, nullptr, &this->m_p_instance);

	VK_ASSERT(status == VK_SUCCESS, "failed to vkCreateInstance");

	this->CreateReportDebugCallback();
}

void ShellRenderInterfaceVulkan::Initialize_Device(void) noexcept
{
	this->CreatePropertiesFor_Device();
	this->AddExtensionToDevice(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	this->AddExtensionToDevice(VK_EXT_SCALAR_BLOCK_LAYOUT_EXTENSION_NAME);

	// this for working with negative values for VkViewport, because check this article
	// https://www.saschawillems.de/blog/2019/03/29/flipping-the-vulkan-viewport/#:~:text=The%20cause%20for%20this%20is,scene%20is%20rendered%20upside%20down.
	this->AddExtensionToDevice(VK_KHR_MAINTENANCE_1_EXTENSION_NAME);

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
	info_device.enabledExtensionCount = this->m_device_extension_names.size();
	info_device.ppEnabledExtensionNames = info_device.enabledExtensionCount ? this->m_device_extension_names.data() : nullptr;
	info_device.pEnabledFeatures = nullptr;

	VkResult status = vkCreateDevice(this->m_p_physical_device_current, &info_device, nullptr, &this->m_p_device);

	VK_ASSERT(status == VK_SUCCESS, "failed to vkCreateDevice");
}

void ShellRenderInterfaceVulkan::Initialize_PhysicalDevice(void) noexcept
{
	this->CollectPhysicalDevices();
	bool status = this->ChoosePhysicalDevice(VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);

	if (status == false)
	{
		Shell::Log("Failed to pick the discrete gpu, now trying to pick integrated GPU");
		status = this->ChoosePhysicalDevice(VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU);

		if (status == false)
		{
			Shell::Log("Failed to pick the integrated gpu, now trying to pick up the CPU");
			status = this->ChoosePhysicalDevice(VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_CPU);

			VK_ASSERT(status, "there's no suitable physical device for rendering, abort this application");
		}
	}

	this->PrintInformationAboutPickedPhysicalDevice(this->m_p_physical_device_current);

	vkGetPhysicalDeviceProperties(this->m_p_physical_device_current, &this->m_current_physical_device_properties);
}

void ShellRenderInterfaceVulkan::Initialize_Swapchain(void) noexcept
{
	VkSwapchainCreateInfoKHR info = {};

	this->m_swapchain_format = this->ChooseSwapchainFormat();

	info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	info.pNext = nullptr;
	info.surface = this->m_p_surface;
	info.imageFormat = this->m_swapchain_format.format;
	info.minImageCount = kSwapchainBackBufferCount;
	info.imageColorSpace = this->m_swapchain_format.colorSpace;
	info.imageExtent = this->CreateValidSwapchainExtent();
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

void ShellRenderInterfaceVulkan::Initialize_Surface(void) noexcept
{
	VK_ASSERT(this->m_p_instance, "you must initialize your VkInstance");

#if defined(RMLUI_PLATFORM_WIN32)
	VK_ASSERT(this->m_p_window_handle, "you must initialize your window before creating Surface!");

	VkWin32SurfaceCreateInfoKHR info = {};

	info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	info.hinstance = GetModuleHandle(NULL);
	info.hwnd = this->m_p_window_handle;

	VkResult status = vkCreateWin32SurfaceKHR(this->m_p_instance, &info, nullptr, &this->m_p_surface);

	VK_ASSERT(status == VK_SUCCESS, "failed to vkCreateWin32SurfaceKHR");
#else
	#error this platform doesn't support Vulkan!!!
#endif
}

void ShellRenderInterfaceVulkan::Initialize_QueueIndecies(void) noexcept
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

	if (this->m_queue_index_present == -1)
	{
		Shell::Log("[Vulkan] User doesn't have one index for two queues, so we need to find for present queue index");

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

	Shell::Log("[Vulkan] User family queues indecies: Graphics[%d] Present[%d] Compute[%d]", this->m_queue_index_graphics,
		this->m_queue_index_present, this->m_queue_index_compute);
}

void ShellRenderInterfaceVulkan::Initialize_Queues(void) noexcept
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

void ShellRenderInterfaceVulkan::Initialize_SyncPrimitives(void) noexcept
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

void ShellRenderInterfaceVulkan::Initialize_Resources(void) noexcept
{
	this->m_command_list.Initialize(this->m_p_device, this->m_queue_index_graphics, kSwapchainBackBufferCount, 2);

	MemoryPoolCreateInfo info = {};
	info.m_number_of_back_buffers = kSwapchainBackBufferCount;
	info.m_gpu_data_count = 100;
	info.m_gpu_data_size = sizeof(shader_vertex_user_data_t);
	info.m_memory_total_size = ShellRenderInterfaceVulkan::ConvertMegabytesToBytes(kVideoMemoryForAllocation);

	this->m_memory_pool.Initialize(&this->m_current_physical_device_properties, this->m_p_allocator, this->m_p_device, info);

	this->m_upload_manager.Initialize(this->m_p_device, this->m_p_queue_graphics, this->m_queue_index_graphics);
	this->m_manager_descriptors.Initialize(this->m_p_device, 100, 100, 10, 10);

	this->m_textures.reserve(kTexturesForReserve);

	// fix for having valid pointers until we not out of bound
	this->m_compiled_geometries.reserve(kGeometryForReserve);

	auto storage = this->LoadShaders();

	this->CreateShaders(storage);
	this->CreateDescriptorSetLayout(storage);
	this->CreatePipelineLayout();
	this->CreateSamplers();
	this->CreateDescriptorSets();
}

void ShellRenderInterfaceVulkan::Initialize_Allocator(void) noexcept
{
	VK_ASSERT(this->m_p_device, "you must have a valid VkDevice here");
	VK_ASSERT(this->m_p_physical_device_current, "you must have a valid VkPhysicalDevice here");
	VK_ASSERT(this->m_p_instance, "you must have a valid VkInstance here");

	VmaAllocatorCreateInfo info = {};

	info.vulkanApiVersion = VK_API_VERSION_1_0;
	info.device = this->m_p_device;
	info.instance = this->m_p_instance;
	info.physicalDevice = this->m_p_physical_device_current;

	auto status = vmaCreateAllocator(&info, &this->m_p_allocator);

	VK_ASSERT(status == VkResult::VK_SUCCESS, "failed to vmaCreateAllocator");
}

void ShellRenderInterfaceVulkan::Destroy_Instance(void) noexcept
{
	vkDestroyInstance(this->m_p_instance, nullptr);
}

void ShellRenderInterfaceVulkan::Destroy_Device() noexcept
{
	vkDestroyDevice(this->m_p_device, nullptr);
}

void ShellRenderInterfaceVulkan::Destroy_Swapchain(void) noexcept
{
	VK_ASSERT(this->m_p_device, "you must initialize device");

	vkDestroySwapchainKHR(this->m_p_device, this->m_p_swapchain, nullptr);
}

void ShellRenderInterfaceVulkan::Destroy_Surface(void) noexcept
{
	vkDestroySurfaceKHR(this->m_p_instance, this->m_p_surface, nullptr);
}

void ShellRenderInterfaceVulkan::Destroy_SyncPrimitives(void) noexcept
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

void ShellRenderInterfaceVulkan::Destroy_Resources(void) noexcept
{
	this->m_memory_pool.Shutdown();
	this->m_command_list.Shutdown();
	this->m_upload_manager.Shutdown();

	// TODO: put this into DestroyDescriptorSets()
	for (auto& pair_descriptor_id_descriptor_wrapper : this->m_descriptor_sets)
	{
		auto p_set = pair_descriptor_id_descriptor_wrapper.second.Get_DescriptorSet();
		this->m_manager_descriptors.Free_Descriptors(this->m_p_device, &p_set);
	}
	this->m_descriptor_sets.clear();

	if (this->m_p_descriptor_set)
	{
		this->m_manager_descriptors.Free_Descriptors(this->m_p_device, &this->m_p_descriptor_set);
	}

	this->m_manager_descriptors.Shutdown(this->m_p_device);

	vkDestroyDescriptorSetLayout(this->m_p_device, this->m_p_descriptor_set_layout, nullptr);
	vkDestroyPipelineLayout(this->m_p_device, this->m_p_pipeline_layout, nullptr);

	for (const auto& p_module : this->m_shaders)
	{
		vkDestroyShaderModule(this->m_p_device, p_module, nullptr);
	}

	this->DestroySamplers();
	this->Destroy_Textures();
}

void ShellRenderInterfaceVulkan::Destroy_Allocator(void) noexcept
{
	VK_ASSERT(this->m_p_allocator, "you must have an initialized allocator for deleting");

	vmaDestroyAllocator(this->m_p_allocator);

	this->m_p_allocator = nullptr;
}

void ShellRenderInterfaceVulkan::QueryInstanceLayers(void) noexcept
{
	uint32_t instance_layer_properties_count = 0;

	VkResult status = vkEnumerateInstanceLayerProperties(&instance_layer_properties_count, nullptr);

	VK_ASSERT(status == VK_SUCCESS, "failed to vkEnumerateInstanceLayerProperties (getting count)");

	if (instance_layer_properties_count)
	{
		this->m_instance_layer_properties.resize(instance_layer_properties_count);

		status = vkEnumerateInstanceLayerProperties(&instance_layer_properties_count, this->m_instance_layer_properties.data());

		VK_ASSERT(status == VK_SUCCESS, "failed to vkEnumerateInstanceLayerProperties (filling vector of VkLayerProperties)");
	}
}

void ShellRenderInterfaceVulkan::QueryInstanceExtensions(void) noexcept
{
	uint32_t instance_extension_property_count = 0;

	VkResult status = vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_property_count, nullptr);

	VK_ASSERT(status == VK_SUCCESS, "failed to vkEnumerateInstanceExtensionProperties (getting count)");

	if (instance_extension_property_count)
	{
		this->m_instance_extension_properties.resize(instance_extension_property_count);
		status = vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_property_count, this->m_instance_extension_properties.data());

		VK_ASSERT(status == VK_SUCCESS, "failed to vkEnumerateInstanceExtensionProperties (filling vector of VkExtensionProperties)");
	}

	uint32_t count = 0;

	// without first argument in vkEnumerateInstanceExtensionProperties
	// it doesn't collect information well so we need brute-force
	// and pass through everything what use has
	for (const auto& layer_property : this->m_instance_layer_properties)
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
					Shell::Log("[Vulkan] obtained extensions for layer: %s, count: %d", layer_property.layerName, props.size());
#endif

					for (const auto& extension : props)
					{
						if (this->IsExtensionPresent(this->m_instance_extension_properties, extension.extensionName) == false)
						{
#ifdef RMLUI_DEBUG
							Shell::Log("[Vulkan] new extension is added: %s", extension.extensionName);
#endif

							this->m_instance_extension_properties.push_back(extension);
						}
					}
				}
			}
		}
	}
}

bool ShellRenderInterfaceVulkan::AddLayerToInstance(const char* p_instance_layer_name) noexcept
{
	if (p_instance_layer_name == nullptr)
	{
		VK_ASSERT(false, "you have an invalid layer");
		return false;
	}

	if (this->IsLayerPresent(this->m_instance_layer_properties, p_instance_layer_name))
	{
		this->m_instance_layer_names.push_back(p_instance_layer_name);
		return true;
	}

	Shell::Log("[Vulkan] can't add layer %s", p_instance_layer_name);

	return false;
}

bool ShellRenderInterfaceVulkan::AddExtensionToInstance(const char* p_instance_extension_name) noexcept
{
	if (p_instance_extension_name == nullptr)
	{
		VK_ASSERT(false, "you have an invalid extension");
		return false;
	}

	if (this->IsExtensionPresent(this->m_instance_extension_properties, p_instance_extension_name))
	{
		this->m_instance_extension_names.push_back(p_instance_extension_name);
		return true;
	}

	Shell::Log("[Vulkan] can't add extension %s", p_instance_extension_name);

	return false;
}

void ShellRenderInterfaceVulkan::CreatePropertiesFor_Instance(void) noexcept
{
	this->QueryInstanceLayers();
	this->QueryInstanceExtensions();

	this->AddLayerToInstance("VK_LAYER_LUNARG_monitor");
	this->AddExtensionToInstance("VK_EXT_debug_utils");

#if defined(RMLUI_PLATFORM_UNIX)
	this->AddExtensionToInstance(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#elif defined(RMLUI_PLATFORM_WIN32)
	this->AddExtensionToInstance(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif

	this->AddExtensionToInstance(VK_KHR_SURFACE_EXTENSION_NAME);

#ifdef RMLUI_DEBUG
	bool is_cpu_validation =
		this->AddLayerToInstance("VK_LAYER_KHRONOS_validation") && this->AddExtensionToInstance(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

	if (is_cpu_validation)
	{
		Shell::Log("[Vulkan] CPU validation is enabled");

		Rml::Vector<const char*> requested_extensions_for_gpu = {VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME};

		for (const auto& extension_name : requested_extensions_for_gpu)
		{
			this->AddExtensionToInstance(extension_name);
		}

		debug_validation_features_ext.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
		debug_validation_features_ext.pNext = nullptr;
		debug_validation_features_ext.enabledValidationFeatureCount =
			sizeof(debug_validation_features_ext_requested) / sizeof(debug_validation_features_ext_requested[0]);
		debug_validation_features_ext.pEnabledValidationFeatures = debug_validation_features_ext_requested;
	}
#endif
}

bool ShellRenderInterfaceVulkan::IsLayerPresent(const Rml::Vector<VkLayerProperties>& properties, const char* p_layer_name) noexcept
{
	if (properties.empty())
		return false;

	if (p_layer_name == nullptr)
		return false;

	return std::find_if(properties.cbegin(), properties.cend(),
			   [p_layer_name](const VkLayerProperties& prop) -> bool { return strcmp(prop.layerName, p_layer_name) == 0; }) != properties.cend();
}

bool ShellRenderInterfaceVulkan::IsExtensionPresent(const Rml::Vector<VkExtensionProperties>& properties, const char* p_extension_name) noexcept
{
	if (properties.empty())
		return false;

	if (p_extension_name == nullptr)
		return false;

	return std::find_if(properties.cbegin(), properties.cend(), [p_extension_name](const VkExtensionProperties& prop) -> bool {
		return strcmp(prop.extensionName, p_extension_name) == 0;
	}) != properties.cend();
}

bool ShellRenderInterfaceVulkan::AddExtensionToDevice(const char* p_device_extension_name) noexcept
{
	if (this->IsExtensionPresent(this->m_device_extension_properties, p_device_extension_name))
	{
		this->m_device_extension_names.push_back(p_device_extension_name);
		return true;
	}

	return false;
}

void ShellRenderInterfaceVulkan::CreatePropertiesFor_Device(void) noexcept
{
	VK_ASSERT(this->m_p_physical_device_current, "you must initialize your physical device. Call InitializePhysicalDevice first");

	uint32_t extension_count = 0;

	VkResult status = vkEnumerateDeviceExtensionProperties(this->m_p_physical_device_current, nullptr, &extension_count, nullptr);

	VK_ASSERT(status == VK_SUCCESS, "failed to vkEnumerateDeviceExtensionProperties (getting count)");

	this->m_device_extension_properties.resize(extension_count);

	status = vkEnumerateDeviceExtensionProperties(
		this->m_p_physical_device_current, nullptr, &extension_count, this->m_device_extension_properties.data());

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
					if (this->IsExtensionPresent(this->m_device_extension_properties, extension.extensionName) == false)
					{
						Shell::Log("[Vulkan] obtained new device extension from layer[%s]: %s", layer.layerName, extension.extensionName);

						this->m_device_extension_properties.push_back(extension);
					}
				}
			}
		}
	}
}

void ShellRenderInterfaceVulkan::CreateReportDebugCallback(void) noexcept
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

void ShellRenderInterfaceVulkan::Destroy_ReportDebugCallback(void) noexcept
{
	PFN_vkDestroyDebugReportCallbackEXT p_destroy_callback =
		reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(vkGetInstanceProcAddr(this->m_p_instance, "vkDestroyDebugReportCallbackEXT"));

	if (p_destroy_callback)
	{
		p_destroy_callback(this->m_p_instance, this->m_debug_report_callback_instance, nullptr);
	}
}

uint32_t ShellRenderInterfaceVulkan::GetUserAPIVersion(void) const noexcept
{
	uint32_t result = 0;

	VkResult status = vkEnumerateInstanceVersion(&result);

	VK_ASSERT(status == VK_SUCCESS, "failed to vkEnumerateInstanceVersion, See Status");

	return result;
}

uint32_t ShellRenderInterfaceVulkan::GetRequiredVersionAndValidateMachine(void) noexcept
{
	constexpr uint32_t kRequiredVersion = VK_API_VERSION_1_0;
	const uint32_t user_version = this->GetUserAPIVersion();

	VK_ASSERT(kRequiredVersion <= user_version, "Your machine doesn't support Vulkan");

	return kRequiredVersion;
}

void ShellRenderInterfaceVulkan::CollectPhysicalDevices(void) noexcept
{
	uint32_t gpu_count = 1;
	const uint32_t required_count = gpu_count;

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

bool ShellRenderInterfaceVulkan::ChoosePhysicalDevice(VkPhysicalDeviceType device_type) noexcept
{
	VK_ASSERT(this->m_physical_devices.empty() == false,
		"you must have one videocard at least or early calling of this method, try call this after CollectPhysicalDevices");

	VkPhysicalDeviceProperties props = {};

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

void ShellRenderInterfaceVulkan::PrintInformationAboutPickedPhysicalDevice(VkPhysicalDevice p_physical_device) noexcept
{
	if (p_physical_device == nullptr)
		return;

	for (const auto& device : this->m_physical_devices)
	{
		if (p_physical_device == device.GetHandle())
		{
			const auto& properties = device.GetProperties();
			Shell::Log("Picked physical device: %s", properties.deviceName);

			return;
		}
	}
}

VkSurfaceFormatKHR ShellRenderInterfaceVulkan::ChooseSwapchainFormat(void) noexcept
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

VkExtent2D ShellRenderInterfaceVulkan::CreateValidSwapchainExtent(void) noexcept
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
		return caps.currentExtent;
	}
}

VkSurfaceTransformFlagBitsKHR ShellRenderInterfaceVulkan::CreatePretransformSwapchain(void) noexcept
{
	auto caps = this->GetSurfaceCapabilities();

	VkSurfaceTransformFlagBitsKHR result =
		(caps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) ? VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR : caps.currentTransform;

	return result;
}

VkCompositeAlphaFlagBitsKHR ShellRenderInterfaceVulkan::ChooseSwapchainCompositeAlpha(void) noexcept
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

// https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkPresentModeKHR.html
// VK_PRESENT_MODE_FIFO_KHR system must support this mode at least so by default we want to use it otherwise user can specify his mode
VkPresentModeKHR ShellRenderInterfaceVulkan::GetPresentMode(VkPresentModeKHR required) noexcept
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

	Shell::Log("[Vulkan] WARNING system can't detect your type of present mode so we choose the first from vector front");

	return present_modes.front();
}

VkSurfaceCapabilitiesKHR ShellRenderInterfaceVulkan::GetSurfaceCapabilities(void) noexcept
{
	VK_ASSERT(this->m_p_device, "[Vulkan] you must initialize your device, before calling this method");
	VK_ASSERT(this->m_p_physical_device_current, "[Vulkan] you must initialize your physical device, before calling this method");
	VK_ASSERT(this->m_p_surface, "[Vulkan] you must initialize your surface, before calling this method");

	VkSurfaceCapabilitiesKHR result;

	VkResult status = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(this->m_p_physical_device_current, this->m_p_surface, &result);

	VK_ASSERT(status == VK_SUCCESS, "[Vulkan] failed to vkGetPhysicalDeviceSurfaceCapabilitiesKHR");

	return result;
}

Rml::Vector<ShellRenderInterfaceVulkan::shader_data_t> ShellRenderInterfaceVulkan::LoadShaders(void) noexcept
{
	auto vertex = this->LoadShader("assets/shader_vertex.spv");
	auto frag_color = this->LoadShader("assets/shader_pixel_without_textures.spv");
	auto frag_texture = this->LoadShader("assets/shader_pixel_with_textures.spv");

	Rml::Vector<shader_data_t> result;

	result.push_back(vertex);
	result.push_back(frag_color);
	result.push_back(frag_texture);

	return result;
}

ShellRenderInterfaceVulkan::shader_data_t ShellRenderInterfaceVulkan::LoadShader(
	const Rml::String& relative_path_from_samples_folder_with_file_and_fileformat) noexcept
{
	VK_ASSERT(Rml::GetFileInterface(), "[Vulkan] you must initialize FileInterface before calling this method");
	shader_data_t result;

	if (relative_path_from_samples_folder_with_file_and_fileformat.empty())
	{
		VK_ASSERT(false, "[Vulkan] you can't pass an empty string for loading shader");
		return result;
	}

	auto* p_file_interface = Rml::GetFileInterface();

	auto p_file = Rml::GetFileInterface()->Open(relative_path_from_samples_folder_with_file_and_fileformat);

	VK_ASSERT(p_file, "[Vulkan] Rml::FileHandle is invalid! %s", relative_path_from_samples_folder_with_file_and_fileformat.c_str());

	auto file_size = p_file_interface->Length(p_file);

	VK_ASSERT(file_size != -1L, "[Vulkan] can't get length of file: %s", relative_path_from_samples_folder_with_file_and_fileformat.c_str());

	Rml::Vector<uint32_t> buffer(file_size);

	p_file_interface->Read(buffer.data(), buffer.size(), p_file);

	p_file_interface->Close(p_file);

	result.Set_Data(buffer);

	return result;
}

void ShellRenderInterfaceVulkan::CreateShaders(const Rml::Vector<shader_data_t>& storage) noexcept
{
	VK_ASSERT(storage.empty() == false, "[Vulkan] you must load shaders before creating resources");
	VK_ASSERT(this->m_p_device, "[Vulkan] you must initialize VkDevice before calling this method");

	VkShaderModuleCreateInfo info = {};

	for (const auto& shader_data : storage)
	{
		VkShaderModule p_module = nullptr;

		info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		info.pCode = shader_data.Get_Data().data();
		info.codeSize = shader_data.Get_Data().size();

		VkResult status = vkCreateShaderModule(this->m_p_device, &info, nullptr, &p_module);

		VK_ASSERT(status == VK_SUCCESS, "[Vulkan] failed to vkCreateShaderModule");

		this->m_shaders.push_back(p_module);
	}
}

void ShellRenderInterfaceVulkan::CreateDescriptorSetLayout(Rml::Vector<shader_data_t>& storage) noexcept
{
	VK_ASSERT(storage.empty() == false, "[Vulkan] you must load shaders before creating resources");
	VK_ASSERT(this->m_p_device, "[Vulkan] you must initialize VkDevice before calling this method");

	Rml::Vector<VkDescriptorSetLayoutBinding> all_bindings;

	for (auto& shader_data : storage)
	{
		const auto& current_bindings = this->CreateDescriptorSetLayoutBindings(shader_data);

		all_bindings.insert(all_bindings.end(), current_bindings.begin(), current_bindings.end());
	}

	VkDescriptorSetLayoutCreateInfo info = {};

	info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	info.pBindings = all_bindings.data();
	info.bindingCount = all_bindings.size();

	VkDescriptorSetLayout p_layout = nullptr;

	VkResult status = vkCreateDescriptorSetLayout(this->m_p_device, &info, nullptr, &p_layout);

	VK_ASSERT(status == VK_SUCCESS, "[Vulkan] failed to vkCreateDescriptorSetLayout");

	this->m_p_descriptor_set_layout = p_layout;
}

Rml::Vector<VkDescriptorSetLayoutBinding> ShellRenderInterfaceVulkan::CreateDescriptorSetLayoutBindings(shader_data_t& data) noexcept
{
	Rml::Vector<VkDescriptorSetLayoutBinding> result;

	VK_ASSERT(data.Get_Data().empty() == false, "[Vulkan] can't be empty data of shader");

	SpvReflectShaderModule spv_module = {};

	SpvReflectResult status = spvReflectCreateShaderModule(data.Get_Data().size() * sizeof(char), data.Get_Data().data(), &spv_module);

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

void ShellRenderInterfaceVulkan::CreatePipelineLayout(void) noexcept
{
	VK_ASSERT(this->m_p_descriptor_set_layout, "[Vulkan] You must initialize VkDescriptorSetLayout before calling this method");
	VK_ASSERT(this->m_p_device, "[Vulkan] you must initialize VkDevice before calling this method");

	VkPipelineLayoutCreateInfo info = {};

	info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	info.pNext = nullptr;
	info.pSetLayouts = &this->m_p_descriptor_set_layout;
	info.setLayoutCount = 1;

	auto status = vkCreatePipelineLayout(this->m_p_device, &info, nullptr, &this->m_p_pipeline_layout);

	VK_ASSERT(status == VK_SUCCESS, "[Vulkan] failed to vkCreatePipelineLayout");
}

void ShellRenderInterfaceVulkan::CreateDescriptorSets(void) noexcept
{
	// TODO: TO ALL, but it is better to use one descriptor set, but if we accumulate drawings and compile every time our geometry to it is better to
	// think about how to reduce descriptor sets or cache g_current_compiled_geometry for every compiled geometry and just pass them to
	// RenderCompiledGeometry
	// <= it is needed to be placed some where because before this calling you accumulate ALL geometry what you need to compile and after filled
	// Rml::Vector<CompiledGeometryHandle> then you call once RenderCompiledGeometry, because you cached all stuff

	/*
	Rml::Vector<VkDescriptorSetLayout> layouts(kDescriptorSetsCount, this->m_p_descriptor_set_layout);
	Rml::Vector<VkDescriptorSet> sets(kDescriptorSetsCount, nullptr);

	this->m_descriptor_sets.reserve(kDescriptorSetsCount);

	this->m_manager_descriptors.Alloc_Descriptor(this->m_p_device, layouts.data(), sets.data(), kDescriptorSetsCount);

	for (int i = 0; i < kDescriptorSetsCount; ++i)
	{
	    this->m_descriptor_sets[i].Set_DescriptorSet(sets.at(i));
	}
	*/

	VK_ASSERT(this->m_p_device, "[Vulkan] you have to initialize your VkDevice before calling this method");
	VK_ASSERT(this->m_p_descriptor_set_layout, "[Vulkan] you have to initialize your VkDescriptorSetLayout before calling this method");

	this->m_manager_descriptors.Alloc_Descriptor(this->m_p_device, &this->m_p_descriptor_set_layout, &this->m_p_descriptor_set);
}

void ShellRenderInterfaceVulkan::CreateSamplers(void) noexcept
{
	VkSamplerCreateInfo info = {};

	info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	info.pNext = nullptr;
	info.magFilter = VK_FILTER_NEAREST;
	info.minFilter = VK_FILTER_NEAREST;
	info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	vkCreateSampler(this->m_p_device, &info, nullptr, &this->m_p_sampler_nearest);
}

void ShellRenderInterfaceVulkan::Create_Pipelines(void) noexcept
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
	info_depth.depthTestEnable = VK_TRUE;
	info_depth.depthWriteEnable = VK_TRUE;
	info_depth.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	info_depth.back.compareOp = VK_COMPARE_OP_ALWAYS;

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

	Rml::Vector<VkDynamicState> dynamicStateEnables = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
		VK_DYNAMIC_STATE_LINE_WIDTH,
	};

	VkPipelineDynamicStateCreateInfo info_dynamic_state = {};

	info_dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	info_dynamic_state.pNext = nullptr;
	info_dynamic_state.pDynamicStates = dynamicStateEnables.data();
	info_dynamic_state.dynamicStateCount = dynamicStateEnables.size();
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
	info_vertex.vertexAttributeDescriptionCount = info_shader_vertex_attributes.size();
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
	info.stageCount = shaders_that_will_be_used_in_pipeline.size();
	info.pStages = shaders_that_will_be_used_in_pipeline.data();
	info.pVertexInputState = &info_vertex;
	info.layout = this->m_p_pipeline_layout;
	info.renderPass = this->m_p_render_pass;

	auto status = vkCreateGraphicsPipelines(this->m_p_device, nullptr, 1, &info, nullptr, &this->m_p_pipeline_with_textures);

	VK_ASSERT(status == VkResult::VK_SUCCESS, "failed to vkCreateGraphicsPipelines");

	info_shader.module = this->m_shaders[static_cast<int>(shader_id_t::kShaderID_Pixel_WithoutTextures)];
	shaders_that_will_be_used_in_pipeline[1] = info_shader;

	status = vkCreateGraphicsPipelines(this->m_p_device, nullptr, 1, &info, nullptr, &this->m_p_pipeline_without_textures);

	VK_ASSERT(status == VkResult::VK_SUCCESS, "failed to vkCreateGraphicsPipelines");
}

void ShellRenderInterfaceVulkan::CreateSwapchainFrameBuffers(void) noexcept
{
	VK_ASSERT(this->m_p_render_pass, "you must create a VkRenderPass before calling this method");
	VK_ASSERT(this->m_p_device, "you must have a valid VkDevice here");

	this->CreateSwapchainImageViews();

	this->m_swapchain_frame_buffers.resize(this->m_swapchain_image_views.size());

	VkFramebufferCreateInfo info = {};
	int index = 0;
	VkResult status = VkResult::VK_SUCCESS;

	for (auto p_view : this->m_swapchain_image_views)
	{
		VkImageView p_attachment_image_views[] = {p_view};
		info.sType = VkStructureType::VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		info.pNext = nullptr;
		info.renderPass = this->m_p_render_pass;
		info.attachmentCount = 1;
		info.pAttachments = p_attachment_image_views;
		info.width = this->m_width;
		info.height = this->m_height;
		info.layers = 1;

		status = vkCreateFramebuffer(this->m_p_device, &info, nullptr, &this->m_swapchain_frame_buffers[index]);

		++index;
	}
}

void ShellRenderInterfaceVulkan::CreateSwapchainImages(void) noexcept
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

void ShellRenderInterfaceVulkan::CreateSwapchainImageViews(void) noexcept
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

void ShellRenderInterfaceVulkan::CreateResourcesDependentOnSize(void) noexcept
{
	this->m_viewport.height = -this->m_height;
	this->m_viewport.width = this->m_width;
	this->m_viewport.minDepth = 0.0f;
	this->m_viewport.maxDepth = 1.0f;
	this->m_viewport.x = 0.0f;
	this->m_viewport.y = this->m_height;

	this->m_scissor.extent.width = this->m_width;
	this->m_scissor.extent.height = this->m_height;
	this->m_scissor.offset.x = 0;
	this->m_scissor.offset.y = 0;

	this->m_projection =
		Rml::Matrix4f::ProjectOrtho(0.0f, static_cast<float>(this->m_width), static_cast<float>(this->m_height), 0.0f, -20.0f, 20.0f);

	this->SetTransform(nullptr);

	this->CreateRenderPass();
	this->CreateSwapchainFrameBuffers();
	this->Create_Pipelines();
}

ShellRenderInterfaceVulkan::buffer_data_t ShellRenderInterfaceVulkan::CreateResource_StagingBuffer(
	VkDeviceSize size, VkBufferUsageFlags flags) noexcept
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

	Shell::Log("allocated buffer with size %d in bytes", info_stats.size);

	result.Set_VkBuffer(p_buffer);
	result.Set_VmaAllocation(p_allocation);

	return result;
}

void ShellRenderInterfaceVulkan::DestroyResource_StagingBuffer(const buffer_data_t& data) noexcept
{
	if (this->m_p_allocator)
	{
		if (data.Get_VkBuffer() && data.Get_VmaAllocation())
		{
			vmaDestroyBuffer(this->m_p_allocator, data.Get_VkBuffer(), data.Get_VmaAllocation());

			Shell::Log("destroy buffer");
		}
	}
}

void ShellRenderInterfaceVulkan::Destroy_Textures(void) noexcept
{
	VK_ASSERT(this->m_p_device, "must exist");
	VK_ASSERT(this->m_p_allocator, "must exist");

	for (const auto& texture : this->m_textures)
	{
		if (texture.Get_VmaAllocation())
		{
			vmaDestroyImage(this->m_p_allocator, texture.Get_VkImage(), texture.Get_VmaAllocation());
			vkDestroyImageView(this->m_p_device, texture.Get_VkImageView(), nullptr);
		}
	}
}

void ShellRenderInterfaceVulkan::DestroyResourcesDependentOnSize(void) noexcept
{
	this->Destroy_Pipelines();
	this->DestroySwapchainFrameBuffers();
	this->DestroyRenderPass();
}

void ShellRenderInterfaceVulkan::DestroySwapchainImageViews(void) noexcept
{
	VK_ASSERT(this->m_p_device, "[Vulkan] you must initialize VkDevice before calling this method");

	this->m_swapchain_images.clear();

	for (auto p_view : this->m_swapchain_image_views)
	{
		vkDestroyImageView(this->m_p_device, p_view, nullptr);
	}

	this->m_swapchain_image_views.clear();
}

void ShellRenderInterfaceVulkan::DestroySwapchainFrameBuffers(void) noexcept
{
	this->DestroySwapchainImageViews();

	for (auto p_frame_buffer : this->m_swapchain_frame_buffers)
	{
		vkDestroyFramebuffer(this->m_p_device, p_frame_buffer, nullptr);
	}

	this->m_swapchain_frame_buffers.clear();
}

void ShellRenderInterfaceVulkan::DestroyRenderPass(void) noexcept
{
	VK_ASSERT(this->m_p_device, "you must have a valid VkDevice here");

	if (this->m_p_render_pass)
	{
		vkDestroyRenderPass(this->m_p_device, this->m_p_render_pass, nullptr);
		this->m_p_render_pass = nullptr;
	}
}

void ShellRenderInterfaceVulkan::Destroy_Pipelines(void) noexcept
{
	vkDestroyPipeline(this->m_p_device, this->m_p_pipeline_with_textures, nullptr);
	vkDestroyPipeline(this->m_p_device, this->m_p_pipeline_without_textures, nullptr);
}

void ShellRenderInterfaceVulkan::DestroyDescriptorSets(void) noexcept {}

void ShellRenderInterfaceVulkan::DestroyPipelineLayout(void) noexcept {}

void ShellRenderInterfaceVulkan::DestroySamplers(void) noexcept
{
	VK_ASSERT(this->m_p_device, "must exist here");
	vkDestroySampler(this->m_p_device, this->m_p_sampler_nearest, nullptr);
}

void ShellRenderInterfaceVulkan::CreateRenderPass(void) noexcept
{
	VK_ASSERT(this->m_p_device, "you must have a valid VkDevice here");

	VkAttachmentDescription attachments[1];

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
	attachments[0].flags = 0;

	VkAttachmentReference swapchain_color_reference;

	swapchain_color_reference.attachment = 0;
	swapchain_color_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};

	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.flags = 0;
	subpass.inputAttachmentCount = 0;
	subpass.pInputAttachments = nullptr;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &swapchain_color_reference;
	subpass.pResolveAttachments = nullptr;
	subpass.pDepthStencilAttachment = nullptr;
	subpass.preserveAttachmentCount = 0;
	subpass.pPreserveAttachments = nullptr;

	VkSubpassDependency dep = {};

	dep.dependencyFlags = 0;
	dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dep.srcAccessMask = 0;
	dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dep.dstSubpass = 0;
	dep.srcSubpass = VK_SUBPASS_EXTERNAL;

	VkRenderPassCreateInfo info = {};

	info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	info.pNext = nullptr;
	info.attachmentCount = 1;
	info.pAttachments = attachments;
	info.subpassCount = 1;
	info.pSubpasses = &subpass;
	info.dependencyCount = 1;
	info.pDependencies = &dep;

	VkResult status = vkCreateRenderPass(this->m_p_device, &info, nullptr, &this->m_p_render_pass);

	VK_ASSERT(status == VK_SUCCESS, "failed to vkCreateRenderPass");
}

void ShellRenderInterfaceVulkan::Wait(void) noexcept
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

void ShellRenderInterfaceVulkan::Submit(void) noexcept
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

void ShellRenderInterfaceVulkan::Present(void) noexcept
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

ShellRenderInterfaceVulkan::PhysicalDeviceWrapper::PhysicalDeviceWrapper(VkPhysicalDevice p_physical_device) : m_p_physical_device(p_physical_device)
{
	VK_ASSERT(this->m_p_physical_device, "you passed an invalid pointer of VkPhysicalDevice");

	vkGetPhysicalDeviceProperties(this->m_p_physical_device, &this->m_physical_device_properties);

	this->m_physical_device_limits = this->m_physical_device_properties.limits;
}

ShellRenderInterfaceVulkan::PhysicalDeviceWrapper::PhysicalDeviceWrapper(void) {}

ShellRenderInterfaceVulkan::PhysicalDeviceWrapper::~PhysicalDeviceWrapper(void) {}

VkPhysicalDevice ShellRenderInterfaceVulkan::PhysicalDeviceWrapper::GetHandle(void) const noexcept
{
	return this->m_p_physical_device;
}

const VkPhysicalDeviceProperties& ShellRenderInterfaceVulkan::PhysicalDeviceWrapper::GetProperties(void) const noexcept
{
	return this->m_physical_device_properties;
}

const VkPhysicalDeviceLimits& ShellRenderInterfaceVulkan::PhysicalDeviceWrapper::GetLimits(void) const noexcept
{
	return this->m_physical_device_limits;
}

ShellRenderInterfaceVulkan::CommandListRing::CommandListRing(void) :
	m_p_current_frame{}, m_frame_index{}, m_number_of_frames{}, m_command_lists_per_frame{}
{}

ShellRenderInterfaceVulkan::CommandListRing::~CommandListRing(void) {}

void ShellRenderInterfaceVulkan::CommandListRing::Initialize(
	VkDevice p_device, uint32_t queue_index_graphics, uint32_t number_of_back_buffers, uint32_t command_list_per_frame) noexcept
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

void ShellRenderInterfaceVulkan::CommandListRing::Shutdown(void)
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

void ShellRenderInterfaceVulkan::CommandListRing::OnBeginFrame(void)
{
	this->m_p_current_frame = &this->m_frames.at(this->m_frame_index % this->m_number_of_frames);

	this->m_p_current_frame->SetUsedCalls(0);

	++this->m_frame_index;
}

VkCommandBuffer ShellRenderInterfaceVulkan::CommandListRing::GetNewCommandList(void)
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

const Rml::Vector<VkCommandBuffer>& ShellRenderInterfaceVulkan::CommandListRing::GetAllCommandBuffersForCurrentFrame(void) const noexcept
{
	VK_ASSERT(this->m_p_current_frame, "you must have a valid pointer here");

	return this->m_p_current_frame->GetCommandBuffers();
}

uint32_t ShellRenderInterfaceVulkan::CommandListRing::GetCountOfCommandBuffersPerFrame(void) const noexcept
{
	return this->m_command_lists_per_frame;
}

ShellRenderInterfaceVulkan::CommandListRing::CommandBufferPerFrame::CommandBufferPerFrame(void) : m_used_calls{}, m_number_per_frame_command_lists{}
{}

ShellRenderInterfaceVulkan::CommandListRing::CommandBufferPerFrame::~CommandBufferPerFrame(void) {}

uint32_t ShellRenderInterfaceVulkan::CommandListRing::CommandBufferPerFrame::GetUsedCalls(void) const noexcept
{
	return this->m_used_calls;
}

const Rml::Vector<VkCommandPool>& ShellRenderInterfaceVulkan::CommandListRing::CommandBufferPerFrame::GetCommandPools(void) const noexcept
{
	return this->m_command_pools;
}

const Rml::Vector<VkCommandBuffer>& ShellRenderInterfaceVulkan::CommandListRing::CommandBufferPerFrame::GetCommandBuffers(void) const noexcept
{
	return this->m_command_buffers;
}

void ShellRenderInterfaceVulkan::CommandListRing::CommandBufferPerFrame::AddCommandPools(VkCommandPool p_command_pool) noexcept
{
	VK_ASSERT(p_command_pool, "you must pass a valid pointer of VkCommandPool");

	this->m_command_pools.push_back(p_command_pool);
}

void ShellRenderInterfaceVulkan::CommandListRing::CommandBufferPerFrame::AddCommandBuffers(VkCommandBuffer p_buffer) noexcept
{
	VK_ASSERT(p_buffer, "you must pass a valid pointer of VkCommandBuffer");

	this->m_command_buffers.push_back(p_buffer);
}

void ShellRenderInterfaceVulkan::CommandListRing::CommandBufferPerFrame::SetUsedCalls(uint32_t number) noexcept
{
	this->m_used_calls = number;
}

void ShellRenderInterfaceVulkan::CommandListRing::CommandBufferPerFrame::SetCountCommandListsAndPools(uint32_t number) noexcept
{
	this->m_number_per_frame_command_lists = number;
}

ShellRenderInterfaceVulkan::MemoryRingAllocator::MemoryRingAllocator(void) : m_head{}, m_total_size{}, m_allocated_size{} {}

ShellRenderInterfaceVulkan::MemoryRingAllocator::~MemoryRingAllocator(void) {}

void ShellRenderInterfaceVulkan::MemoryRingAllocator::Initialize(uint32_t total_size) noexcept
{
	this->m_total_size = total_size;
}

uint32_t ShellRenderInterfaceVulkan::MemoryRingAllocator::MakePaddingToAvoidCrossover(uint32_t size) const noexcept
{
	uint32_t tail = this->GetTail();

	if ((tail + size) > this->m_total_size)
	{
		return (this->m_total_size - tail);
	}
	else
	{
		return 0;
	}
}

uint32_t ShellRenderInterfaceVulkan::MemoryRingAllocator::GetSize(void) const noexcept
{
	return this->m_allocated_size;
}

uint32_t ShellRenderInterfaceVulkan::MemoryRingAllocator::GetHead(void) const noexcept
{
	return this->m_head;
}

uint32_t ShellRenderInterfaceVulkan::MemoryRingAllocator::GetTail(void) const noexcept
{
	return (this->m_head + this->m_allocated_size) % this->m_total_size;
}

bool ShellRenderInterfaceVulkan::MemoryRingAllocator::Alloc(uint32_t size, uint32_t* p_out) noexcept
{
	if (this->m_allocated_size + size <= this->m_total_size)
	{
		if (p_out)
		{
			*p_out = this->GetTail();
		}

		this->m_allocated_size += size;

		return true;
	}

	VK_ASSERT(false, "overflow, rebuild your allocator with pool");

	return false;
}

bool ShellRenderInterfaceVulkan::MemoryRingAllocator::Free(uint32_t size)
{
	if (this->m_allocated_size >= size)
	{
		this->m_head = (this->m_head + size) % this->m_total_size;
		this->m_allocated_size -= size;

		return true;
	}

	return false;
}

ShellRenderInterfaceVulkan::MemoryRingAllocatorWithTabs::MemoryRingAllocatorWithTabs(void) :
	m_frame_buffer_index{}, m_frame_buffer_number{}, m_memory_allocated_in_frame{}, m_p_allocated_memory_per_back_buffer{}
{}

ShellRenderInterfaceVulkan::MemoryRingAllocatorWithTabs::~MemoryRingAllocatorWithTabs(void) {}

void ShellRenderInterfaceVulkan::MemoryRingAllocatorWithTabs::Initialize(uint32_t number_of_frames, uint32_t total_size_memory) noexcept
{
	this->m_frame_buffer_number = number_of_frames;
	this->m_allocator.Initialize(total_size_memory);
}

void ShellRenderInterfaceVulkan::MemoryRingAllocatorWithTabs::Shutdown(void) noexcept
{
	this->m_allocator.Free(this->m_allocator.GetSize());
}

bool ShellRenderInterfaceVulkan::MemoryRingAllocatorWithTabs::Alloc(uint32_t size, uint32_t* p_out) noexcept
{
	uint32_t padding = this->m_allocator.MakePaddingToAvoidCrossover(size);

	if (padding > 0)
	{
		this->m_memory_allocated_in_frame += padding;

		if (this->m_allocator.Alloc(padding, nullptr) == false)
		{
#ifdef RMLUI_DEBUG
			Shell::Log("[Vulkan][Debug] can't allocat, because padding equals 0");
#endif

			return false;
		}
	}

	if (this->m_allocator.Alloc(size, p_out) == true)
	{
		this->m_memory_allocated_in_frame += size;
		return true;
	}

	return false;
}

void ShellRenderInterfaceVulkan::MemoryRingAllocatorWithTabs::OnBeginFrame(void) noexcept
{
	this->m_p_allocated_memory_per_back_buffer[this->m_frame_buffer_index] = this->m_memory_allocated_in_frame;
	this->m_memory_allocated_in_frame = 0;

	this->m_frame_buffer_index = (this->m_frame_buffer_index + 1) % this->m_frame_buffer_number;

	uint32_t memory_to_free = this->m_p_allocated_memory_per_back_buffer[this->m_frame_buffer_index];

	this->m_allocator.Free(memory_to_free);
}

ShellRenderInterfaceVulkan::MemoryPool::MemoryPool(void) :
	m_min_alignment_for_uniform_buffer{}, m_memory_total_size{}, m_memory_gpu_data_one_object{}, m_p_data{}, m_p_physical_device_current_properties{},
	m_p_buffer{}, m_p_buffer_alloc{}, m_p_device{}, m_p_vk_allocator{}
{}

ShellRenderInterfaceVulkan::MemoryPool::~MemoryPool(void) {}

void ShellRenderInterfaceVulkan::MemoryPool::Initialize(
	VkPhysicalDeviceProperties* p_props, VmaAllocator p_allocator, VkDevice p_device, const MemoryPoolCreateInfo& info_creation) noexcept
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
	Shell::Log("[Vulkan][Debug] the alignment for uniform buffer is: %d", this->m_min_alignment_for_uniform_buffer);
#endif

	this->m_memory_total_size = ShellRenderInterfaceVulkan::AlignUp(info_creation.m_memory_total_size, this->m_min_alignment_for_uniform_buffer);
	this->m_memory_gpu_data_one_object = ShellRenderInterfaceVulkan::AlignUp(info_creation.m_gpu_data_size, this->m_min_alignment_for_uniform_buffer);
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
	Shell::Log("[Vulkan][Debug] allocated memory for pool: %d Mbs", ShellRenderInterfaceVulkan::TranslateBytesToMegaBytes(info_stats.size));
#endif

	status = vmaMapMemory(this->m_p_vk_allocator, this->m_p_buffer_alloc, (void**)&this->m_p_data);

	VK_ASSERT(status == VkResult::VK_SUCCESS, "failed to vmaMapMemory");
}

void ShellRenderInterfaceVulkan::MemoryPool::Shutdown(void) noexcept
{
	VK_ASSERT(this->m_p_vk_allocator, "you must have a valid VmaAllocator");
	VK_ASSERT(this->m_p_buffer, "you must allocate VkBuffer for deleting");
	VK_ASSERT(this->m_p_buffer_alloc, "you must allocate VmaAllocation for deleting");

#ifdef RMLUI_DEBUG
	Shell::Log(
		"[Vulkan][Debug] Destroyed buffer with memory [%d] Mbs", ShellRenderInterfaceVulkan::TranslateBytesToMegaBytes(this->m_memory_total_size));
#endif

	vmaUnmapMemory(this->m_p_vk_allocator, this->m_p_buffer_alloc);
	vmaDestroyVirtualBlock(this->m_p_block);
	vmaDestroyBuffer(this->m_p_vk_allocator, this->m_p_buffer, this->m_p_buffer_alloc);
}

bool ShellRenderInterfaceVulkan::MemoryPool::Alloc_GeneralBuffer(
	uint32_t size, void** p_data, VkDescriptorBufferInfo* p_out, VmaVirtualAllocation* p_alloc) noexcept
{
	VK_ASSERT(p_out, "you must pass a valid pointer");
	VK_ASSERT(this->m_p_buffer, "you must have a valid VkBuffer");

	VK_ASSERT(*p_alloc == nullptr, "you can't pass a VALID object, because it is for initialization. So it means you passed the already allocated "
								   "VmaVirtualAllocation and it means you did something wrong, like you wanted to allocate into the same object...");

	size = ShellRenderInterfaceVulkan::AlignUp(size, this->m_min_alignment_for_uniform_buffer);

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

bool ShellRenderInterfaceVulkan::MemoryPool::Alloc_VertexBuffer(
	uint32_t number_of_elements, uint32_t stride_in_bytes, void** p_data, VkDescriptorBufferInfo* p_out, VmaVirtualAllocation* p_alloc) noexcept
{
	return this->Alloc_GeneralBuffer(number_of_elements * stride_in_bytes, p_data, p_out, p_alloc);
}

bool ShellRenderInterfaceVulkan::MemoryPool::Alloc_IndexBuffer(
	uint32_t number_of_elements, uint32_t stride_in_bytes, void** p_data, VkDescriptorBufferInfo* p_out, VmaVirtualAllocation* p_alloc) noexcept
{
	return this->Alloc_GeneralBuffer(number_of_elements * stride_in_bytes, p_data, p_out, p_alloc);
}

// TODO: probably needs to remove because we don't use ring allocator
void ShellRenderInterfaceVulkan::MemoryPool::OnBeginFrame(void) noexcept {}

void ShellRenderInterfaceVulkan::MemoryPool::SetDescriptorSet(
	uint32_t binding_index, uint32_t size, VkDescriptorType descriptor_type, VkDescriptorSet p_set) noexcept
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

void ShellRenderInterfaceVulkan::MemoryPool::SetDescriptorSet(
	uint32_t binding_index, VkDescriptorBufferInfo* p_info, VkDescriptorType descriptor_type, VkDescriptorSet p_set) noexcept
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

void ShellRenderInterfaceVulkan::MemoryPool::SetDescriptorSet(uint32_t binding_index, VkSampler p_sampler, VkImageLayout layout, VkImageView p_view,
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

void ShellRenderInterfaceVulkan::MemoryPool::Free_GeometryHandle(geometry_handle_t* p_valid_geometry_handle) noexcept
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

#ifdef RMLUI_DEBUG
	Shell::Log("[Vulkan][Debug] Geometry handle is deleted! [%d]", p_valid_geometry_handle->m_descriptor_id);
#endif
}
