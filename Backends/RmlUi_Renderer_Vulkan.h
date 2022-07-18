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

#ifndef RMLUI_BACKENDS_RENDERER_VULKAN_H
#define RMLUI_BACKENDS_RENDERER_VULKAN_H

#include <RmlUi/Core/RenderInterface.h>

#ifdef RMLUI_PLATFORM_WIN32
	#include "RmlUi_Include_Windows.h"
	#define VK_USE_PLATFORM_WIN32_KHR

	// strange but VMA_DEBUG_LOG is commented in library header...
	// So if you need additional log about your memory leaks just uncomment this section
	/*
	#ifdef RMLUI_DEBUG
	    #define VMA_DEBUG_LOG(format, ...)                        \
	        do                                                    \
	        {                                                     \
	            printf(format, __VA_ARGS__);                      \
	            char str[1024];                                   \
	            sprintf_s(str, sizeof(str), format, __VA_ARGS__); \
	            OutputDebugStringA("\n");                         \
	            OutputDebugStringA(str);                          \
	        } while (false)
	#endif
	*/
#endif

#ifdef _WIN32
	#pragma warning(push, 0)
#elif __clang__
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wall"
	#pragma clang diagnostic ignored "-Wextra"
	#pragma clang diagnostic ignored "-Wnullability-extension"
	#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
	#pragma clang diagnostic ignored "-Wnullability-completeness"
#elif __GNUC__
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
	#pragma GCC diagnostic ignored "-Wunused-function"
	#pragma GCC diagnostic ignored "-Wunused-parameter"
	#pragma GCC diagnostic ignored "-Wunused-variable"
	#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
	#pragma GCC diagnostic ignored "-Wswitch"
	#pragma GCC diagnostic ignored "-Wpedantic"
	#pragma GCC diagnostic ignored "-Wattributes"
	#pragma GCC diagnostic ignored "-Wignored-qualifiers"
	#pragma GCC diagnostic ignored "-Wparentheses"
#endif

#include "RmlUi_Vulkan/spirv_reflect.h"
#include "RmlUi_Vulkan/vulkan.h"

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#include "RmlUi_Vulkan/vk_mem_alloc.h"

#ifdef _WIN32
	#pragma warning(pop)
#elif __clang__
	#pragma clang diagnostic pop
#elif __GNUC__
	#pragma GCC diagnostic pop
#endif

// probably a compiler's bug because idk how to fix it
#ifdef __clang__
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif

