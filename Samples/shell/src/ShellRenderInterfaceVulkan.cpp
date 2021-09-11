#include "ShellRenderInterfaceVulkan.h"

#define VK_ASSERT(statement, msg, ...)         \
	if (!!(statement) == false)                \
	{                                          \
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

#pragma region System Constants for Vulkan API
constexpr uint32_t kSwapchainBackBufferCount = 3;
#pragma endregion

ShellRenderInterfaceVulkan::ShellRenderInterfaceVulkan() : m_is_transform_enabled(false), m_width(0), m_height(0), m_queue_index_present(0), m_queue_index_graphics(0), m_queue_index_compute(0), m_p_instance(nullptr), m_p_device(nullptr), m_p_physical_device_current(nullptr), m_p_surface(nullptr), m_p_swapchain(nullptr), m_p_window_handle(nullptr), m_p_queue_present(nullptr), m_p_queue_graphics(nullptr), m_p_queue_compute(nullptr)  {}

ShellRenderInterfaceVulkan::~ShellRenderInterfaceVulkan(void) {}

void ShellRenderInterfaceVulkan::RenderGeometry(
	Rml::Vertex* vertices, int num_vertices, int* indices, int num_indices, Rml::TextureHandle texture, const Rml::Vector2f& translation)
{}

Rml::CompiledGeometryHandle ShellRenderInterfaceVulkan::CompileGeometry(
	Rml::Vertex* vertices, int num_vertices, int* indices, int num_indices, Rml::TextureHandle texture)
{
	return Rml::CompiledGeometryHandle();
}

void ShellRenderInterfaceVulkan::RenderCompiledGeometry(Rml::CompiledGeometryHandle geometry, const Rml::Vector2f& translation) {}

void ShellRenderInterfaceVulkan::ReleaseCompiledGeometry(Rml::CompiledGeometryHandle geometry) {}

void ShellRenderInterfaceVulkan::EnableScissorRegion(bool enable) {}

void ShellRenderInterfaceVulkan::SetScissorRegion(int x, int y, int width, int height) {}

bool ShellRenderInterfaceVulkan::LoadTexture(Rml::TextureHandle& texture_handle, Rml::Vector2i& texture_dimensions, const Rml::String& source)
{
	return false;
}

bool ShellRenderInterfaceVulkan::GenerateTexture(Rml::TextureHandle& texture_handle, const Rml::byte* source, const Rml::Vector2i& source_dimensions)
{
	return false;
}

void ShellRenderInterfaceVulkan::ReleaseTexture(Rml::TextureHandle texture_handle) {}

void ShellRenderInterfaceVulkan::SetTransform(const Rml::Matrix4f* transform) {}

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

void ShellRenderInterfaceVulkan::PrepareRenderBuffer(void) {}

void ShellRenderInterfaceVulkan::PresentRenderBuffer(void) {}

void ShellRenderInterfaceVulkan::Initialize(void) noexcept
{
	this->Initialize_Instance();
	this->Initialize_PhysicalDevice();
	this->Initialize_Surface();
	this->Initialize_QueueIndecies();
	this->Initialize_Device();
	this->Initialize_Queues();
	this->Initialize_SyncPrimitives();
}

void ShellRenderInterfaceVulkan::Shutdown(void) noexcept
{
	this->Destroy_SyncPrimitives();
	this->Destroy_Swapchain();
	this->Destroy_Surface();
	this->Destroy_Device();
	this->Destroy_ReportDebugCallback();
	this->Destroy_Instance();
}

void ShellRenderInterfaceVulkan::OnResize(int width, int height) noexcept
{
	this->m_width = width;
	this->m_height = height;

	if (this->m_p_swapchain) {
		this->Destroy_Swapchain();
	}

	this->Initialize_Swapchain();
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
	else
	{
		Shell::Log("Picked discrete GPU!");
	}
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
		uint32_t p_indecies[2] = {
			queue_family_index_graphics,
			queue_family_index_present
		};

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
		// Rml::Log::Message(Rml::Log::LT_INFO, "CPU validation is enabled");

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

	VkResult status = vkEnumeratePhysicalDevices(this->m_p_instance, &gpu_count, nullptr);

	VK_ASSERT(status == VK_SUCCESS, "failed to vkEnumeratePhysicalDevices (getting count)");

	this->m_physical_devices.resize(gpu_count);

	status = vkEnumeratePhysicalDevices(this->m_p_instance, &gpu_count, this->m_physical_devices.data());

	VK_ASSERT(status == VK_SUCCESS, "failed to vkEnumeratePhysicalDevices (filling the vector of VkPhysicalDevice)");

	VK_ASSERT(this->m_physical_devices.empty() == false, "you must have one videocard at least!");
}

bool ShellRenderInterfaceVulkan::ChoosePhysicalDevice(VkPhysicalDeviceType device_type) noexcept
{
	VK_ASSERT(this->m_physical_devices.empty() == false,
		"you must have one videocard at least or early calling of this method, try call this after CollectPhysicalDevices");

	VkPhysicalDeviceProperties props = {};

	for (const auto& p_device : this->m_physical_devices)
	{
		vkGetPhysicalDeviceProperties(p_device, &props);

		if (props.deviceType == device_type)
		{
			this->m_p_physical_device_current = p_device;
			return true;
		}
	}

	return false;
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
		result = caps.currentExtent;
	}

	return result;
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
	VK_ASSERT(this->m_p_device, "you must initialize your device, before calling this method");
	VK_ASSERT(this->m_p_physical_device_current, "you must initialize your physical device, before calling this method");
	VK_ASSERT(this->m_p_surface, "you must initialize your surface, before calling this method");

	VkPresentModeKHR result = required;

	uint32_t present_modes_count = 0;

	VkResult status = vkGetPhysicalDeviceSurfacePresentModesKHR(this->m_p_physical_device_current, this->m_p_surface, &present_modes_count, nullptr);

	VK_ASSERT(status == VK_SUCCESS, "failed to vkGetPhysicalDeviceSurfacePresentModesKHR (getting count)");

	Rml::Vector<VkPresentModeKHR> present_modes(present_modes_count);

	status =
		vkGetPhysicalDeviceSurfacePresentModesKHR(this->m_p_physical_device_current, this->m_p_surface, &present_modes_count, present_modes.data());

	VK_ASSERT(status == VK_SUCCESS, "failed to vkGetPhysicalDeviceSurfacePresentModesKHR (filling vector of VkPresentModeKHR)");

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
	VK_ASSERT(this->m_p_device, "you must initialize your device, before calling this method");
	VK_ASSERT(this->m_p_physical_device_current, "you must initialize your physical device, before calling this method");
	VK_ASSERT(this->m_p_surface, "you must initialize your surface, before calling this method");

	VkSurfaceCapabilitiesKHR result;

	VkResult status = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(this->m_p_physical_device_current, this->m_p_surface, &result);

	VK_ASSERT(status == VK_SUCCESS, "failed to vkGetPhysicalDeviceSurfaceCapabilitiesKHR");

	return result;
}
