#ifndef RMLUI_SHELL_SHELLRENDERINTERFACEVULKAN_H
#define RMLUI_SHELL_SHELLRENDERINTERFACEVULKAN_H

#include "ShellOpenGL.h"
#include <RmlUi/Core/RenderInterface.h>

// TODO: add preprocessor definition in case if cmake found Vulkan package
#include "spirv_reflect.h"
#include "vk_mem_alloc.h"

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
 * @author wh1t3lord
 */

#pragma region System Constants for Vulkan API
constexpr uint32_t kSwapchainBackBufferCount = 3;

// I will finish it, so the thing is to use dynamic offset for VkDescriptorSet
constexpr uint32_t kDescriptorSetsCount = 100;

// This value goes to function that converts to Mbs, it is like 32 * 1024 * 1024
constexpr uint32_t kVideoMemoryForAllocation = 32;

// m_textures.reserve()
constexpr uint32_t kTexturesForReserve = 100;
#pragma endregion

class ShellRenderInterfaceVulkan : public Rml::RenderInterface, public ShellRenderInterfaceExtensions {
	enum class shader_type_t : int { kShaderType_Vertex, kShaderType_Pixel, kShaderType_Unknown = -1 };

	// For obtainig our VkShaderModule in m_shaders, the order is specified in LoadShaders method
	// TODO: RmlUI team think how to make not hardcoded state (or is it enough for tutorials?)
	enum class shader_id_t : int { kShaderID_Vertex, kShaderID_Pixel_WithoutTextures, kShaderID_Pixel_WithTextures };

	class shader_data_t {
	public:
		shader_data_t(void) : m_shader_type{} {}
		~shader_data_t(void) {}

		void Set_Type(VkShaderStageFlagBits type) noexcept { this->m_shader_type = type; }
		VkShaderStageFlagBits Get_Type(void) const noexcept { return this->m_shader_type; }

		const Rml::Vector<uint32_t>& Get_Data(void) const noexcept { return this->m_data; }
		void Set_Data(const Rml::Vector<uint32_t>& data) noexcept { this->m_data = data; }

	private:
		Rml::Vector<uint32_t> m_data;
		VkShaderStageFlagBits m_shader_type;
	};

	// don't mix, if you change the order it will copy with wrong data (idk why)
	struct shader_vertex_user_data_t {
		Rml::Matrix4f m_transform;
		Rml::Vector2f m_translate;
	};

	class texture_data_t {
	public:
		texture_data_t() : m_width{}, m_height{}, m_p_vk_image{}, m_p_vk_image_view{}, m_p_vk_sampler{}, m_p_vma_allocation{} {}
		~texture_data_t() {}

		void Set_VkImage(VkImage p_image) noexcept { this->m_p_vk_image = p_image; }
		void Set_VkImageView(VkImageView p_view) noexcept { this->m_p_vk_image_view = p_view; }
		void Set_VkSampler(VkSampler p_sampler) noexcept { this->m_p_vk_sampler = p_sampler; }
		void Set_VmaAllocation(VmaAllocation p_allocation) noexcept { this->m_p_vma_allocation = p_allocation; }
		void Set_FileName(const Rml::String& filename) noexcept { this->m_filename = filename; }
		void Set_Width(int width) noexcept { this->m_width = width; }
		void Set_Height(int height) noexcept { this->m_height = height; }

		VkImage Get_VkImage(void) const noexcept { return this->m_p_vk_image; }
		VkImageView Get_VkImageView(void) const noexcept { return this->m_p_vk_image_view; }
		VkSampler Get_VkSampler(void) const noexcept { return this->m_p_vk_sampler; }
		VmaAllocation Get_VmaAllocation(void) const noexcept { return this->m_p_vma_allocation; }
		const Rml::String& Get_FileName(void) const noexcept { return this->m_filename; }
		int Get_Width(void) const noexcept { return this->m_width; }
		int Get_Height(void) const noexcept { return this->m_height; }

	private:
		int m_width;
		int m_height;
		VkImage m_p_vk_image;
		VkImageView m_p_vk_image_view;
		VkSampler m_p_vk_sampler;
		VmaAllocation m_p_vma_allocation;
		Rml::String m_filename;
	};