#ifdef RMLUI_DEBUG
	#include <RmlUi/Core/Log.h>
	#define VK_ASSERT(statement, msg, ...)                                 \
		{                                                                  \
			RMLUI_ASSERT(statement);                                       \
			if (!!(statement) == false)                                    \
				Rml::Log::Message(Rml::Log::LT_ERROR, msg, ##__VA_ARGS__); \
		}
#else
	#define VK_ASSERT(statement, msg, ...) static_cast<void>(statement)
#endif

/**
 * Low level Vulkan render interface for RmlUi
 *
 * My aim is to create compact, but easy to use class
 * I understand that it isn't good architectural choice to keep all things in one class
 * But I follow to RMLUI design and for implementing one GAPI backend it just needs one class
 * For user looks cool, but for programmer...
 *
 * It's better to try operate with very clean 'one-class' architecture rather than create own library for Vulkan
 * With many different classes, with not trivial signatures and etc
 * And as a result we should document that library so it's just a headache for all of us
 *
 * Reminder to users: If you want to implement your Vulkan renderer check previous commits of this work, because current system works only with new
 * and delete operations every frame (CPU side), on GPU we implemented the pre-allocated buffer with virtual allocs (Vma) so there's no problems and
 * all fine. I wrote all ideas and implementation for that.
 *
 * @author wh1t3lord
 */

constexpr uint32_t kSwapchainBackBufferCount = 3;
// This value goes to function that converts to Mbs, it is like N * 1024 * 1024
constexpr uint32_t kVideoMemoryForAllocation = 4;

class RenderInterface_Vulkan : public Rml::RenderInterface {
	enum class shader_type_t : int { kShaderType_Vertex, kShaderType_Pixel, kShaderType_Unknown = -1 };

	// For obtainig our VkShaderModule in m_shaders, the order is specified in LoadShaders method
	// TODO: RmlUI team think how to make not hardcoded state (or is it enough for tutorials?)
	enum class shader_id_t : int { kShaderID_Vertex, kShaderID_Pixel_WithoutTextures, kShaderID_Pixel_WithTextures };

	class shader_data_t {
	public:
		shader_data_t(const uint32_t* data, size_t data_size) noexcept : m_data(data), m_data_size(data_size) {}

		void Set_Type(VkShaderStageFlagBits type) noexcept { this->m_shader_type = type; }
		VkShaderStageFlagBits Get_Type(void) const noexcept { return this->m_shader_type; }

		const uint32_t* Get_Data(void) const noexcept { return this->m_data; }
		size_t Get_DataSize(void) const noexcept { return this->m_data_size; }

	private:
		const uint32_t* m_data = nullptr;
		size_t m_data_size = {};
		VkShaderStageFlagBits m_shader_type = {};
	};

	// don't mix, if you change the order it will copy with wrong data (idk why)
	struct shader_vertex_user_data_t {
		Rml::Matrix4f m_transform;
		Rml::Vector2f m_translate;
	};

	class texture_data_t {
	public:
		texture_data_t() :
			m_width{}, m_height{}, m_id{-1}, m_p_vk_image{}, m_p_vk_image_view{}, m_p_vk_sampler{}, m_p_vk_descriptor_set{}, m_p_vma_allocation{}
		{}

		~texture_data_t() {}

		void Set_VkImage(VkImage p_image) noexcept { this->m_p_vk_image = p_image; }
		void Set_VkImageView(VkImageView p_view) noexcept { this->m_p_vk_image_view = p_view; }
		void Set_VkSampler(VkSampler p_sampler) noexcept { this->m_p_vk_sampler = p_sampler; }
		void Set_VmaAllocation(VmaAllocation p_allocation) noexcept { this->m_p_vma_allocation = p_allocation; }
		void Set_FileName(const Rml::String& filename) noexcept { this->m_filename = filename; }
		void Set_Width(int width) noexcept { this->m_width = width; }
		void Set_Height(int height) noexcept { this->m_height = height; }
		void Set_VkDescriptorSet(VkDescriptorSet p_set) noexcept { this->m_p_vk_descriptor_set = p_set; }
		void Set_ID(int value) noexcept { this->m_id = value; }

		VkImage Get_VkImage(void) const noexcept { return this->m_p_vk_image; }
		VkImageView Get_VkImageView(void) const noexcept { return this->m_p_vk_image_view; }
		VkSampler Get_VkSampler(void) const noexcept { return this->m_p_vk_sampler; }
		VmaAllocation Get_VmaAllocation(void) const noexcept { return this->m_p_vma_allocation; }
		VkDescriptorSet Get_VkDescriptorSet(void) const noexcept { return this->m_p_vk_descriptor_set; }
		const Rml::String& Get_FileName(void) const noexcept { return this->m_filename; }
		int Get_Width(void) const noexcept { return this->m_width; }
		int Get_Height(void) const noexcept { return this->m_height; }
		int Get_ID(void) const noexcept { return this->m_id; }

	private:
		int m_width;
		int m_height;
		int m_id;
		VkImage m_p_vk_image;
		VkImageView m_p_vk_image_view;
		VkSampler m_p_vk_sampler;
		VkDescriptorSet m_p_vk_descriptor_set;
		VmaAllocation m_p_vma_allocation;
		Rml::String m_filename;
	};

	// TODO: RMLUI team add set and get methods for this structure PLEASE
	// handle of compiled geometry
	struct geometry_handle_t {
		// means it passed to vkCmd functions otherwise it is a raw geometry handle that don't rendered
		bool m_is_cached = {false};

		bool m_is_has_texture = {false};
		int m_num_indices = 0;
		int m_descriptor_id = 0;

		Rml::Vector2f m_translation{};
		Rml::Matrix4f m_transform{};

		texture_data_t* m_p_texture{};

		VkDescriptorBufferInfo m_p_vertex{};
		VkDescriptorBufferInfo m_p_index{};
		VkDescriptorBufferInfo m_p_shader{};

		// @ this is for freeing our logical blocks for VMA
		// see https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/virtual_allocator.html
		VmaVirtualAllocation m_p_vertex_allocation{};
		VmaVirtualAllocation m_p_index_allocation{};
		VmaVirtualAllocation m_p_shader_allocation{};
	};

	// possibly delete this class after all tests
	class descriptor_wrapper_t {
	public:
		descriptor_wrapper_t(void) : m_is_available{true}, m_p_set{} {}
		~descriptor_wrapper_t(void) {}

		bool Is_Available(void) const noexcept { return this->m_is_available; }
		void Set_Available(bool value) noexcept { this->m_is_available = value; }

		VkDescriptorSet Get_DescriptorSet(void) const noexcept { return this->m_p_set; }
		void Set_DescriptorSet(VkDescriptorSet p_set) noexcept
		{
			VK_ASSERT(p_set, "you can't pass an invalid VkDescriptorSet");
			this->m_p_set = p_set;
		}

	private:
		bool m_is_available;
		VkDescriptorSet m_p_set;
	};

	class buffer_data_t {
	public:
		buffer_data_t() : m_p_vk_buffer{}, m_p_vma_allocation{} {}
		~buffer_data_t() {}

		void Set_VkBuffer(VkBuffer p_buffer) noexcept { this->m_p_vk_buffer = p_buffer; }
		void Set_VmaAllocation(VmaAllocation p_allocation) noexcept { this->m_p_vma_allocation = p_allocation; }

		VkBuffer Get_VkBuffer(void) const noexcept { return this->m_p_vk_buffer; }
		VmaAllocation Get_VmaAllocation(void) const noexcept { return this->m_p_vma_allocation; }

	private:
		VkBuffer m_p_vk_buffer;
		VmaAllocation m_p_vma_allocation;
	};

	class UploadResourceManager {
	public:
		UploadResourceManager(void) : m_p_device{}, m_p_fence{}, m_p_command_buffer{}, m_p_command_pool{}, m_p_graphics_queue{} {}
		~UploadResourceManager(void) {}

		void Initialize(VkDevice p_device, VkQueue p_queue, uint32_t queue_family_index)
		{
			VK_ASSERT(p_queue, "you have to pass a valid VkQueue");
			VK_ASSERT(p_device, "you have to pass a valid VkDevice for creation resources");

			this->m_p_device = p_device;
			this->m_p_graphics_queue = p_queue;

			this->Create_All(queue_family_index);
		}

		void Shutdown(void)
		{
			vkDestroyFence(this->m_p_device, this->m_p_fence, nullptr);
			vkDestroyCommandPool(this->m_p_device, this->m_p_command_pool, nullptr);
		}

		void UploadToGPU(Rml::Function<void(VkCommandBuffer p_cmd)>&& p_user_commands) noexcept
		{
			VK_ASSERT(this->m_p_command_buffer, "you didn't initialize VkCommandBuffer");

			VkCommandBufferBeginInfo info_command = {};

			info_command.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			info_command.pNext = nullptr;
			info_command.pInheritanceInfo = nullptr;
			info_command.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			VkResult status = vkBeginCommandBuffer(this->m_p_command_buffer, &info_command);

			VK_ASSERT(status == VkResult::VK_SUCCESS, "failed to vkBeginCommandBuffer");

			p_user_commands(this->m_p_command_buffer);

			status = vkEndCommandBuffer(this->m_p_command_buffer);

			VK_ASSERT(status == VkResult::VK_SUCCESS, "faield to vkEndCommandBuffer");

			this->Submit();
			this->Wait();
		}

	private:
		void Create_Fence(void) noexcept
		{
			VkFenceCreateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			info.pNext = nullptr;
			info.flags = 0;

			VkResult status = vkCreateFence(this->m_p_device, &info, nullptr, &this->m_p_fence);

			VK_ASSERT(status == VkResult::VK_SUCCESS, "failed to vkCreateFence");
		}

		void Create_CommandBuffer(void) noexcept
		{
			VK_ASSERT(this->m_p_command_pool, "you have to initialize VkCommandPool before calling this method!");

			VkCommandBufferAllocateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			info.pNext = nullptr;
			info.commandPool = this->m_p_command_pool;
			info.commandBufferCount = 1;
			info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

			VkResult status = vkAllocateCommandBuffers(this->m_p_device, &info, &this->m_p_command_buffer);

			VK_ASSERT(status == VkResult::VK_SUCCESS, "failed to vkAllocateCommandBuffers");
		}

		void Create_CommandPool(uint32_t queue_family_index) noexcept
		{
			VkCommandPoolCreateInfo info = {};

			info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			info.pNext = nullptr;
			info.queueFamilyIndex = queue_family_index;
			info.flags = 0;

			VkResult status = vkCreateCommandPool(this->m_p_device, &info, nullptr, &this->m_p_command_pool);

			VK_ASSERT(status == VkResult::VK_SUCCESS, "failed to vkCreateCommandPool");
		}

		void Create_All(uint32_t queue_family_index) noexcept
		{
			this->Create_Fence();
			this->Create_CommandPool(queue_family_index);
			this->Create_CommandBuffer();
		}

		void Wait(void) noexcept
		{
			VK_ASSERT(this->m_p_fence, "you must initialize your VkFence");

			vkWaitForFences(this->m_p_device, 1, &this->m_p_fence, VK_TRUE, UINT64_MAX);
			vkResetFences(this->m_p_device, 1, &this->m_p_fence);
			vkResetCommandPool(this->m_p_device, this->m_p_command_pool, 0);
		}

		void Submit(void) noexcept
		{
			VkSubmitInfo info = {};

			info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			info.pNext = nullptr;
			info.waitSemaphoreCount = 0;
			info.signalSemaphoreCount = 0;
			info.pSignalSemaphores = nullptr;
			info.pWaitSemaphores = nullptr;
			info.pWaitDstStageMask = nullptr;
			info.pCommandBuffers = &this->m_p_command_buffer;
			info.commandBufferCount = 1;

			auto status = vkQueueSubmit(this->m_p_graphics_queue, 1, &info, this->m_p_fence);

			VK_ASSERT(status == VkResult::VK_SUCCESS, "failed to vkQueueSubmit");
		}

	private:
		VkDevice m_p_device;
		VkFence m_p_fence;
		VkCommandBuffer m_p_command_buffer;
		VkCommandPool m_p_command_pool;
		VkQueue m_p_graphics_queue;
	};

	class PhysicalDeviceWrapper {
	public:
		PhysicalDeviceWrapper(VkPhysicalDevice p_physical_device);
		PhysicalDeviceWrapper(void);
		~PhysicalDeviceWrapper(void);

		VkPhysicalDevice GetHandle(void) const noexcept;
		const VkPhysicalDeviceProperties& GetProperties(void) const noexcept;
		const VkPhysicalDeviceLimits& GetLimits(void) const noexcept;

	private:
		VkPhysicalDevice m_p_physical_device;
		VkPhysicalDeviceProperties m_physical_device_properties;
		VkPhysicalDeviceLimits m_physical_device_limits;
	};

	class MemoryRingAllocator {
	public:
		MemoryRingAllocator(void);
		~MemoryRingAllocator(void);

		void Initialize(uint32_t total_size) noexcept;
		uint32_t MakePaddingToAvoidCrossover(uint32_t size) const noexcept;

		uint32_t GetSize(void) const noexcept;
		uint32_t GetHead(void) const noexcept;
		uint32_t GetTail(void) const noexcept;

		bool Alloc(uint32_t size, uint32_t* p_out) noexcept;
		bool Free(uint32_t size);

	private:
		uint32_t m_head;
		uint32_t m_total_size;
		uint32_t m_allocated_size;
	};

	// Be careful frame buffer doesn't refer to Vulkan's object!
	// It means current buffer of current frame (logical abstraction)
	// @ this allocator is simple and doesn't support deletion operation, so you can't free memory from it, so we use VMA allocator that is enough
	class MemoryRingAllocatorWithTabs {
	public:
		MemoryRingAllocatorWithTabs(void);
		~MemoryRingAllocatorWithTabs(void);

		void Initialize(uint32_t number_of_frames, uint32_t total_size_memory) noexcept;
		void Shutdown(void) noexcept;
		bool Alloc(uint32_t size, uint32_t* p_out) noexcept;
		void OnBeginFrame(void) noexcept;

	private:
		uint32_t m_frame_buffer_index;
		uint32_t m_frame_buffer_number;
		uint32_t m_memory_allocated_in_frame;
		MemoryRingAllocator m_allocator;

		uint32_t m_p_allocated_memory_per_back_buffer[kSwapchainBackBufferCount];
	};

	struct MemoryPoolCreateInfo {
		uint32_t m_number_of_back_buffers;

		// @ allocate memory for 1000
		uint32_t m_gpu_data_count = 1000;

		// @ you just pass sizeof(YourDataStructureThatIdentifiesTheWholeDataPerFrame)
		uint32_t m_gpu_data_size = uint32_t(-1);

		// @ Real memory, how big we need to allocate (you pass the raw value it means if you want to allocate 32 Mbs you need to 32 * 1024 * 1024,
		// but not just only pass '32', use the appropriate functions for converting values into raw representation)
		VkDeviceSize m_memory_total_size;
	};

	// @ main manager for "allocating" vertex, index, uniform stuff
	class MemoryPool {
	public:
		MemoryPool(void);
		~MemoryPool(void);

		void Initialize(VkPhysicalDeviceProperties* p_properties, VmaAllocator p_allocator, VkDevice p_device,
			const MemoryPoolCreateInfo& info) noexcept;
		void Shutdown(void) noexcept;

		bool Alloc_GeneralBuffer(VkDeviceSize size, void** p_data, VkDescriptorBufferInfo* p_out, VmaVirtualAllocation* p_alloc) noexcept;
		bool Alloc_VertexBuffer(uint32_t number_of_elements, uint32_t stride_in_bytes, void** p_data, VkDescriptorBufferInfo* p_out,
			VmaVirtualAllocation* p_alloc) noexcept;
		bool Alloc_IndexBuffer(uint32_t number_of_elements, uint32_t stride_in_bytes, void** p_data, VkDescriptorBufferInfo* p_out,
			VmaVirtualAllocation* p_alloc) noexcept;

		void SetDescriptorSet(uint32_t binding_index, uint32_t size, VkDescriptorType descriptor_type, VkDescriptorSet p_set) noexcept;
		void SetDescriptorSet(uint32_t binding_index, VkDescriptorBufferInfo* p_info, VkDescriptorType descriptor_type,
			VkDescriptorSet p_set) noexcept;
		void SetDescriptorSet(uint32_t binding_index, VkSampler p_sampler, VkImageLayout layout, VkImageView p_view, VkDescriptorType descriptor_type,
			VkDescriptorSet p_set) noexcept;

		void Free_GeometryHandle(geometry_handle_t* p_valid_geometry_handle) noexcept;
		void Free_GeometryHandle_ShaderDataOnly(geometry_handle_t* p_valid_geometry_handle) noexcept;

	private:
		VkDeviceSize m_memory_gpu_data_total;
		VkDeviceSize m_memory_total_size;
		VkDeviceSize m_memory_gpu_data_one_object;
		VkDeviceSize m_min_alignment_for_uniform_buffer;
		char* m_p_data;
		VkBuffer m_p_buffer;
		VkPhysicalDeviceProperties* m_p_physical_device_current_properties;
		VmaAllocation m_p_buffer_alloc;
		VkDevice m_p_device;
		VmaAllocator m_p_vk_allocator;
		VmaVirtualBlock m_p_block;
	};

	// Explanation of how to use Vulkan efficiently
	// https://vkguide.dev/docs/chapter-4/double_buffering/
	class CommandListRing {
		class CommandBufferPerFrame {
		public:
			CommandBufferPerFrame(void);
			~CommandBufferPerFrame(void);

			uint32_t GetUsedCalls(void) const noexcept;
			const Rml::Vector<VkCommandPool>& GetCommandPools(void) const noexcept;
			const Rml::Vector<VkCommandBuffer>& GetCommandBuffers(void) const noexcept;

			void AddCommandPools(VkCommandPool p_command_pool) noexcept;
			void AddCommandBuffers(VkCommandBuffer p_buffer) noexcept;

			void SetUsedCalls(uint32_t number) noexcept;
			void SetCountCommandListsAndPools(uint32_t number) noexcept;

		private:
			uint32_t m_used_calls;
			uint32_t m_number_per_frame_command_lists;
			Rml::Vector<VkCommandPool> m_command_pools;
			Rml::Vector<VkCommandBuffer> m_command_buffers;
		};

	public:
		CommandListRing(void);
		~CommandListRing(void);

		void Initialize(VkDevice p_device, uint32_t queue_index_graphics, uint32_t number_of_back_buffers, uint32_t command_list_per_frame) noexcept;
		void Shutdown(void);

		void OnBeginFrame(void);

		VkCommandBuffer GetNewCommandList(void);

		const Rml::Vector<VkCommandBuffer>& GetAllCommandBuffersForCurrentFrame(void) const noexcept;
		uint32_t GetCountOfCommandBuffersPerFrame(void) const noexcept;

	private:
		uint32_t m_frame_index;
		uint32_t m_number_of_frames;
		uint32_t m_command_lists_per_frame;
		VkDevice m_p_device;
		CommandBufferPerFrame* m_p_current_frame;
		Rml::Vector<CommandBufferPerFrame> m_frames;
	};

	class DescriptorPoolManager {
	public:
		DescriptorPoolManager(void) : m_allocated_descriptor_count{}, m_p_descriptor_pool{} {}
		~DescriptorPoolManager(void)
		{
			VK_ASSERT(this->m_allocated_descriptor_count <= 0, "something is wrong. You didn't free some VkDescriptorSet");
		}

		void Initialize(VkDevice p_device, uint32_t count_uniform_buffer, uint32_t count_image_sampler, uint32_t count_sampler,
			uint32_t count_storage_buffer) noexcept
		{
			VK_ASSERT(p_device, "you can't pass an invalid VkDevice here");

			Rml::Array<VkDescriptorPoolSize, 5> sizes;

			sizes[0] = {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, count_uniform_buffer};
			sizes[1] = {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, count_uniform_buffer};
			sizes[2] = {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, count_image_sampler};
			sizes[3] = {VK_DESCRIPTOR_TYPE_SAMPLER, count_sampler};
			sizes[4] = {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, count_storage_buffer};

			VkDescriptorPoolCreateInfo info = {};

			info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			info.pNext = nullptr;
			info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
			info.maxSets = 1000;
			info.poolSizeCount = static_cast<uint32_t>(sizes.size());
			info.pPoolSizes = sizes.data();

			auto status = vkCreateDescriptorPool(p_device, &info, nullptr, &this->m_p_descriptor_pool);

			VK_ASSERT(status == VkResult::VK_SUCCESS, "failed to vkCreateDescriptorPool");
		}

		void Shutdown(VkDevice p_device)
		{
			VK_ASSERT(p_device, "you can't pass an invalid VkDevice here");

			vkDestroyDescriptorPool(p_device, this->m_p_descriptor_pool, nullptr);
		}

		uint32_t Get_AllocatedDescriptorCount(void) const noexcept { return this->m_allocated_descriptor_count; }

		bool Alloc_Descriptor(VkDevice p_device, VkDescriptorSetLayout* p_layouts, VkDescriptorSet* p_sets,
			uint32_t descriptor_count_for_creation = 1) noexcept
		{
			VK_ASSERT(p_layouts, "you have to pass a valid and initialized VkDescriptorSetLayout (probably you must create it)");
			VK_ASSERT(p_device, "you must pass a valid VkDevice here");

			VkDescriptorSetAllocateInfo info = {};

			info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			info.pNext = nullptr;
			info.descriptorPool = this->m_p_descriptor_pool;
			info.descriptorSetCount = descriptor_count_for_creation;
			info.pSetLayouts = p_layouts;

			auto status = vkAllocateDescriptorSets(p_device, &info, p_sets);

			VK_ASSERT(status == VkResult::VK_SUCCESS, "failed to vkAllocateDescriptorSets");

			this->m_allocated_descriptor_count += descriptor_count_for_creation;

			return status == VkResult::VK_SUCCESS;
		}

		void Free_Descriptors(VkDevice p_device, VkDescriptorSet* p_sets, uint32_t descriptor_count = 1) noexcept
		{
			VK_ASSERT(p_device, "you must pass a valid VkDevice here");

			if (p_sets)
			{
				this->m_allocated_descriptor_count -= descriptor_count;
				vkFreeDescriptorSets(p_device, this->m_p_descriptor_pool, descriptor_count, p_sets);
			}
		}

	private:
		int m_allocated_descriptor_count;
		VkDescriptorPool m_p_descriptor_pool;
	};

public:
	RenderInterface_Vulkan();
	~RenderInterface_Vulkan(void);

	using CreateSurfaceCallback = bool (*)(VkInstance instance, VkSurfaceKHR* out_surface);

	bool Initialize(CreateSurfaceCallback create_surface_callback);
	void Shutdown();

	void SetViewport(int viewport_width, int viewport_height);
	void BeginFrame();
	void EndFrame();

	// -- Inherited from Rml::RenderInterface --

	/// Called by RmlUi when it wants to render geometry that it does not wish to optimise.
	void RenderGeometry(Rml::Vertex* vertices, int num_vertices, int* indices, int num_indices, Rml::TextureHandle texture,
		const Rml::Vector2f& translation) override;

	/// Called by RmlUi when it wants to compile geometry it believes will be static for the forseeable future.
	Rml::CompiledGeometryHandle CompileGeometry(Rml::Vertex* vertices, int num_vertices, int* indices, int num_indices,
		Rml::TextureHandle texture) override;

	/// Called by RmlUi when it wants to render application-compiled geometry.
	void RenderCompiledGeometry(Rml::CompiledGeometryHandle geometry, const Rml::Vector2f& translation) override;

	/// Called by RmlUi when it wants to release application-compiled geometry.
	void ReleaseCompiledGeometry(Rml::CompiledGeometryHandle geometry) override;

	/// Called by RmlUi when it wants to enable or disable scissoring to clip content.
	void EnableScissorRegion(bool enable) override;
	/// Called by RmlUi when it wants to change the scissor region.
	void SetScissorRegion(int x, int y, int width, int height) override;

	/// Called by RmlUi when a texture is required by the library.
	bool LoadTexture(Rml::TextureHandle& texture_handle, Rml::Vector2i& texture_dimensions, const Rml::String& source) override;
	/// Called by RmlUi when a texture is required to be built from an internally-generated sequence of pixels.
	bool GenerateTexture(Rml::TextureHandle& texture_handle, const Rml::byte* source, const Rml::Vector2i& source_dimensions) override;
	/// Called by RmlUi when a loaded texture is no longer required.
	void ReleaseTexture(Rml::TextureHandle texture_handle) override;

	/// Called by RmlUi when it wants to set the current transform matrix to a new matrix.
	void SetTransform(const Rml::Matrix4f* transform) override;

	// AlignUp(314, 256) = 512
	template <typename T>
	static T AlignUp(T val, T alignment)
	{
		return (val + alignment - (T)1) & ~(alignment - (T)1);
	}

	// Example: opposed function to ConvertValueToMegabytes
	// TranslateBytesToMegaBytes(52428800) returns 52428800 / (1024 * 1024) = 50 <= value indicates that it's 50 Megabytes
	static VkDeviceSize TranslateBytesToMegaBytes(VkDeviceSize raw_number) noexcept { return raw_number / (1024 * 1024); }

	// Example: ConvertValueToMegabytes(50) returns 50 * 1024 * 1024 = 52428800 BYTES!!!!
	static VkDeviceSize ConvertMegabytesToBytes(VkDeviceSize value_shows_megabytes) noexcept { return value_shows_megabytes * 1024 * 1024; }

	void OnWindowMinimize(void);
	void OnWindowRestored(void);
	void OnWindowMaximize(void);
	void OnWindowHidden(void);
	void OnWindowShown(void);

private:
	void OnResize(int width, int height) noexcept;

	void Initialize_Instance(void) noexcept;
	void Initialize_Device(void) noexcept;
	void Initialize_PhysicalDevice(void) noexcept;
	void Initialize_Swapchain(void) noexcept;
	void Initialize_Surface(CreateSurfaceCallback create_surface_callback) noexcept;
	void Initialize_QueueIndecies(void) noexcept;
	void Initialize_Queues(void) noexcept;
	void Initialize_SyncPrimitives(void) noexcept;
	void Initialize_Resources(void) noexcept;
	void Initialize_Allocator(void) noexcept;

	void Destroy_Instance(void) noexcept;
	void Destroy_Device() noexcept;
	void Destroy_Swapchain(void) noexcept;
	void Destroy_Surface(void) noexcept;
	void Destroy_SyncPrimitives(void) noexcept;
	void Destroy_Resources(void) noexcept;
	void Destroy_Allocator(void) noexcept;

	void QueryInstanceLayers(void) noexcept;
	void QueryInstanceExtensions(void) noexcept;
	bool AddLayerToInstance(const char* p_instance_layer_name) noexcept;
	bool AddExtensionToInstance(const char* p_instance_extension_name) noexcept;
	void CreatePropertiesFor_Instance(void) noexcept;

	bool IsLayerPresent(const Rml::Vector<VkLayerProperties>& properties, const char* p_layer_name) noexcept;
	bool IsExtensionPresent(const Rml::Vector<VkExtensionProperties>& properties, const char* p_extension_name) noexcept;

	bool AddExtensionToDevice(const char* p_device_extension_name) noexcept;
	void CreatePropertiesFor_Device(void) noexcept;

	void CreateReportDebugCallback(void) noexcept;
	void Destroy_ReportDebugCallback(void) noexcept;

	uint32_t GetUserAPIVersion(void) const noexcept;
	uint32_t GetRequiredVersionAndValidateMachine(void) noexcept;

	void CollectPhysicalDevices(void) noexcept;
	bool ChoosePhysicalDevice(VkPhysicalDeviceType device_type) noexcept;
	void PrintInformationAboutPickedPhysicalDevice(VkPhysicalDevice p_physical_device) noexcept;

	VkSurfaceFormatKHR ChooseSwapchainFormat(void) noexcept;
	VkSurfaceTransformFlagBitsKHR CreatePretransformSwapchain(void) noexcept;
	VkCompositeAlphaFlagBitsKHR ChooseSwapchainCompositeAlpha(void) noexcept;
	int Choose_SwapchainImageCount(uint32_t user_swapchain_count_for_creation = kSwapchainBackBufferCount, bool if_failed_choose_min = true) noexcept;
	VkPresentModeKHR GetPresentMode(VkPresentModeKHR type = VkPresentModeKHR::VK_PRESENT_MODE_FIFO_KHR) noexcept;
	VkSurfaceCapabilitiesKHR GetSurfaceCapabilities(void) noexcept;
	Rml::Vector<RenderInterface_Vulkan::shader_data_t> LoadShaders(void) noexcept;

	VkExtent2D Choose_ValidSwapchainExtent(void) noexcept;

	// true = means the extent is valid; otherwise it is invalid.
	bool Check_IfSwapchainExtentValid(void) noexcept;

	void CreateShaders(const Rml::Vector<shader_data_t>& storage) noexcept;
	void CreateDescriptorSetLayout(Rml::Vector<shader_data_t>& storage) noexcept;
	Rml::Vector<VkDescriptorSetLayoutBinding> CreateDescriptorSetLayoutBindings(shader_data_t& data) noexcept;
	void CreatePipelineLayout(void) noexcept;
	void CreateDescriptorSets(void) noexcept;
	void CreateSamplers(void) noexcept;
	void Create_Pipelines(void) noexcept;
	void CreateRenderPass(void) noexcept;

	void CreateSwapchainFrameBuffers(void) noexcept;

	// This method is called in Views, so don't call it manually
	void CreateSwapchainImages(void) noexcept;
	void CreateSwapchainImageViews(void) noexcept;

	void Create_DepthStencilImage(void) noexcept;
	void Create_DepthStencilImageViews(void) noexcept;

	void CreateResourcesDependentOnSize(void) noexcept;

	buffer_data_t CreateResource_StagingBuffer(VkDeviceSize size, VkBufferUsageFlags flags) noexcept;
	void DestroyResource_StagingBuffer(const buffer_data_t& data) noexcept;

	void Destroy_Textures(void) noexcept;
	void Destroy_Geometries(void) noexcept;

	void Destroy_Texture(const texture_data_t& p_texture) noexcept;

	void DestroyResourcesDependentOnSize(void) noexcept;
	void DestroySwapchainImageViews(void) noexcept;
	void DestroySwapchainFrameBuffers(void) noexcept;
	void DestroyRenderPass(void) noexcept;
	void Destroy_Pipelines(void) noexcept;
	void DestroyDescriptorSets(void) noexcept;
	void DestroyPipelineLayout(void) noexcept;
	void DestroySamplers(void) noexcept;

	void Wait(void) noexcept;

	void Update_PendingForDeletion_Textures_By_Frames(void) noexcept;
	void Update_PendingForDeletion_Geometries(void) noexcept;

	void Submit(void) noexcept;
	void Present(void) noexcept;

	uint32_t Get_CurrentDescriptorID(void) const noexcept { return this->m_current_descriptor_id; }

	VkFormat Get_SupportedDepthFormat(void);

private:
	bool m_is_transform_enabled;
	bool m_is_apply_to_regular_geometry_stencil;
	bool m_is_use_scissor_specified;
	bool m_is_use_stencil_pipeline;
	bool m_is_can_render;

	int m_width;
	int m_height;

	uint32_t m_queue_index_present;
	uint32_t m_queue_index_graphics;
	uint32_t m_queue_index_compute;
	uint32_t m_semaphore_index;
	uint32_t m_semaphore_index_previous;
	uint32_t m_image_index;

	// for obtaining descriptor sets from m_descriptor_sets[pass_here_this_variable]
	// Don't use this variable in raw manner like this-> or just directly access, only through Get_ and Next method because for easy debugging
	// Especially when we say about multithreading...
	uint32_t m_current_descriptor_id;

	VkInstance m_p_instance;
	VkDevice m_p_device;
	VkPhysicalDevice m_p_physical_device_current;
	VkPhysicalDeviceProperties m_current_physical_device_properties;
	VkSurfaceKHR m_p_surface;
	VkSwapchainKHR m_p_swapchain;
	VmaAllocator m_p_allocator;
	// @ obtained from command list see PrepareRenderBuffer method
	VkCommandBuffer m_p_current_command_buffer;

	VkDescriptorSetLayout m_p_descriptor_set_layout_uniform_buffer_dynamic;
	VkDescriptorSetLayout m_p_descriptor_set_layout_for_textures;
	VkPipelineLayout m_p_pipeline_layout;
	VkPipeline m_p_pipeline_with_textures;
	VkPipeline m_p_pipeline_without_textures;
	VkPipeline m_p_pipeline_stencil_for_region_where_geometry_will_be_drawn;
	VkPipeline m_p_pipeline_stencil_for_regular_geometry_that_applied_to_region_with_textures;
	VkPipeline m_p_pipeline_stencil_for_regular_geometry_that_applied_to_region_without_textures;
	VkDescriptorSet m_p_descriptor_set;
	VkRenderPass m_p_render_pass;
	VkSampler m_p_sampler_linear;
	VkRect2D m_scissor;

	// @ means it captures the window size full width and full height, offset equals both x and y to 0
	VkRect2D m_scissor_original;
	VkViewport m_viewport;

	VkQueue m_p_queue_present;
	VkQueue m_p_queue_graphics;
	VkQueue m_p_queue_compute;

#ifdef RMLUI_DEBUG
	VkDebugReportCallbackEXT m_debug_report_callback_instance;
#endif

	VkSurfaceFormatKHR m_swapchain_format;
	shader_vertex_user_data_t m_user_data_for_vertex_shader;
	texture_data_t m_texture_depthstencil;

	Rml::Matrix4f m_projection;
	Rml::Vector<PhysicalDeviceWrapper> m_physical_devices;
	Rml::Vector<VkLayerProperties> m_instance_layer_properties;
	Rml::Vector<VkExtensionProperties> m_instance_extension_properties;
	Rml::Vector<VkExtensionProperties> m_device_extension_properties;
	Rml::Vector<const char*> m_instance_layer_names;
	Rml::Vector<const char*> m_instance_extension_names;
	Rml::Vector<const char*> m_device_extension_names;
	Rml::Vector<VkFence> m_executed_fences;
	Rml::Vector<VkSemaphore> m_semaphores_image_available;
	Rml::Vector<VkSemaphore> m_semaphores_finished_render;
	Rml::Vector<VkFramebuffer> m_swapchain_frame_buffers;
	Rml::Vector<VkImage> m_swapchain_images;
	Rml::Vector<VkImageView> m_swapchain_image_views;
	Rml::Vector<VkShaderModule> m_shaders;
	Rml::Vector<Rml::Vector<texture_data_t*>> m_pending_for_deletion_textures_by_frames;

	// vma handles that thing, so there's no need for frame splitting
	Rml::Vector<geometry_handle_t*> m_pending_for_deletion_geometries;

	CommandListRing m_command_list;
	MemoryPool m_memory_pool;
	UploadResourceManager m_upload_manager;
	DescriptorPoolManager m_manager_descriptors;
};

#ifdef __clang__
	#pragma clang diagnostic pop
#endif

#endif
