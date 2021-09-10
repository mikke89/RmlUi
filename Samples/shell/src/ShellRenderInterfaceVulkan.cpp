#include "ShellRenderInterfaceVulkan.h"

#define VK_ASSERT(statement, msg, ...)         \
	if (statement == false)                    \
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

ShellRenderInterfaceVulkan::ShellRenderInterfaceVulkan()
{
	this->Initialize();
}

ShellRenderInterfaceVulkan::~ShellRenderInterfaceVulkan(void)
{
	this->Shutdown();
}

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

void ShellRenderInterfaceVulkan::SetViewport(int width, int height) {}

bool ShellRenderInterfaceVulkan::AttachToNative(void* nativeWindow)
{
	return false;
}

void ShellRenderInterfaceVulkan::DetachFromNative(void) {}

void ShellRenderInterfaceVulkan::PrepareRenderBuffer(void) {}

void ShellRenderInterfaceVulkan::PresentRenderBuffer(void) {}

void ShellRenderInterfaceVulkan::Initialize(void) noexcept
{
	this->Initialize_Instance();
	this->Initialize_PhysicalDevice();
	this->Initialize_Surface();
	this->Initialize_QueueIndecies();
	this->Initialize_Device();
	this->Initialize_Swapchain();
}

void ShellRenderInterfaceVulkan::Shutdown(void) noexcept
{
	this->Destroy_Swapchain();
	this->Destroy_Surface();
	this->Destroy_Device();
	this->Destroy_ReportDebugCallback();
	this->Destroy_Instance();
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

void ShellRenderInterfaceVulkan::Initialize_Swapchain(void) noexcept {}

void ShellRenderInterfaceVulkan::Initialize_Surface(void) noexcept
{
	VK_ASSERT(this->m_p_instance, "you must initialize your VkInstance");

#if defined(RMLUI_PLATFORM_WIN32)
	VkWin32SurfaceCreateInfoKHR info = {};

	VkResult status = vkCreateWin32SurfaceKHR(this->m_p_instance, )
#else
#error this platform doesn't support Vulkan!!!
#endif
}

void ShellRenderInterfaceVulkan::Initialize_QueueIndecies(void) noexcept
{
	VK_ASSERT(this->m_p_physical_device_current, "you must initialize your physical device");

	uint32_t queue_family_count = 0;

	vkGetPhysicalDeviceQueueFamilyProperties(this->m_p_physical_device_current, &queue_family_count, nullptr);

	VK_ASSERT(queue_family_count >= 1, "failed to vkGetPhysicalDeviceQueueFamilyProperties (getting count)");

	Rml::Vector<VkQueueFamilyProperties> queue_props;

	queue_props.resize(queue_family_count);

	vkGetPhysicalDeviceQueueFamilyProperties(this->m_p_physical_device_current, &queue_family_count, queue_props.data());

	VK_ASSERT(queue_family_count >= 1, "failed to vkGetPhysicalDeviceQueueFamilyProperties (filling vector of VkQueueFamilyProperties)");
}

void ShellRenderInterfaceVulkan::Destroy_Instance(void) noexcept
{
	vkDestroyInstance(this->m_p_instance, nullptr);
}

void ShellRenderInterfaceVulkan::Destroy_Device() noexcept {}

void ShellRenderInterfaceVulkan::Destroy_Swapchain(void) noexcept {}

void ShellRenderInterfaceVulkan::Destroy_Surface(void) noexcept {}

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
