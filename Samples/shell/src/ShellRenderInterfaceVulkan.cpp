#include "ShellRenderInterfaceVulkan.h"

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
	this->Initialize_Device();
	this->Initialize_Surface();
	this->Initialize_Swapchain();
}

void ShellRenderInterfaceVulkan::Shutdown(void) noexcept
{
	this->Destroy_Swapchain();
	this->Destroy_Surface();
	this->Destroy_PhysicalDevice();
	this->Destroy_Device();
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
}

void ShellRenderInterfaceVulkan::Initialize_Device(void) noexcept {}

void ShellRenderInterfaceVulkan::Initialize_PhysicalDevice(void) noexcept {}

void ShellRenderInterfaceVulkan::Initialize_Swapchain(void) noexcept {}

void ShellRenderInterfaceVulkan::Initialize_Surface(void) noexcept {}

void ShellRenderInterfaceVulkan::Destroy_Instance(void) noexcept {}

void ShellRenderInterfaceVulkan::Destroy_Device() noexcept {}

void ShellRenderInterfaceVulkan::Destroy_PhysicalDevice(void) noexcept {}

void ShellRenderInterfaceVulkan::Destroy_Swapchain(void) noexcept {}

void ShellRenderInterfaceVulkan::Destroy_Surface(void) noexcept {}

void ShellRenderInterfaceVulkan::QueryInstanceLayers(void) noexcept {}

void ShellRenderInterfaceVulkan::QueryInstanceExtensions(void) noexcept {}

void ShellRenderInterfaceVulkan::AddLayerToInstance(const char* p_instance_layer_name) noexcept 
{
	if (p_instance_layer_name == nullptr)
		return;

	if (this->IsLayerPresent(this->m_instance_layer_properties, p_instance_layer_name)) 
	{
		return;
	}

	RMLUI_ASSERT(false && "layer doesn't exist in your Vulkan");
}

void ShellRenderInterfaceVulkan::AddExtensionToInstance(const char* p_instance_extension_name) noexcept {}

void ShellRenderInterfaceVulkan::CreatePropertiesFor_Instance(void) noexcept
{
	this->QueryInstanceLayers();
	this->QueryInstanceExtensions();
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

void ShellRenderInterfaceVulkan::CreatePropertiesFor_Device(void) noexcept {}

uint32_t ShellRenderInterfaceVulkan::GetUserAPIVersion(void) const noexcept
{
	uint32_t result = 0;

	VkResult status = vkEnumerateInstanceVersion(&result);

	RMLUI_ASSERT(status == VK_SUCCESS && "failed to vkEnumerateInstanceVersion, See Status");

	return result;
}

uint32_t ShellRenderInterfaceVulkan::GetRequiredVersionAndValidateMachine(void) noexcept
{
	constexpr uint32_t kRequiredVersion = VK_API_VERSION_1_0;
	const uint32_t user_version = this->GetUserAPIVersion();

	RMLUI_ASSERT(kRequiredVersion <= user_version && "Your machine doesn't support Vulkan");

	return kRequiredVersion;
}
