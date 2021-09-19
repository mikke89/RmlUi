#ifndef RMLUI_SHELL_SHELLRENDERINTERFACEVULKAN_H
#define RMLUI_SHELL_SHELLRENDERINTERFACEVULKAN_H

#include <RmlUi/Core/RenderInterface.h>

// TODO: [From diamondhat] I suggest to rename ShellOpenGL to ShellRender, because if we have more than one Renderer it's better to keep codebase
// simple as much as possible, that means if we don't want to grow headers and other code we should use general file that keeps all needs for our
// system in this case I mean to not create other ShellVulkan, ShellDX12, ShellDX11, ShellXXX...
#include "ShellOpenGL.h"

/**
 * Low level Vulkan render interface for RmlUi
 *
 * My aim is to create compact, but easy to use class
 * I understand that it doesn't good architectural choice to keep all things in one class
 * But I follow to RMLUI design and for implementing one GAPI backend it just needs one class
 * For user looks cool, but for programmer...
 *
 * It's better to try operate with very clean 'one-class' architecture rather than create own library for Vulkan
 * With many different classes, with not trivial signatures and etc
 * And we should document that library so it's just a headache for all of us
 *
 * @author diamondhat
 */

class ShellRenderInterfaceVulkan : public Rml::RenderInterface, public ShellRenderInterfaceExtensions 
{
	enum class shader_type_t : int
	{
		kShaderType_Vertex,
		kShaderType_Pixel,
		kShaderType_Unknown = -1
	};

	using shader_data_t = Rml::Vector<char>;
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

	void Destroy_Instance(void) noexcept;
	void Destroy_Device() noexcept;
	void Destroy_Swapchain(void) noexcept;
	void Destroy_Surface(void) noexcept;
	void Destroy_SyncPrimitives(void) noexcept;
	void Destroy_Resources(void) noexcept;

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
	void CreateLayouts(void) noexcept;
	void CreateSwapchainFrameBuffers(void) noexcept;
	void CreateRenderPass(void) noexcept;

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

	#pragma region Resources
	VkDescriptorSetLayout m_p_descriptor_set_layout;
	VkPipelineLayout m_p_pipeline_layout;
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

	Rml::Vector<VkPhysicalDevice> m_physical_devices;
	Rml::Vector<VkLayerProperties> m_instance_layer_properties;
	Rml::Vector<VkExtensionProperties> m_instance_extension_properties;
	Rml::Vector<VkExtensionProperties> m_device_extension_properties;
	Rml::Vector<const char*> m_instance_layer_names;
	Rml::Vector<const char*> m_instance_extension_names;
	Rml::Vector<const char*> m_device_extension_names;
	Rml::Vector<VkFence> m_executed_fences;
	Rml::Vector<VkSemaphore> m_semaphores_image_available;
	Rml::Vector<VkSemaphore> m_semaphores_finished_render;

	#pragma region Resources
	Rml::UnorderedMap<shader_type_t, VkShaderModule> m_shaders;
	#pragma endregion

	VkPhysicalDeviceMemoryProperties m_physical_device_current_memory_properties;
	VkSurfaceFormatKHR m_swapchain_format;
};

#endif