	// TODO: RMLUI team add set and get methods for this structure PLEASE
	// handle of compiled geometry
	struct geometry_handle_t {
		// means it passed to vkCmd functions otherwise it is a raw geometry handle that don't rendered
		bool m_is_draw = {false};

		bool m_is_has_texture = {false};
		int m_num_indices = 0;
		int m_descriptor_id = 0;
		VkDescriptorBufferInfo m_p_vertex{};
		VkDescriptorBufferInfo m_p_index{};
		VkDescriptorBufferInfo m_p_shader{};

		// @ this is for freeing our logical block for VMA
		// see https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/virtual_allocator.html
		VmaVirtualAllocation m_p_allocation{};
		VmaVirtualBlock m_p_block{}; 
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
			RMLUI_ASSERT(p_queue, "you have to pass a valid VkQueue");
			RMLUI_ASSERT(p_device, "you have to pass a valid VkDevice for creation resources");

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
			RMLUI_ASSERT(this->m_p_command_buffer, "you didn't initialize VkCommandBuffer");

			VkCommandBufferBeginInfo info_command = {};

			info_command.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			info_command.pNext = nullptr;
			info_command.pInheritanceInfo = nullptr;
			info_command.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			VkResult status = vkBeginCommandBuffer(this->m_p_command_buffer, &info_command);

			RMLUI_ASSERT(status == VkResult::VK_SUCCESS, "failed to vkBeginCommandBuffer");

			p_user_commands(this->m_p_command_buffer);

			status = vkEndCommandBuffer(this->m_p_command_buffer);

			RMLUI_ASSERT(status == VkResult::VK_SUCCESS, "faield to vkEndCommandBuffer");

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

			RMLUI_ASSERT(status == VkResult::VK_SUCCESS, "failed to vkCreateFence");
		}

		void Create_CommandBuffer(void) noexcept
		{
			RMLUI_ASSERT(this->m_p_command_pool, "you have to initialize VkCommandPool before calling this method!");

			VkCommandBufferAllocateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			info.pNext = nullptr;
			info.commandPool = this->m_p_command_pool;
			info.commandBufferCount = 1;
			info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

			VkResult status = vkAllocateCommandBuffers(this->m_p_device, &info, &this->m_p_command_buffer);

			RMLUI_ASSERT(status == VkResult::VK_SUCCESS, "failed to vkAllocateCommandBuffers");
		}

		void Create_CommandPool(uint32_t queue_family_index) noexcept
		{
			VkCommandPoolCreateInfo info = {};

			info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			info.pNext = nullptr;
			info.queueFamilyIndex = queue_family_index;
			info.flags = 0;

			VkResult status = vkCreateCommandPool(this->m_p_device, &info, nullptr, &this->m_p_command_pool);

			RMLUI_ASSERT(status == VkResult::VK_SUCCESS, "failed to vkCreateCommandPool");
		}

		void Create_All(uint32_t queue_family_index) noexcept
		{
			this->Create_Fence();
			this->Create_CommandPool(queue_family_index);
			this->Create_CommandBuffer();
		}

		void Wait(void) noexcept
		{
			RMLUI_ASSERT(this->m_p_fence, "you must initialize your VkFence");

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

			RMLUI_ASSERT(status == VkResult::VK_SUCCESS, "failed to vkQueueSubmit");
		}

