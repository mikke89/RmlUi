#include "ShellRenderInterfaceVulkan.h"
#include "ShellFileInterface.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

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

ShellRenderInterfaceVulkan::ShellRenderInterfaceVulkan() :
	m_is_transform_enabled(false), m_width(0), m_height(0), m_queue_index_present(0), m_queue_index_graphics(0), m_queue_index_compute(0),
	m_semaphore_index(0), m_semaphore_index_previous(0), m_p_instance(nullptr), m_p_device(nullptr), m_p_physical_device_current(nullptr),
	m_p_surface(nullptr), m_p_swapchain(nullptr), m_p_window_handle(nullptr), m_p_queue_present(nullptr), m_p_queue_graphics(nullptr),
	m_p_queue_compute(nullptr), m_p_descriptor_set_layout(nullptr), m_p_pipeline_layout(nullptr), m_p_render_pass(nullptr), m_p_allocator{},
	m_p_current_command_buffer(nullptr)
{}

ShellRenderInterfaceVulkan::~ShellRenderInterfaceVulkan(void) {}

void ShellRenderInterfaceVulkan::RenderGeometry(
	Rml::Vertex* vertices, int num_vertices, int* indices, int num_indices, Rml::TextureHandle texture, const Rml::Vector2f& translation)
{
}

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
	return true;
}

