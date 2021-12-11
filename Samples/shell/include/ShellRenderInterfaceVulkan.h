#ifndef RMLUI_SHELL_SHELLRENDERINTERFACEVULKAN_H
#define RMLUI_SHELL_SHELLRENDERINTERFACEVULKAN_H

#include <RmlUi/Core/RenderInterface.h>

// TODO: [From wh1t3lord] I suggest to rename ShellOpenGL to ShellRender, because if we have more than one Renderer it's better to keep codebase
// simple as much as possible, that means if we don't want to grow headers and other code we should use general file that keeps all needs for our
// system in this case I mean to not create other ShellVulkan, ShellDX12, ShellDX11, ShellXXX...
// I mean you can put all headers into one file and have a general name like ShellRender.h it means this header contains everything needed information
// for writing own ShellRenderInterfaceXXX
#include "ShellOpenGL.h"

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
#pragma endregion

class ShellRenderInterfaceVulkan : public Rml::RenderInterface, public ShellRenderInterfaceExtensions {
	enum class shader_type_t : int { kShaderType_Vertex, kShaderType_Pixel, kShaderType_Unknown = -1 };

	using shader_data_t = Rml::Vector<uint32_t>;

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

		void Initialize(VmaAllocator p_allocator, VkDevice p_device, uint32_t number_of_back_buffers, uint32_t memory_total_size) noexcept;
		void Shutdown(void) noexcept;

		bool AllocConstantBuffer(uint32_t size, void** p_data, VkDescriptorBufferInfo* p_out) noexcept;
		VkDescriptorBufferInfo AllocConstantBuffer(uint32_t size, void* p_data) noexcept;
		bool AllocVertexBuffer(uint32_t number_of_elements, uint32_t stride_in_bytes, void** p_data, VkDescriptorBufferInfo* p_out) noexcept;
		bool AllocIndexBuffer(uint32_t number_of_elements, uint32_t stride_in_bytes, void** p_data, VkDescriptorBufferInfo* p_out) noexcept;
		void OnBeginFrame(void) noexcept;
		void SetDescriptorSet(uint32_t binding_index, uint32_t size, VkDescriptorType descriptor_type, VkDescriptorSet p_set) noexcept;
		void SetDescriptorSet(uint32_t binding_index, VkSampler p_sampler, VkImageLayout layout, VkImageView p_view, VkDescriptorType descriptor_type,
			VkDescriptorSet p_set) noexcept;

	private:
		uint32_t m_memory_total_size;
		char* m_p_data;
		VkBuffer m_p_buffer;
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
	static uint32_t ConvertCountToMegabytes(uint32_t value_shows_megabytes) noexcept { return value_shows_megabytes * 1024 * 1024; }

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
	VkExtent2D CreateValidSwapchainExtent(void) noexcept;
	VkSurfaceTransformFlagBitsKHR CreatePretransformSwapchain(void) noexcept;
	VkCompositeAlphaFlagBitsKHR ChooseSwapchainCompositeAlpha(void) noexcept;
	VkPresentModeKHR GetPresentMode(VkPresentModeKHR type = VkPresentModeKHR::VK_PRESENT_MODE_FIFO_KHR) noexcept;
	VkSurfaceCapabilitiesKHR GetSurfaceCapabilities(void) noexcept;

	Rml::UnorderedMap<shader_type_t, shader_data_t> LoadShaders(void) noexcept;
	shader_data_t LoadShader(const Rml::String& relative_path_from_samples_folder_with_file_and_fileformat) noexcept;
	void CreateShaders(const Rml::UnorderedMap<shader_type_t, shader_data_t>& storage) noexcept;
	void CreateDescriptorSetLayout(const Rml::UnorderedMap<shader_type_t, shader_data_t>& storage) noexcept;
	Rml::Vector<VkDescriptorSetLayoutBinding> CreateDescriptorSetLayoutBindings(const shader_data_t& data) noexcept;
	void CreatePipelineLayout(void) noexcept;
	void CreateDescriptorSets(void) noexcept;
	void CreatePipeline(void) noexcept;
	void CreateRenderPass(void) noexcept;

	void CreateSwapchainFrameBuffers(void) noexcept;

	// This method is called in Views, so don't call it manually
	void CreateSwapchainImages(void) noexcept;
	void CreateSwapchainImageViews(void) noexcept;

	void CreateResourcesDependentOnSize(void) noexcept;

	void DestroyResourcesDependentOnSize(void) noexcept;
	void DestroySwapchainImageViews(void) noexcept;
	void DestroySwapchainFrameBuffers(void) noexcept;
	void DestroyRenderPass(void) noexcept;
	void DestroyPipeline(void) noexcept;
	void DestroyDescriptorSets(void) noexcept;
	void DestroyPipelineLayout(void) noexcept;

	void Wait(void) noexcept;
	void Submit(void) noexcept;
	void Present(void) noexcept;

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

	VkInstance m_p_instance;
	VkDevice m_p_device;
	VkPhysicalDevice m_p_physical_device_current;
	VkSurfaceKHR m_p_surface;
	VkSwapchainKHR m_p_swapchain;
	VmaAllocator m_p_allocator;

#pragma region Resources
	VkDescriptorSetLayout m_p_descriptor_set_layout;
	VkPipelineLayout m_p_pipeline_layout;
	VkPipeline m_p_pipeline;
	VkDescriptorSet m_p_descriptor_set;
	VkRenderPass m_p_render_pass;
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
	Rml::UnorderedMap<shader_type_t, VkShaderModule> m_shaders;
#pragma endregion

	VkPhysicalDeviceMemoryProperties m_physical_device_current_memory_properties;
	VkSurfaceFormatKHR m_swapchain_format;

#pragma region Resources
	CommandListRing m_command_list;
	MemoryRingPool m_memory_pool;
#pragma endregion
};

#endif