	private:
		VkDevice m_p_device;
		VkFence m_p_fence;
		VkCommandBuffer m_p_command_buffer;
		VkCommandPool m_p_command_pool;
		VkQueue m_p_graphics_queue;
	};

	class StatisticsWrapper {
	public:
		StatisticsWrapper(void) : m_memory_allocated_for_textures{}, m_memory_allocated_for_vertex_geometry_buffer{} {}
		~StatisticsWrapper(void) {}

		uint32_t Get_MemoryTotal(void) const noexcept
		{
			return this->Get_MemoryAllocatedForTextures() + this->Get_MemoryAllocatedForVertexGeometryBuffer();
		}

		void AddMemoryTo_AllocatedForTextures(uint32_t value) noexcept { this->m_memory_allocated_for_textures += value; }
		uint32_t Get_MemoryAllocatedForTextures(void) const noexcept { return this->m_memory_allocated_for_textures; }

		void AddMemoryTo_AllocatedForVertexGeometryBuffer(uint32_t value) noexcept { this->m_memory_allocated_for_vertex_geometry_buffer += value; }
		uint32_t Get_MemoryAllocatedForVertexGeometryBuffer(void) const noexcept { return this->m_memory_allocated_for_vertex_geometry_buffer; }

	private:
		uint32_t m_memory_allocated_for_textures;
		uint32_t m_memory_allocated_for_vertex_geometry_buffer;
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

	class MemoryRingPool {
	public:
		MemoryRingPool(void);
		~MemoryRingPool(void);

		void Initialize(VkPhysicalDeviceProperties* p_properties, VmaAllocator p_allocator, VkDevice p_device, uint32_t number_of_back_buffers,
			uint32_t memory_total_size) noexcept;
		void Shutdown(void) noexcept;

		bool AllocConstantBuffer(uint32_t size, void** p_data, VkDescriptorBufferInfo* p_out) noexcept;
		VkDescriptorBufferInfo AllocConstantBuffer(uint32_t size, void* p_data) noexcept;
		bool AllocVertexBuffer(uint32_t number_of_elements, uint32_t stride_in_bytes, void** p_data, VkDescriptorBufferInfo* p_out) noexcept;
		bool AllocIndexBuffer(uint32_t number_of_elements, uint32_t stride_in_bytes, void** p_data, VkDescriptorBufferInfo* p_out) noexcept;
		void OnBeginFrame(void) noexcept;
		void SetDescriptorSet(uint32_t binding_index, uint32_t size, VkDescriptorType descriptor_type, VkDescriptorSet p_set) noexcept;
		void SetDescriptorSet(
			uint32_t binding_index, VkDescriptorBufferInfo* p_info, VkDescriptorType descriptor_type, VkDescriptorSet p_set) noexcept;
		void SetDescriptorSet(uint32_t binding_index, VkSampler p_sampler, VkImageLayout layout, VkImageView p_view, VkDescriptorType descriptor_type,
			VkDescriptorSet p_set) noexcept;

	private:
		uint32_t m_memory_total_size;
		uint32_t m_align_koef;
		char* m_p_data;
		VkBuffer m_p_buffer;
		VkPhysicalDeviceProperties* m_p_physical_device_current_properties;
		VmaAllocation m_p_buffer_alloc;
		VkDevice m_p_device;
		VmaAllocator m_p_vk_allocator;
		MemoryRingAllocatorWithTabs m_allocator;
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
		DescriptorPoolManager(void) {}
		~DescriptorPoolManager(void) {}

		void Initialize(VkDevice p_device, uint32_t count_uniform_buffer, uint32_t count_image_sampler, uint32_t count_sampler,
			uint32_t count_storage_buffer) noexcept
		{
			RMLUI_ASSERT(p_device, "you can't pass an invalid VkDevice here");

			const VkDescriptorPoolSize sizes[] = {{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, count_uniform_buffer},
				{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, count_uniform_buffer}, {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, count_image_sampler},
				{VK_DESCRIPTOR_TYPE_SAMPLER, count_sampler}, {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, count_storage_buffer}};

			VkDescriptorPoolCreateInfo info = {};

			info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			info.pNext = nullptr;
			info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
			info.maxSets = 1000;
			info.poolSizeCount = _countof(sizes);
			info.pPoolSizes = sizes;

			auto status = vkCreateDescriptorPool(p_device, &info, nullptr, &this->m_p_descriptor_pool);

			RMLUI_ASSERT(status == VkResult::VK_SUCCESS, "failed to vkCreateDescriptorPool");
		}

		void Shutdown(VkDevice p_device)
		{
			RMLUI_ASSERT(p_device, "you can't pass an invalid VkDevice here");

			vkDestroyDescriptorPool(p_device, this->m_p_descriptor_pool, nullptr);
		}

		uint32_t Get_AllocatedDescriptorCount(void) const noexcept { return this->m_allocated_descriptor_count; }

		bool Alloc_Descriptor(
			VkDevice p_device, VkDescriptorSetLayout* p_layouts, VkDescriptorSet* p_sets, uint32_t descriptor_count_for_creation = 1) noexcept
		{
			RMLUI_ASSERT(p_layouts, "you have to pass a valid and initialized VkDescriptorSetLayout (probably you must create it)");
			RMLUI_ASSERT(p_device, "you must pass a valid VkDevice here");

			VkDescriptorSetAllocateInfo info = {};

			info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			info.pNext = nullptr;
			info.descriptorPool = this->m_p_descriptor_pool;
			info.descriptorSetCount = descriptor_count_for_creation;
			info.pSetLayouts = p_layouts;

			auto status = vkAllocateDescriptorSets(p_device, &info, p_sets);

			RMLUI_ASSERT(status == VkResult::VK_SUCCESS, "failed to vkAllocateDescriptorSets");

			this->m_allocated_descriptor_count += descriptor_count_for_creation;

			return status == VkResult::VK_SUCCESS;
		}

		void Free_Descriptors(VkDevice p_device, VkDescriptorSet* p_sets, uint32_t descriptor_count = 1) noexcept
		{
			RMLUI_ASSERT(p_device, "you must pass a valid VkDevice here");

			if (p_sets)
			{
				--this->m_allocated_descriptor_count;
				vkFreeDescriptorSets(p_device, this->m_p_descriptor_pool, descriptor_count, p_sets);
			}
		}

	private:
		uint32_t m_allocated_descriptor_count;
		VkDescriptorPool m_p_descriptor_pool;
	};

public:
	ShellRenderInterfaceVulkan();
	~ShellRenderInterfaceVulkan(void);
	/// Called by RmlUi when it wants to render geometry that it does not wish to optimise.
	void RenderGeometry(Rml::Vertex* vertices, int num_vertices, int* indices, int num_indices, Rml::TextureHandle texture,
		const Rml::Vector2f& translation) override;

	/// Called by RmlUi when it wants to compile geometry it believes will be static for the forseeable future.
	Rml::CompiledGeometryHandle CompileGeometry(
		Rml::Vertex* vertices, int num_vertices, int* indices, int num_indices, Rml::TextureHandle texture) override;

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

	/*
	    // Extensions used by the test suite
	    struct Image {
	        int width = 0;
	        int height = 0;
	        int num_components = 0;
	        Rml::UniquePtr<Rml::byte[]> data;
	    };
	    Image CaptureScreen();*/

	// ShellRenderInterfaceExtensions
	void SetViewport(int width, int height) override;
	bool AttachToNative(void* nativeWindow) override;
	void DetachFromNative(void) override;
	void PrepareRenderBuffer(void) override;
	void PresentRenderBuffer(void) override;

	// AlignUp(314, 256) = 512
	template <typename T>
	static T AlignUp(T val, T alignment)
	{
		return (val + alignment - (T)1) & ~(alignment - (T)1);
	}

	// Example: opposed function to ConvertValueToMegabytes
	// TranslateBytesToMegaBytes(52428800) returns 52428800 / (1024 * 1024) = 50 <= value indicates that it's 50 Megabytes
	static uint32_t TranslateBytesToMegaBytes(uint32_t raw_number) noexcept { return raw_number / (1024 * 1024); }

	// Example: ConvertValueToMegabytes(50) returns 50 * 1024 * 1024 = 52428800 BYTES!!!!
	static uint32_t ConvertMegabytesToBytes(uint32_t value_shows_megabytes) noexcept { return value_shows_megabytes * 1024 * 1024; }

#pragma region New Methods
private:
	void Initialize(void) noexcept;
	void Shutdown(void) noexcept;

	void OnResize(int width, int height) noexcept;

	void Initialize_Instance(void) noexcept;
	void Initialize_Device(void) noexcept;
	void Initialize_PhysicalDevice(void) noexcept;
	void Initialize_Swapchain(void) noexcept;
	void Initialize_Surface(void) noexcept;
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
	VkPresentModeKHR GetPresentMode(VkPresentModeKHR type = VkPresentModeKHR::VK_PRESENT_MODE_FIFO_KHR) noexcept;
	VkSurfaceCapabilitiesKHR GetSurfaceCapabilities(void) noexcept;
	Rml::Vector<ShellRenderInterfaceVulkan::shader_data_t> LoadShaders(void) noexcept;
	shader_data_t LoadShader(const Rml::String& relative_path_from_samples_folder_with_file_and_fileformat) noexcept;

	VkExtent2D CreateValidSwapchainExtent(void) noexcept;
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

	void CreateResourcesDependentOnSize(void) noexcept;

	buffer_data_t CreateResource_StagingBuffer(VkDeviceSize size, VkBufferUsageFlags flags) noexcept;
	void DestroyResource_StagingBuffer(const buffer_data_t& data) noexcept;

	void Destroy_Textures(void) noexcept;

	void DestroyResourcesDependentOnSize(void) noexcept;
	void DestroySwapchainImageViews(void) noexcept;
	void DestroySwapchainFrameBuffers(void) noexcept;
	void DestroyRenderPass(void) noexcept;
	void Destroy_Pipelines(void) noexcept;
	void DestroyDescriptorSets(void) noexcept;
	void DestroyPipelineLayout(void) noexcept;
	void DestroySamplers(void) noexcept;

	void Wait(void) noexcept;
	void Submit(void) noexcept;
	void Present(void) noexcept;

	void NextDescriptorID(void) noexcept
	{
		// don't do anything because we don't initialize stuff
		if (this->m_descriptor_sets.empty())
			return;

		++this->m_current_descriptor_id;
		RMLUI_ASSERT(this->m_current_descriptor_id < this->m_descriptor_sets.size(),
			"you can't have more than m_descriptor_sets's size, otherwise it means you want to use more descriptor sets than it is specified in "
			"kDescriptorSetsCount constant. Overflow!");
	}

	uint32_t Get_CurrentDescriptorID(void) const noexcept { return this->m_current_descriptor_id; }

	VkDescriptorSet Get_DescriptorSet(uint32_t id) const noexcept
	{
		if (this->m_descriptor_sets.empty())
			return static_cast<VkDescriptorSet>(nullptr);

		RMLUI_ASSERT(id < this->m_descriptor_sets.size(),
			"you can't have more than m_descriptor_sets's size, otherwise it means you want to use more descriptor sets than it is specified in "
			"kDescriptorSetsCount constant. Overflow!");

		return this->m_descriptor_sets.at(id);
	}

#pragma region Resource management
#pragma endregion

#pragma endregion

private:
	bool m_is_transform_enabled;
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
#pragma region Resources
	VkDescriptorSetLayout m_p_descriptor_set_layout;
	VkPipelineLayout m_p_pipeline_layout;
	VkPipeline m_p_pipeline_with_textures;
	VkPipeline m_p_pipeline_without_textures;
	VkDescriptorSet m_p_descriptor_set;
	VkRenderPass m_p_render_pass;
	VkSampler m_p_sampler_nearest;
	VkRect2D m_scissor;
	VkViewport m_viewport;
#pragma endregion

#if defined(RMLUI_PLATFORM_MACOSX)
#elif defined(RMLUI_PLATFORM_LINUX)
#elif defined(RMLUI_PLATFORM_WIN32)
	HWND m_p_window_handle;
#else
	#error Platform is undefined and it doesn't supported by the RMLUI
#endif

	VkQueue m_p_queue_present;
	VkQueue m_p_queue_graphics;
	VkQueue m_p_queue_compute;

	VkDebugReportCallbackEXT m_debug_report_callback_instance;

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

#pragma region Resources
	Rml::Vector<VkShaderModule> m_shaders;
	Rml::Vector<VkDescriptorSet> m_descriptor_sets;

	// this need for rendering constantly, but if we has flag m_is_draw as true we just directly pass it to vkCmdXXX commands otherwise we have to
	// update the uniform buffer and remember commands anyway.
	// I wanted to use Vector but a new element makes pointer to element in invalid state, so it is impossible to use pointers with Rml::Vector, so
	// here we have a map...
	Rml::UnorderedMap<uint32_t, geometry_handle_t> m_compiled_geometries;
	Rml::Vector<texture_data_t> m_textures;
#pragma endregion

	VkPhysicalDeviceMemoryProperties m_physical_device_current_memory_properties;
	VkSurfaceFormatKHR m_swapchain_format;

#pragma region Resources
	CommandListRing m_command_list;
	MemoryRingPool m_memory_pool;
#pragma endregion

	StatisticsWrapper m_stats;
	UploadResourceManager m_upload_manager;
	DescriptorPoolManager m_manager_descriptors;
	shader_vertex_user_data_t m_user_data_for_vertex_shader;

	// TODO: after all tests remove it because you have already all compiled geometries in m_compiled_geometries field
	geometry_handle_t g_current_compiled_geometry;
};

#endif