bool ShellRenderInterfaceVulkan::GenerateTexture(Rml::TextureHandle& texture_handle, const Rml::byte* source, const Rml::Vector2i& source_dimensions)
{
	return true;
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

void ShellRenderInterfaceVulkan::PrepareRenderBuffer(void)
{
	this->m_memory_pool.OnBeginFrame();
	this->m_command_list.OnBeginFrame();
	this->m_p_current_command_buffer = this->m_command_list.GetNewCommandList();
	this->Wait();

	VkCommandBufferBeginInfo info = {};

	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	info.pInheritanceInfo = nullptr;
	info.pNext = nullptr;
	info.flags = VkCommandBufferUsageFlagBits::VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	auto status = vkBeginCommandBuffer(this->m_p_current_command_buffer, &info);

	VK_ASSERT(status == VkResult::VK_SUCCESS, "failed to vkBeginCommandBuffer");

	VkClearValue for_filling_back_buffer_color;

	for_filling_back_buffer_color.color = {1.0f, 1.0f, 0.0f, 1.0f};

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
	this->m_memory_pool.Initialize(
		this->m_p_allocator, this->m_p_device, kSwapchainBackBufferCount, ShellRenderInterfaceVulkan::ConvertCountToMegabytes(32));

	auto storage = this->LoadShaders();

	this->CreateShaders(storage);
	this->CreateDescriptorSetLayout(storage);
	this->CreatePipelineLayout();
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

	vkDestroyDescriptorSetLayout(this->m_p_device, this->m_p_descriptor_set_layout, nullptr);
	vkDestroyPipelineLayout(this->m_p_device, this->m_p_pipeline_layout, nullptr);

	for (const auto& pair_shader_type_shader_module : this->m_shaders)
	{
		vkDestroyShaderModule(this->m_p_device, pair_shader_type_shader_module.second, nullptr);
	}
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

Rml::UnorderedMap<ShellRenderInterfaceVulkan::shader_type_t, ShellRenderInterfaceVulkan::shader_data_t> ShellRenderInterfaceVulkan::LoadShaders(
	void) noexcept
{
	auto frag = this->LoadShader("assets/rmlui_frag.spv");
	auto vertex = this->LoadShader("assets/rmlui_vert.spv");

	Rml::UnorderedMap<shader_type_t, shader_data_t> result;

	result[shader_type_t::kShaderType_Vertex] = vertex;
	result[shader_type_t::kShaderType_Pixel] = frag;

	return result;
}

ShellRenderInterfaceVulkan::shader_data_t ShellRenderInterfaceVulkan::LoadShader(
	const Rml::String& relative_path_from_samples_folder_with_file_and_fileformat) noexcept
{
	VK_ASSERT(Rml::GetFileInterface(), "[Vulkan] you must initialize FileInterface before calling this method");

	if (relative_path_from_samples_folder_with_file_and_fileformat.empty())
	{
		VK_ASSERT(false, "[Vulkan] you can't pass an empty string for loading shader");
		return shader_data_t();
	}

	auto* p_file_interface = Rml::GetFileInterface();

	auto p_file = Rml::GetFileInterface()->Open(relative_path_from_samples_folder_with_file_and_fileformat);

	VK_ASSERT(p_file, "[Vulkan] Rml::FileHandle is invalid! %s", relative_path_from_samples_folder_with_file_and_fileformat.c_str());

	auto file_size = p_file_interface->Length(p_file);

	VK_ASSERT(file_size != -1L, "[Vulkan] can't get length of file: %s", relative_path_from_samples_folder_with_file_and_fileformat.c_str());

	shader_data_t buffer(file_size);

	p_file_interface->Read(buffer.data(), buffer.size(), p_file);

	p_file_interface->Close(p_file);

	return buffer;
}

void ShellRenderInterfaceVulkan::CreateShaders(const Rml::UnorderedMap<shader_type_t, shader_data_t>& storage) noexcept
{
	VK_ASSERT(storage.empty() == false, "[Vulkan] you must load shaders before creating resources");
	VK_ASSERT(this->m_p_device, "[Vulkan] you must initialize VkDevice before calling this method");

	VkShaderModuleCreateInfo info = {};

	for (const auto& pair_shader_type_shader_data : storage)
	{
		VkShaderModule p_module = nullptr;

		info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		info.pCode = pair_shader_type_shader_data.second.data();
		info.codeSize = pair_shader_type_shader_data.second.size();

		VkResult status = vkCreateShaderModule(this->m_p_device, &info, nullptr, &p_module);

		VK_ASSERT(status == VK_SUCCESS, "[Vulkan] failed to vkCreateShaderModule");

		this->m_shaders[pair_shader_type_shader_data.first] = p_module;
	}
}

void ShellRenderInterfaceVulkan::CreateDescriptorSetLayout(const Rml::UnorderedMap<shader_type_t, shader_data_t>& storage) noexcept
{
	VK_ASSERT(storage.empty() == false, "[Vulkan] you must load shaders before creating resources");
	VK_ASSERT(this->m_p_device, "[Vulkan] you must initialize VkDevice before calling this method");

	Rml::Vector<VkDescriptorSetLayoutBinding> all_bindings;

	for (const auto& pair_shader_type_shader_data : storage)
	{
		const auto& current_bindings = this->CreateDescriptorSetLayoutBindings(pair_shader_type_shader_data.second);

		all_bindings.insert(all_bindings.end(), all_bindings.begin(), all_bindings.end());
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

Rml::Vector<VkDescriptorSetLayoutBinding> ShellRenderInterfaceVulkan::CreateDescriptorSetLayoutBindings(const shader_data_t& data) noexcept
{
	Rml::Vector<VkDescriptorSetLayoutBinding> result;

	VK_ASSERT(data.empty() == false, "[Vulkan] can't be empty data of shader");

	SpvReflectShaderModule spv_module = {};

	SpvReflectResult status = spvReflectCreateShaderModule(data.size() * sizeof(char), data.data(), &spv_module);

	VK_ASSERT(status == SPV_REFLECT_RESULT_SUCCESS, "[Vulkan] SPIRV-Reflect failed to spvReflectCreateShaderModule");

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

void ShellRenderInterfaceVulkan::CreateDescriptorSets(void) noexcept {}

void ShellRenderInterfaceVulkan::CreatePipeline(void) noexcept {}

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

	this->CreateRenderPass();
	this->CreateSwapchainFrameBuffers();
}

void ShellRenderInterfaceVulkan::DestroyResourcesDependentOnSize(void) noexcept
{
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

void ShellRenderInterfaceVulkan::DestroyPipeline(void) noexcept {}

void ShellRenderInterfaceVulkan::DestroyDescriptorSets(void) noexcept {}

void ShellRenderInterfaceVulkan::DestroyPipelineLayout(void) noexcept {}

void ShellRenderInterfaceVulkan::CreateRenderPass(void) noexcept
{
	VK_ASSERT(this->m_p_device, "you must have a valid VkDevice here");

	VkAttachmentDescription attachments[1];

	attachments[0].format = this->m_swapchain_format.format;
	attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
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

ShellRenderInterfaceVulkan::MemoryRingPool::MemoryRingPool(void) :
	m_memory_total_size{}, m_p_data{}, m_p_buffer{}, m_p_buffer_alloc{}, m_p_device{}, m_p_vk_allocator{}
{}

ShellRenderInterfaceVulkan::MemoryRingPool::~MemoryRingPool(void) {}

void ShellRenderInterfaceVulkan::MemoryRingPool::Initialize(
	VmaAllocator p_allocator, VkDevice p_device, uint32_t number_of_back_buffers, uint32_t memory_total_size) noexcept
{
	VK_ASSERT(p_device, "you must pass a valid VkDevice");
	VK_ASSERT(p_allocator, "you must pass a valid VmaAllocator");

	this->m_p_device = p_device;
	this->m_p_vk_allocator = p_allocator;

	this->m_memory_total_size = ShellRenderInterfaceVulkan::AlignUp(memory_total_size, 256u);

	this->m_allocator.Initialize(number_of_back_buffers, this->m_memory_total_size);

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

#ifdef RMLUI_DEBUG
	Shell::Log("[Vulkan][Debug] allocated memory for pool: %d Mbs", ShellRenderInterfaceVulkan::TranslateBytesToMegaBytes(info_stats.size));
#endif

	status = vmaMapMemory(this->m_p_vk_allocator, this->m_p_buffer_alloc, (void**)&this->m_p_data);

	VK_ASSERT(status == VkResult::VK_SUCCESS, "failed to vmaMapMemory");
}

void ShellRenderInterfaceVulkan::MemoryRingPool::Shutdown(void) noexcept
{
	VK_ASSERT(this->m_p_vk_allocator, "you must have a valid VmaAllocator");
	VK_ASSERT(this->m_p_buffer, "you must allocate VkBuffer for deleting");
	VK_ASSERT(this->m_p_buffer_alloc, "you must allocate VmaAllocation for deleting");

	vmaUnmapMemory(this->m_p_vk_allocator, this->m_p_buffer_alloc);
	vmaDestroyBuffer(this->m_p_vk_allocator, this->m_p_buffer, this->m_p_buffer_alloc);

	this->m_allocator.Shutdown();
}

bool ShellRenderInterfaceVulkan::MemoryRingPool::AllocConstantBuffer(uint32_t size, void** p_data, VkDescriptorBufferInfo* p_out) noexcept
{
	VK_ASSERT(p_out, "you must pass a valid pointer");
	VK_ASSERT(this->m_p_buffer, "you must have a valid VkBuffer");

	size = ShellRenderInterfaceVulkan::AlignUp(size, 256u);

	uint32_t offset_memory{};

	if (this->m_allocator.Alloc(size, &offset_memory) == false)
	{
		VK_ASSERT(false, "overflow, rebuild your buffer with new size that bigger current: %d", this->m_memory_total_size);
	}

	*p_data = (void*)(this->m_p_data + offset_memory);

	p_out->buffer = this->m_p_buffer;
	p_out->offset = offset_memory;
	p_out->range = size;

	return true;
}

VkDescriptorBufferInfo ShellRenderInterfaceVulkan::MemoryRingPool::AllocConstantBuffer(uint32_t size, void* p_data) noexcept
{
	void* p_buffer{};

	VkDescriptorBufferInfo result = {};

	if (this->AllocConstantBuffer(size, &p_buffer, &result))
	{
		memcpy(p_buffer, p_data, size);
	}

	return result;
}

bool ShellRenderInterfaceVulkan::MemoryRingPool::AllocVertexBuffer(
	uint32_t number_of_elements, uint32_t stride_in_bytes, void** p_data, VkDescriptorBufferInfo* p_out) noexcept
{
	return this->AllocConstantBuffer(number_of_elements * stride_in_bytes, p_data, p_out);
}

bool ShellRenderInterfaceVulkan::MemoryRingPool::AllocIndexBuffer(
	uint32_t number_of_elements, uint32_t stride_in_bytes, void** p_data, VkDescriptorBufferInfo* p_out) noexcept
{
	return this->AllocConstantBuffer(number_of_elements * stride_in_bytes, p_data, p_out);
}

void ShellRenderInterfaceVulkan::MemoryRingPool::OnBeginFrame(void) noexcept
{
	this->m_allocator.OnBeginFrame();
}

void ShellRenderInterfaceVulkan::MemoryRingPool::SetDescriptorSet(
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

void ShellRenderInterfaceVulkan::MemoryRingPool::SetDescriptorSet(uint32_t binding_index, VkSampler p_sampler, VkImageLayout layout,
	VkImageView p_view, VkDescriptorType descriptor_type, VkDescriptorSet p_set) noexcept
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
