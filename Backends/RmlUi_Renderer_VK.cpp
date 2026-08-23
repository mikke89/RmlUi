#include "RmlUi_Renderer_VK.h"
#include "RmlUi_Vulkan/ShadersCompiledSPV.h"
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/DecorationTypes.h>
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Core/Log.h>
#include <RmlUi/Core/Math.h>
#include <RmlUi/Core/Mesh.h>
#include <RmlUi/Core/MeshUtilities.h>
#include <RmlUi/Core/Platform.h>
#include <RmlUi/Core/Profiling.h>
#include <RmlUi/Core/SystemInterface.h>
#include <algorithm>
#include <math.h>
#include <string.h>

// The implementation translation unit for the bundled glad Vulkan loader and VulkanMemoryAllocator (both header-only
// style). The first include above pulled in the declarations; these re-includes expand only the implementation blocks
// (they live outside the include guards of vulkan.h/vk_mem_alloc.h).
#if defined _MSC_VER
	#pragma warning(push, 0)
#elif defined __clang__
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wall"
	#pragma clang diagnostic ignored "-Wextra"
	#pragma clang diagnostic ignored "-Wnullability-extension"
	#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
	#pragma clang diagnostic ignored "-Wnullability-completeness"
#elif defined __GNUC__
	#pragma GCC system_header
#endif
#define GLAD_VULKAN_IMPLEMENTATION
#include "RmlUi_Vulkan/vulkan.h"
#define VMA_IMPLEMENTATION
#include "RmlUi_Vulkan/vk_mem_alloc.h"
#if defined _MSC_VER
	#pragma warning(pop)
#elif defined __clang__
	#pragma clang diagnostic pop
#endif

// AlignUp(314, 256) = 512
template <typename T>
static T AlignUp(T val, T alignment)
{
	return (val + alignment - (T)1) & ~(alignment - (T)1);
}

#ifdef RMLUI_VK_DEBUG
static VkValidationFeaturesEXT debug_validation_features_ext = {};
static VkValidationFeatureEnableEXT debug_validation_features_ext_requested[] = {
	VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
	VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT,
	VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
};

inline Rml::String FormatByteSize(VkDeviceSize size) noexcept
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

/// @brief in bytes see shader_vert_main.vert -> layout(set = 0, binding = 0) uniform UserData (mat4 + vec2)
static constexpr uint32_t kAllocationSize_ConstantBuffer_Vertex_Main = 72;

/// @brief in bytes see shader_vert_blur.vert/shader_frag_blur.frag -> SharedConstantBuffer
/// (64 transform + 8 translate + 8 texelOffset + 16 weights + 8 texCoordMin + 8 texCoordMax)
static constexpr uint32_t kAllocationSize_ConstantBuffer_Vertex_Blur = 112;

/// @brief in bytes see shader_frag_gradient.frag -> SharedConstantBuffer (416 bytes, zero padding)
static constexpr uint32_t kAllocationSize_ConstantBuffer_Pixel_Gradient = 416;

/// @brief in bytes see shader_frag_creation.frag -> SharedConstantBuffer
static constexpr uint32_t kAllocationSize_ConstantBuffer_Pixel_Creation = 84;

/// @brief in bytes see shader_frag_color_matrix.frag -> ConstantBuffer (single mat4)
static constexpr uint32_t kAllocationSize_ConstantBuffer_Pixel_ColorMatrix = 64;

/// @brief in bytes see shader_frag_drop_shadow.frag -> DropShadowBuffer
static constexpr uint32_t kAllocationSize_ConstantBuffer_Pixel_DropShadow = 32;

/// @brief generally saying it is not universal approach but for keeping things not so much complex better to specify max amount of constantbuffer
/// that's enough to satisfy all shaders otherwise better to use only those which size is required for allocation
static constexpr uint32_t kAllocationSizeMax_ConstantBuffer = std::max({kAllocationSize_ConstantBuffer_Vertex_Main,
	kAllocationSize_ConstantBuffer_Vertex_Blur, kAllocationSize_ConstantBuffer_Pixel_Gradient, kAllocationSize_ConstantBuffer_Pixel_Creation,
	kAllocationSize_ConstantBuffer_Pixel_ColorMatrix, kAllocationSize_ConstantBuffer_Pixel_DropShadow});

#define MAX_NUM_STOPS 16
#define BLUR_SIZE 7
#define BLUR_NUM_WEIGHTS ((BLUR_SIZE + 1) / 2)

enum class ShaderGradientFunction { Linear, Radial, Conic, RepeatingLinear, RepeatingRadial, RepeatingConic }; // Must match shader definitions.

enum class FilterType { Invalid = 0, Passthrough, Blur, DropShadow, ColorMatrix, MaskImage };
struct CompiledFilter {
	FilterType type;

	// Passthrough
	float blend_factor;

	// Blur
	float sigma;

	// Drop shadow
	Rml::Vector2f offset;
	Rml::ColourbPremultiplied color;

	// ColorMatrix
	Rml::Matrix4f color_matrix;
};

enum class CompiledShaderType { Invalid = 0, Gradient, Creation };
struct CompiledShader {
	CompiledShaderType type;

	// Gradient
	ShaderGradientFunction gradient_function;
	Rml::Vector2f p;
	Rml::Vector2f v;
	Rml::Vector<float> stop_positions;
	Rml::Vector<Rml::Colourf> stop_colors;

	// Shader
	Rml::Vector2f dimensions;
};

// those programs that have postfix as _MSAA it means that it accepts render target with sample count >= 2 (thus it is called MSAA)
// (mirrors the DX12 renderer one to one)
enum class ProgramId : int {
	None,
	Color_Stencil_Always,
	Color_Stencil_Equal,
	Color_Stencil_Set,
	Color_Stencil_SetInverse,
	Color_Stencil_Intersect,
	Color_Stencil_Disabled,
	Texture_Stencil_Always,
	Texture_Stencil_Equal,
	Texture_Stencil_Disabled,
	Gradient,
	Creation,
	// this is for presenting our msaa render target texture for NO MSAA RT
	// if you do not correctly stuff DX12 validation will say about different
	// sample count like it is expected 1 (because no MSAA) but your RT target texture was created with
	// sample count = 2, so it is not a correct way of using it
	Passthrough,
	Passthrough_NoDepthStencil,
	Passthrough_Opacity,
	Passthrough_MSAA,
	Passthrough_MSAA_Equal,
	Passthrough_NoBlend,          // for MSAA RT
	Passthrough_NoBlendAndNoMSAA, // for RT that's not MSAA
	ColorMatrix,
	BlendMask,
	Blur,
	DropShadow,
	Count
};

static Rml::Colourf ConvertToColorf(Rml::ColourbPremultiplied c0)
{
	RMLUI_ZoneScopedN("Vulkan - ConvertToColorf");
	Rml::Colourf result;
	for (int i = 0; i < 4; i++)
		result[i] = (1.f / 255.f) * float(c0[i]);
	return result;
}

static void SetBlurWeights(Rml::Vector4f& p_weights, float sigma)
{
	float weights[BLUR_NUM_WEIGHTS];
	float normalization = 0.0f;
	for (int i = 0; i < BLUR_NUM_WEIGHTS; i++)
	{
		if (Rml::Math::Absolute(sigma) < 0.1f)
			weights[i] = float(i == 0);
		else
			weights[i] = Rml::Math::Exp(-float(i * i) / (2.0f * sigma * sigma)) / (Rml::Math::SquareRoot(2.f * Rml::Math::RMLUI_PI) * sigma);

		normalization += (i == 0 ? 1.f : 2.0f) * weights[i];
	}
	for (int i = 0; i < BLUR_NUM_WEIGHTS; i++)
		weights[i] /= normalization;

	p_weights.x = weights[0];
	p_weights.y = weights[1];
	p_weights.z = weights[2];
	p_weights.w = weights[3];
}

static void SetTexCoordLimits(Rml::Vector2f& p_tex_coord_min, Rml::Vector2f& p_tex_coord_max, Rml::Rectanglei rectangle,
	Rml::Vector2i framebuffer_size)
{
	// Offset by half-texel values so that texture lookups are clamped to fragment centers, thereby avoiding color
	// bleeding from neighboring texels due to bilinear interpolation.
	const Rml::Vector2f min = (Rml::Vector2f(rectangle.p0) + Rml::Vector2f(0.5f)) / Rml::Vector2f(framebuffer_size);
	const Rml::Vector2f max = (Rml::Vector2f(rectangle.p1) - Rml::Vector2f(0.5f)) / Rml::Vector2f(framebuffer_size);

	p_tex_coord_min.x = min.x;
	p_tex_coord_min.y = min.y;
	p_tex_coord_max.x = max.x;
	p_tex_coord_max.y = max.y;
}

static void SigmaToParameters(const float desired_sigma, int& out_pass_level, float& out_sigma)
{
	constexpr int max_num_passes = 10;
	static_assert(max_num_passes < 31, "");
	constexpr float max_single_pass_sigma = 3.0f;
	out_pass_level = Rml::Math::Clamp(Rml::Math::Log2(int(desired_sigma * (2.f / max_single_pass_sigma))), 0, max_num_passes);
	out_sigma = Rml::Math::Clamp(desired_sigma / float(1 << out_pass_level), 0.0f, max_single_pass_sigma);
}

// The header deliberately does not declare a program -> pipeline layout lookup table member, so member functions that
// need the pipeline layout (or the constant-buffer set index) of a program expand this macro to get a local lookup
// lambda (file-local functions cannot access the private layout members). The mapping itself is static knowledge:
// Color/Gradient/Creation -> transform, Texture -> transform_texture, all Passthrough -> texture, ColorMatrix/Blur/
// DropShadow -> texture_effect, BlendMask -> blend_mask.
#define RMLUI_VK_PROGRAM_PIPELINE_LAYOUT_LOOKUP(var_name)                                       \
	const auto var_name = [this](ProgramId id) -> VkPipelineLayout {                            \
		switch (id)                                                                             \
		{                                                                                       \
		case ProgramId::Color_Stencil_Always:                                                   \
		case ProgramId::Color_Stencil_Equal:                                                    \
		case ProgramId::Color_Stencil_Set:                                                      \
		case ProgramId::Color_Stencil_SetInverse:                                               \
		case ProgramId::Color_Stencil_Intersect:                                                \
		case ProgramId::Color_Stencil_Disabled:                                                 \
		case ProgramId::Gradient:                                                               \
		case ProgramId::Creation: return m_p_pipeline_layout_transform;                         \
		case ProgramId::Texture_Stencil_Always:                                                 \
		case ProgramId::Texture_Stencil_Equal:                                                  \
		case ProgramId::Texture_Stencil_Disabled: return m_p_pipeline_layout_transform_texture; \
		case ProgramId::ColorMatrix:                                                            \
		case ProgramId::Blur:                                                                   \
		case ProgramId::DropShadow: return m_p_pipeline_layout_texture_effect;                  \
		case ProgramId::BlendMask: return m_p_pipeline_layout_blend_mask;                       \
		default: return m_p_pipeline_layout_texture;                                            \
		}                                                                                       \
	}

namespace Gfx {
struct FramebufferData {
public:
	FramebufferData() :
		m_is_render_target{true}, m_width{}, m_height{}, m_id{-1}, m_p_texture{}, m_p_texture_depth_stencil{}, m_p_framebuffer{}, m_p_render_pass{}
	{}

	FramebufferData(const FramebufferData&) = delete;
	FramebufferData& operator=(const FramebufferData&) = delete;

	// overload because we don't need to copy information about how our object was allocated (from storage or on stack, see m_is_allocated_on_stack)
	FramebufferData(FramebufferData&& data) noexcept :
		m_is_render_target{std::exchange(data.m_is_render_target, true)}, m_width{std::exchange(data.m_width, 0)},
		m_height{std::exchange(data.m_height, 0)}, m_id{std::exchange(data.m_id, -1)}, m_p_texture{std::exchange(data.m_p_texture, nullptr)},
		m_p_texture_depth_stencil{std::exchange(data.m_p_texture_depth_stencil, nullptr)},
		m_p_framebuffer{std::exchange(data.m_p_framebuffer, nullptr)}, m_p_render_pass{std::exchange(data.m_p_render_pass, nullptr)}
	{}

	// overload because we don't need to copy information about how our object was allocated (from storage or on stack, see m_is_allocated_on_stack)
	FramebufferData& operator=(FramebufferData&& data) noexcept
	{
		m_is_render_target = std::exchange(data.m_is_render_target, true);
		m_width = std::exchange(data.m_width, 0);
		m_height = std::exchange(data.m_height, 0);
		m_id = std::exchange(data.m_id, -1);
		m_p_texture = std::exchange(data.m_p_texture, nullptr);
		m_p_texture_depth_stencil = std::exchange(data.m_p_texture_depth_stencil, nullptr);
		m_p_framebuffer = std::exchange(data.m_p_framebuffer, nullptr);
		m_p_render_pass = std::exchange(data.m_p_render_pass, nullptr);
		return *this;
	}

	~FramebufferData()
	{
		m_id = -1;

#ifdef RMLUI_VK_DEBUG
		if (m_is_allocated_on_stack == false)
			RMLUI_ASSERTMSG(m_p_texture == nullptr, "you must manually deallocate texture and set this field as nullptr!");
#endif
	}

	int Get_Width() const { return m_width; }
	void Set_Width(int value) { m_width = value; }

	int Get_Height() const { return m_height; }
	void Set_Height(int value) { m_height = value; }

	void Set_ID(int layer_current_size_index) { m_id = layer_current_size_index; }
	int Get_ID() const { return m_id; }

	void Set_Texture(RenderInterface_VK::TextureHandleType* p_texture) { m_p_texture = p_texture; }
	RenderInterface_VK::TextureHandleType* Get_Texture() const { return m_p_texture; }

	void Set_SharedDepthStencilTexture(FramebufferData* p_data) { m_p_texture_depth_stencil = p_data; }
	FramebufferData* Get_SharedDepthStencilTexture() const { return m_p_texture_depth_stencil; }

	// The Vulkan framebuffer object; null for the shared depth-stencil data block (it owns no framebuffer by itself).
	void Set_Framebuffer(VkFramebuffer p_framebuffer) { m_p_framebuffer = p_framebuffer; }
	VkFramebuffer Get_Framebuffer() const { return m_p_framebuffer; }

	// The render pass this framebuffer is used with (stored at creation time): the layer pass for layer framebuffers,
	// the postprocess pass for postprocess framebuffers, null for the shared depth-stencil data block.
	void Set_RenderPass(VkRenderPass p_render_pass) { m_p_render_pass = p_render_pass; }
	VkRenderPass Get_RenderPass() const { return m_p_render_pass; }

	bool Is_RenderTarget() const { return m_is_render_target; }
	void Set_RenderTarget(bool value) { m_is_render_target = value; }

#ifdef RMLUI_VK_DEBUG
	// in order to prevent false triggering assert in destructor we determine if object was created on stack or not
	// if not we manually set this field to false
	bool m_is_allocated_on_stack = true;
#endif

private:
	bool m_is_render_target;
	int m_width;
	int m_height;
	int m_id;
	RenderInterface_VK::TextureHandleType* m_p_texture;
	// this is shared texture and original pointer stored and managed at renderlayerstack class
	FramebufferData* m_p_texture_depth_stencil;
	VkFramebuffer m_p_framebuffer;
	VkRenderPass m_p_render_pass;
};
} // namespace Gfx

namespace {
// Transient descriptor sets of the 'blend_mask' layout, allocated per mask-image filter application (the header is
// frozen, so this deferred-deletion list lives at file scope). Each entry is stamped with the renderer's frame counter
// and freed once a full swapchain cycle passed, so a set is never released while still referenced by in-flight GPU
// work. Not thread-safe by design: like the rest of this backend (and the DX12 renderer it mirrors), all rendering
// calls are expected from a single render thread. Entries are tagged with the owning descriptor manager so that
// multiple coexisting RenderInterface_VK instances only ever free their own sets.
struct PendingDescriptorSetDeletion {
	VkDescriptorSet p_set;
	uint64_t frame_stamp;
	RenderInterface_VK::DescriptorPoolManager* p_owner;
};
Rml::Vector<PendingDescriptorSetDeletion> g_pending_for_deletion_descriptor_sets;

// Frees this renderer's transient descriptor sets that were retired a full swapchain cycle ago (or all of them when
// force_all is set, which is only done at shutdown after the device was drained).
void Update_PendingForDeletion_DescriptorSets(VkDevice p_device, RenderInterface_VK::DescriptorPoolManager* p_manager, uint64_t frame_counter,
	bool force_all)
{
	for (auto it = g_pending_for_deletion_descriptor_sets.begin(); it != g_pending_for_deletion_descriptor_sets.end();)
	{
		if (it->p_owner == p_manager && (force_all || frame_counter - it->frame_stamp >= uint64_t(RenderInterface_VK::kSwapchainBackBufferCount)))
		{
			p_manager->Free_Descriptors(p_device, &it->p_set);
			it = g_pending_for_deletion_descriptor_sets.erase(it);
		}
		else
		{
			++it;
		}
	}
}
} // namespace

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

RenderInterface_VK::RenderInterface_VK() :
	m_is_transform_enabled{false}, m_is_scissor_was_set{false}, m_is_stencil_enabled{false}, m_is_stencil_equal{false}, m_is_use_msaa{false},
	m_msaa_sample_count{1}, m_width{}, m_height{}, m_current_clip_operation{-1}, m_active_program_id{ProgramId::None}, m_scissor{},
	m_stencil_ref_value{}, m_queue_index_present{}, m_queue_index_graphics{}, m_queue_index_compute{}, m_semaphore_index{},
	m_semaphore_index_previous{}, m_image_index{}, m_frame_counter{}, m_p_instance{}, m_p_device{}, m_p_physical_device{}, m_p_surface{},
	m_p_swapchain{}, m_p_allocator{}, m_p_current_command_buffer{}, m_p_descriptor_set_layout_transform{}, m_p_descriptor_set_layout_texture{},
	m_p_descriptor_set_layout_blend_mask{}, m_p_pipeline_layout_transform{}, m_p_pipeline_layout_transform_texture{}, m_p_pipeline_layout_texture{},
	m_p_pipeline_layout_texture_effect{}, m_p_pipeline_layout_blend_mask{}, m_p_render_pass_layer{}, m_p_render_pass_layer_clear{},
	m_p_render_pass_layer_clear_all{}, m_p_render_pass_layer_clear_ds{}, m_p_render_pass_postprocess{}, m_p_render_pass_swapchain{},
	m_p_sampler_linear{}, m_scissor_original{}, m_viewport{}, m_p_active_render_pass{}, m_p_active_framebuffer{},
	m_p_queue_present{}, m_p_queue_graphics{}, m_p_queue_compute{},
#ifdef RMLUI_VK_DEBUG
	m_debug_messenger{},
#endif
	m_swapchain_format{}, m_texture_depthstencil{}, m_projection{}, m_constant_buffer_data_transform{}, m_precompiled_fullscreen_quad_geometry{}
{
	static_assert(RenderInterface_VK::NumPrograms == static_cast<size_t>(ProgramId::Count),
		"Please adjust the NumPrograms constant for consistency with the number of program IDs.");

	std::memset(m_pipelines, 0, sizeof(m_pipelines));
	std::memset(m_shaders, 0, sizeof(m_shaders));

	for (auto& count : m_constant_buffer_count_per_frame)
		count = 0;

	m_pending_for_deletion_geometries.reserve(128);
	m_pending_for_deletion_textures.reserve(128);
}

RenderInterface_VK::~RenderInterface_VK() {}

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

	if (m_p_device)
	{
		// full GPU drain, like the DX12 renderer's destructor does through Flush()
		Flush();
	}

	DestroyResourcesDependentOnSize();
	Destroy_Resources();
	Destroy_Allocator();
	Destroy_SyncPrimitives();

	if (m_p_swapchain)
	{
		Destroy_Swapchain();
		m_p_swapchain = nullptr;
	}

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
	// the validation-features block is only initialized when the CPU validation layer was found (debug builds);
	// never attach it uninitialized (its sType would be 0, which is invalid)
	info_instance.pNext =
		(debug_validation_features_ext.sType == VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT) ? &debug_validation_features_ext : nullptr;
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

	if (selected_physical_device == nullptr)
		return;

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
	RMLUI_ASSERT(m_p_instance && "you must initialize your VkInstance");

	if (m_p_instance == nullptr)
		return;

	bool result = create_surface_callback(m_p_instance, &m_p_surface);
	RMLUI_VK_ASSERTMSG(result && m_p_surface, "failed to call create_surface_callback");
}

void RenderInterface_VK::Initialize_QueueIndecies() noexcept
{
	RMLUI_ASSERT(m_p_physical_device && "you must initialize your physical device");
	RMLUI_ASSERT(m_p_surface && "you must initialize VkSurfaceKHR before calling this method");

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
	RMLUI_ASSERT(m_p_device && "you must initialize VkDevice before using this method");

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
	RMLUI_ASSERT(m_p_device && "you must initialize your device");

	m_executed_fences.resize(kSwapchainBackBufferCount);
	m_semaphores_image_available.resize(kSwapchainBackBufferCount);

	for (uint32_t i = 0; i < kSwapchainBackBufferCount; ++i)
		m_submitted_fences[i] = false;

	VkResult status = VK_SUCCESS;

	for (uint32_t i = 0; i < kSwapchainBackBufferCount; ++i)
	{
		VkFenceCreateInfo info_fence = {};

		info_fence.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		info_fence.pNext = nullptr;
		// created unsignaled on purpose: a fence may only be submitted when unsignaled, and slots that were never
		// submitted are skipped in Wait() (tracked via m_submitted_fences)
		info_fence.flags = 0;

		status = vkCreateFence(m_p_device, &info_fence, nullptr, &m_executed_fences[i]);

		RMLUI_VK_ASSERTMSG(status == VK_SUCCESS, "failed to vkCreateFence");

		VkSemaphoreCreateInfo info_semaphore = {};

		info_semaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		info_semaphore.pNext = nullptr;
		info_semaphore.flags = 0;

		status = vkCreateSemaphore(m_p_device, &info_semaphore, nullptr, &m_semaphores_image_available[i]);

		RMLUI_VK_ASSERTMSG(status == VK_SUCCESS, "failed to vkCreateSemaphore");

		// note: the render-finished semaphores are NOT created here but in Create_SwapchainImages, one per swapchain
		// image (their reuse is tied to the presentation engine releasing the image, not to frame slots)
	}
}

void RenderInterface_VK::Initialize_Resources(const VkPhysicalDeviceProperties& physical_device_properties) noexcept
{
	RMLUI_ZoneScopedN("Vulkan - Initialize_Resources");

	m_command_buffer_ring.Initialize(m_p_device, m_queue_index_graphics);

	const unsigned char max_msaa_supported_sample_count = GetMSAASupportedSampleCount(RMLUI_RENDER_BACKEND_FIELD_MSAA_SAMPLE_COUNT);

	// requested count is 1 so we force it to false because in case if GPU doesn't support multisampling and returns 1 we get 1 == 1 situation
	// and m_is_use_msaa will be true but it is not right and validation layers will get assertions about this situation like we want to resolve
	// resource that as source with sample count equal to 1 but it expects to be >= 2
	m_is_use_msaa = (RMLUI_RENDER_BACKEND_FIELD_MSAA_SAMPLE_COUNT != 1) && (max_msaa_supported_sample_count > 1);
	m_msaa_sample_count = m_is_use_msaa ? max_msaa_supported_sample_count : static_cast<unsigned char>(1);

#ifdef RMLUI_VK_DEBUG
	Rml::Log::Message(Rml::Log::LT_INFO, "[Vulkan] Max supported MSAA sample count: %d", int(max_msaa_supported_sample_count));
	Rml::Log::Message(Rml::Log::LT_INFO, "[Vulkan] Requested MSAA level: %d | used: %d", int(RMLUI_RENDER_BACKEND_FIELD_MSAA_SAMPLE_COUNT),
		int(m_msaa_sample_count));
	Rml::Log::Message(Rml::Log::LT_INFO, "[Vulkan] MSAA: %s", m_is_use_msaa ? "supported and enabled" : "not supported or disabled");
#endif

	Create_Shaders();
	Create_DescriptorSetLayouts();
	Create_PipelineLayouts();
	Create_Samplers();

	m_manager_descriptors.Initialize(m_p_device, RMLUI_RENDER_BACKEND_FIELD_DESCRIPTORAMOUNT_FOR_UNIFORM_BUFFER_DYNAMIC,
		RMLUI_RENDER_BACKEND_FIELD_DESCRIPTORAMOUNT_FOR_IMAGE_SAMPLER, 8, 8);

	m_upload_manager.Initialize(m_p_device, m_p_queue_graphics, m_p_allocator, m_queue_index_graphics,
		RMLUI_RENDER_BACKEND_FIELD_STAGING_BUFFER_SIZE);

	// dynamic UBO offsets must be aligned to the device's limit (which can exceed the compile-time default of 256)
	VkDeviceSize constant_buffer_alignment = physical_device_properties.limits.minUniformBufferOffsetAlignment;
	if (constant_buffer_alignment < static_cast<VkDeviceSize>(RMLUI_RENDER_BACKEND_FIELD_ALIGNMENT_FOR_BUFFER))
		constant_buffer_alignment = RMLUI_RENDER_BACKEND_FIELD_ALIGNMENT_FOR_BUFFER;

	m_manager_buffer.Initialize(m_p_device, m_p_allocator, &m_manager_descriptors, m_p_descriptor_set_layout_transform, constant_buffer_alignment);
	m_manager_texture.Initialize(this, m_p_device, m_p_allocator, &m_upload_manager, &m_manager_descriptors, m_p_descriptor_set_layout_texture,
		m_p_sampler_linear);
	m_manager_render_layer.Initialize(this);

	Rml::Mesh mesh;
	Rml::MeshUtilities::GenerateQuad(mesh, Rml::Vector2f(-1.f), Rml::Vector2f(2.f), {});

	m_precompiled_fullscreen_quad_geometry = RenderInterface_VK::CompileGeometry(mesh.vertices, mesh.indices);
}

void RenderInterface_VK::Initialize_Allocator() noexcept
{
	RMLUI_ASSERT(m_p_device && "you must have a valid VkDevice here");
	RMLUI_ASSERT(m_p_physical_device && "you must have a valid VkPhysicalDevice here");
	RMLUI_ASSERT(m_p_instance && "you must have a valid VkInstance here");

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
	RMLUI_ZoneScopedN("Vulkan - Destroy_Resources");

	// the device must be idle here (Shutdown drains it), so immediate destruction is safe
	m_manager_render_layer.Shutdown();

	// drain the transient blend-mask descriptor sets of this renderer (none of them can be referenced by the GPU
	// anymore); foreign entries of other live renderer instances are left untouched
	Update_PendingForDeletion_DescriptorSets(m_p_device, &m_manager_descriptors, m_frame_counter, true);

	if (m_precompiled_fullscreen_quad_geometry)
	{
		Free_Geometry(reinterpret_cast<GeometryHandleType*>(m_precompiled_fullscreen_quad_geometry));
		m_precompiled_fullscreen_quad_geometry = {};
	}

	// free the preallocated per-slot constant buffer rings (the DX12 renderer's Destroy_Resource_For_Shaders)
	for (auto& deque_cbs : m_constantbuffers)
	{
		for (auto& cb : deque_cbs)
		{
			if (cb.m_alloc_info.buffer_index != -1)
				m_manager_buffer.Free_ConstantBuffer(&cb);
		}

		deque_cbs.clear();
	}

	Destroy_Geometries();
	Destroy_Textures();

	m_manager_buffer.Shutdown();
	m_manager_texture.Shutdown();
	m_upload_manager.Shutdown(m_p_allocator);
	m_command_buffer_ring.Shutdown();

	Destroy_Pipelines();
	DestroyRenderPasses();
	DestroyPipelineLayouts();
	DestroyDescriptorSetLayouts();
	DestroySamplers();
	Destroy_Shaders();

	// the by-value depth-stencil handle was destroyed inline by Destroy_Texture (possibly several times, on resize);
	// mark it once here so its destructor's leak check passes (see Destroy_Texture for the reasoning)
	m_texture_depthstencil.Mark_Destroyed();

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
		RMLUI_ASSERT(p_instance_layer_name && "you have an invalid layer");
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
	RMLUI_ASSERT(m_p_physical_device && "you must initialize your physical device. Call InitializePhysicalDevice first");

	if (m_p_physical_device == nullptr)
		return;

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
	RMLUI_ASSERT(physical_devices.empty() == false &&
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

	RMLUI_ASSERT(m_p_physical_device && "you must initialize your physical device, before calling this method");
	RMLUI_ASSERT(m_p_surface && "you must initialize your surface, before calling this method");

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

	for (uint32_t i = 0; i < 4; ++i)
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
	RMLUI_ASSERT(user_swapchain_count_for_creation >= caps.minImageCount &&
		"can't be, you must have a valid count that bounds from minImageCount to maxImageCount! Otherwise you will get a validation error that "
		"specifies that you created a swapchain with invalid image count");
	RMLUI_ASSERT(user_swapchain_count_for_creation <= caps.maxImageCount &&
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
	RMLUI_ASSERT(m_p_device && "you must initialize your device, before calling this method");
	RMLUI_ASSERT(m_p_physical_device && "you must initialize your physical device, before calling this method");
	RMLUI_ASSERT(m_p_surface && "you must initialize your surface, before calling this method");

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
	RMLUI_ASSERT(m_p_device && "you must initialize your device, before calling this method");
	RMLUI_ASSERT(m_p_physical_device && "you must initialize your physical device, before calling this method");
	RMLUI_ASSERT(m_p_surface && "you must initialize your surface, before calling this method");

	VkSurfaceCapabilitiesKHR result;
	VkResult status = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_p_physical_device, m_p_surface, &result);
	RMLUI_VK_ASSERTMSG(status == VK_SUCCESS, "[Vulkan] failed to vkGetPhysicalDeviceSurfaceCapabilitiesKHR");

	return result;
}

unsigned char RenderInterface_VK::GetMSAASupportedSampleCount(unsigned char max_samples)
{
	RMLUI_ZoneScopedN("Vulkan - GetMSAASupportedSampleCount");
	RMLUI_VK_ASSERTMSG(m_p_physical_device, "early calling, you must pick a physical device before calling this method");

	if (m_p_physical_device == nullptr)
		return 1;

	VkPhysicalDeviceProperties properties = {};
	vkGetPhysicalDeviceProperties(m_p_physical_device, &properties);

	const VkSampleCountFlags supported_samples = properties.limits.framebufferColorSampleCounts & properties.limits.framebufferDepthSampleCounts &
		properties.limits.framebufferStencilSampleCounts;

	static constexpr unsigned char kPossibleCounts[] = {64, 32, 16, 8, 4, 2};

	for (unsigned char count : kPossibleCounts)
	{
		if (count <= max_samples && (supported_samples & count))
			return count;
	}

	return 1;
}

VkFormat RenderInterface_VK::Get_SupportedDepthFormat()
{
	RMLUI_VK_ASSERTMSG(m_p_physical_device, "you must initialize and pick physical device for your renderer");

	// only stencil-capable formats: the clip-mask feature programs its stencil channel (the DX12 renderer hardcodes
	// D32_FLOAT_S8X24_UINT for the same reason); every relevant device supports at least one of these
	Rml::Array<VkFormat, 3> formats = {VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT};

	VkFormatProperties properties;
	for (const auto& format : formats)
	{
		vkGetPhysicalDeviceFormatProperties(m_p_physical_device, format, &properties);

		if (properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
		{
			return format;
		}
	}

	RMLUI_VK_ASSERTMSG(false, "no stencil-capable depth-stencil format is supported by this device, the clip mask feature requires one");
	return VkFormat::VK_FORMAT_UNDEFINED;
}

void RenderInterface_VK::Create_Shaders() noexcept
{
	RMLUI_ASSERT(m_p_device && "you must initialize VkDevice before calling this method");

	struct shader_data_t {
		const uint32_t* m_data = nullptr;
		size_t m_data_size = 0;
	};

	// let it be on stack don't use constexpr because it will grow compiled library size
	shader_data_t shaders[static_cast<int>(eVKShaderID::count)];

	shaders[static_cast<int>(eVKShaderID::shader_frag_blend_mask)] = {reinterpret_cast<const uint32_t*>(shader_frag_blend_mask),
		sizeof(shader_frag_blend_mask)};
	shaders[static_cast<int>(eVKShaderID::shader_frag_blur)] = {reinterpret_cast<const uint32_t*>(shader_frag_blur), sizeof(shader_frag_blur)};
	shaders[static_cast<int>(eVKShaderID::shader_frag_color)] = {reinterpret_cast<const uint32_t*>(shader_frag_color), sizeof(shader_frag_color)};
	shaders[static_cast<int>(eVKShaderID::shader_frag_color_matrix)] = {reinterpret_cast<const uint32_t*>(shader_frag_color_matrix),
		sizeof(shader_frag_color_matrix)};
	shaders[static_cast<int>(eVKShaderID::shader_frag_creation)] = {reinterpret_cast<const uint32_t*>(shader_frag_creation),
		sizeof(shader_frag_creation)};
	shaders[static_cast<int>(eVKShaderID::shader_frag_drop_shadow)] = {reinterpret_cast<const uint32_t*>(shader_frag_drop_shadow),
		sizeof(shader_frag_drop_shadow)};
	shaders[static_cast<int>(eVKShaderID::shader_frag_gradient)] = {reinterpret_cast<const uint32_t*>(shader_frag_gradient),
		sizeof(shader_frag_gradient)};
	shaders[static_cast<int>(eVKShaderID::shader_frag_passthrough)] = {reinterpret_cast<const uint32_t*>(shader_frag_passthrough),
		sizeof(shader_frag_passthrough)};
	shaders[static_cast<int>(eVKShaderID::shader_frag_texture)] = {reinterpret_cast<const uint32_t*>(shader_frag_texture),
		sizeof(shader_frag_texture)};
	shaders[static_cast<int>(eVKShaderID::shader_vert_blur)] = {reinterpret_cast<const uint32_t*>(shader_vert_blur), sizeof(shader_vert_blur)};
	shaders[static_cast<int>(eVKShaderID::shader_vert_main)] = {reinterpret_cast<const uint32_t*>(shader_vert_main), sizeof(shader_vert_main)};
	shaders[static_cast<int>(eVKShaderID::shader_vert_passthrough)] = {reinterpret_cast<const uint32_t*>(shader_vert_passthrough),
		sizeof(shader_vert_passthrough)};

	static_assert(sizeof(m_shaders) / sizeof(m_shaders[0]) == sizeof(shaders) / sizeof(shaders[0]),
		"something is wrong, different amount of shaders!");

	constexpr int max_size = static_cast<int>(sizeof(m_shaders) / sizeof(m_shaders[0]));

	for (int i = 0; i < max_size; ++i)
	{
		RMLUI_ASSERT(shaders[i].m_data_size > 0 && "you forgot to initialize some of shader slot");

		VkShaderModuleCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		info.pCode = shaders[i].m_data;
		info.codeSize = shaders[i].m_data_size;

		VkShaderModule p_module = nullptr;
		VkResult status = vkCreateShaderModule(m_p_device, &info, nullptr, &p_module);

		RMLUI_VK_ASSERTMSG(status == VK_SUCCESS, "[Vulkan] failed to vkCreateShaderModule");

		m_shaders[i] = p_module;
	}
}

void RenderInterface_VK::Destroy_Shaders() noexcept
{
	RMLUI_VK_ASSERTMSG(m_p_device, "you must have a valid VkDevice here");

	for (auto& p_module : m_shaders)
	{
		if (p_module)
		{
			vkDestroyShaderModule(m_p_device, p_module, nullptr);
			p_module = nullptr;
		}
	}
}

void RenderInterface_VK::Create_DescriptorSetLayouts() noexcept
{
	RMLUI_ASSERT(m_p_device && "you must initialize VkDevice before calling this method");
	RMLUI_ASSERT(
		!m_p_descriptor_set_layout_transform && !m_p_descriptor_set_layout_texture && !m_p_descriptor_set_layout_blend_mask && "Already initialized");

	{
		// 'transform': single dynamic uniform buffer, visible to both stages (the analog of DX12's root CBVs at b0).
		VkDescriptorSetLayoutBinding binding_for_transform = {};
		binding_for_transform.binding = 0;
		binding_for_transform.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		binding_for_transform.descriptorCount = 1;
		binding_for_transform.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		info.pBindings = &binding_for_transform;
		info.bindingCount = 1;

		VkResult status = vkCreateDescriptorSetLayout(m_p_device, &info, nullptr, &m_p_descriptor_set_layout_transform);
		RMLUI_VK_ASSERTMSG(status == VK_SUCCESS, "[Vulkan] failed to vkCreateDescriptorSetLayout (transform)");
	}

	{
		// 'texture': single combined image sampler.
		VkDescriptorSetLayoutBinding binding_for_texture = {};
		binding_for_texture.binding = 0;
		binding_for_texture.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		binding_for_texture.descriptorCount = 1;
		binding_for_texture.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		info.pBindings = &binding_for_texture;
		info.bindingCount = 1;

		VkResult status = vkCreateDescriptorSetLayout(m_p_device, &info, nullptr, &m_p_descriptor_set_layout_texture);
		RMLUI_VK_ASSERTMSG(status == VK_SUCCESS, "[Vulkan] failed to vkCreateDescriptorSetLayout (texture)");
	}

	{
		// 'blend_mask': two combined image samplers (input texture + mask texture).
		VkDescriptorSetLayoutBinding bindings_for_blend_mask[2] = {};
		bindings_for_blend_mask[0].binding = 0;
		bindings_for_blend_mask[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		bindings_for_blend_mask[0].descriptorCount = 1;
		bindings_for_blend_mask[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		bindings_for_blend_mask[1].binding = 1;
		bindings_for_blend_mask[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		bindings_for_blend_mask[1].descriptorCount = 1;
		bindings_for_blend_mask[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		info.pBindings = bindings_for_blend_mask;
		info.bindingCount = 2;

		VkResult status = vkCreateDescriptorSetLayout(m_p_device, &info, nullptr, &m_p_descriptor_set_layout_blend_mask);
		RMLUI_VK_ASSERTMSG(status == VK_SUCCESS, "[Vulkan] failed to vkCreateDescriptorSetLayout (blend mask)");
	}
}

void RenderInterface_VK::DestroyDescriptorSetLayouts() noexcept
{
	RMLUI_VK_ASSERTMSG(m_p_device, "you must have a valid VkDevice here");

	if (m_p_descriptor_set_layout_transform)
	{
		vkDestroyDescriptorSetLayout(m_p_device, m_p_descriptor_set_layout_transform, nullptr);
		m_p_descriptor_set_layout_transform = nullptr;
	}

	if (m_p_descriptor_set_layout_texture)
	{
		vkDestroyDescriptorSetLayout(m_p_device, m_p_descriptor_set_layout_texture, nullptr);
		m_p_descriptor_set_layout_texture = nullptr;
	}

	if (m_p_descriptor_set_layout_blend_mask)
	{
		vkDestroyDescriptorSetLayout(m_p_device, m_p_descriptor_set_layout_blend_mask, nullptr);
		m_p_descriptor_set_layout_blend_mask = nullptr;
	}
}

void RenderInterface_VK::Create_PipelineLayouts() noexcept
{
	RMLUI_VK_ASSERTMSG(m_p_device, "[Vulkan] you must initialize VkDevice before calling this method");
	RMLUI_VK_ASSERTMSG(m_p_descriptor_set_layout_transform && m_p_descriptor_set_layout_texture && m_p_descriptor_set_layout_blend_mask,
		"[Vulkan] you must initialize the descriptor set layouts before calling this method");

	const auto create_layout = [this](const VkDescriptorSetLayout* p_set_layouts, uint32_t count, VkPipelineLayout* p_out) {
		VkPipelineLayoutCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		info.pNext = nullptr;
		info.pSetLayouts = p_set_layouts;
		info.setLayoutCount = count;

		VkResult status = vkCreatePipelineLayout(m_p_device, &info, nullptr, p_out);
		RMLUI_VK_ASSERTMSG(status == VK_SUCCESS, "[Vulkan] failed to vkCreatePipelineLayout");
	};

	create_layout(&m_p_descriptor_set_layout_transform, 1, &m_p_pipeline_layout_transform);

	{
		VkDescriptorSetLayout p_layouts[] = {m_p_descriptor_set_layout_transform, m_p_descriptor_set_layout_texture};
		create_layout(p_layouts, 2, &m_p_pipeline_layout_transform_texture);
	}

	create_layout(&m_p_descriptor_set_layout_texture, 1, &m_p_pipeline_layout_texture);

	{
		VkDescriptorSetLayout p_layouts[] = {m_p_descriptor_set_layout_texture, m_p_descriptor_set_layout_transform};
		create_layout(p_layouts, 2, &m_p_pipeline_layout_texture_effect);
	}

	create_layout(&m_p_descriptor_set_layout_blend_mask, 1, &m_p_pipeline_layout_blend_mask);
}

void RenderInterface_VK::DestroyPipelineLayouts() noexcept
{
	RMLUI_VK_ASSERTMSG(m_p_device, "you must have a valid VkDevice here");

	if (m_p_pipeline_layout_transform)
	{
		vkDestroyPipelineLayout(m_p_device, m_p_pipeline_layout_transform, nullptr);
		m_p_pipeline_layout_transform = nullptr;
	}

	if (m_p_pipeline_layout_transform_texture)
	{
		vkDestroyPipelineLayout(m_p_device, m_p_pipeline_layout_transform_texture, nullptr);
		m_p_pipeline_layout_transform_texture = nullptr;
	}

	if (m_p_pipeline_layout_texture)
	{
		vkDestroyPipelineLayout(m_p_device, m_p_pipeline_layout_texture, nullptr);
		m_p_pipeline_layout_texture = nullptr;
	}

	if (m_p_pipeline_layout_texture_effect)
	{
		vkDestroyPipelineLayout(m_p_device, m_p_pipeline_layout_texture_effect, nullptr);
		m_p_pipeline_layout_texture_effect = nullptr;
	}

	if (m_p_pipeline_layout_blend_mask)
	{
		vkDestroyPipelineLayout(m_p_device, m_p_pipeline_layout_blend_mask, nullptr);
		m_p_pipeline_layout_blend_mask = nullptr;
	}
}

void RenderInterface_VK::Create_Samplers() noexcept
{
	VkSamplerCreateInfo info = {};

	info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	info.pNext = nullptr;
	info.magFilter = VK_FILTER_LINEAR;
	info.minFilter = VK_FILTER_LINEAR;
	info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	info.minLod = 0.0f;
	info.maxLod = 1000.0f;

	VkResult status = vkCreateSampler(m_p_device, &info, nullptr, &m_p_sampler_linear);
	RMLUI_VK_ASSERTMSG(status == VK_SUCCESS, "failed to vkCreateSampler");
}

void RenderInterface_VK::DestroySamplers() noexcept
{
	RMLUI_VK_ASSERTMSG(m_p_device, "must exist here");

	if (m_p_sampler_linear)
	{
		vkDestroySampler(m_p_device, m_p_sampler_linear, nullptr);
		m_p_sampler_linear = nullptr;
	}
}

void RenderInterface_VK::Create_RenderPasses() noexcept
{
	RMLUI_VK_ASSERTMSG(m_p_device, "you must have a valid VkDevice here");
	RMLUI_VK_ASSERTMSG(m_p_render_pass_layer == nullptr && m_p_render_pass_layer_clear == nullptr && m_p_render_pass_layer_clear_all == nullptr &&
			m_p_render_pass_layer_clear_ds == nullptr && m_p_render_pass_postprocess == nullptr && m_p_render_pass_swapchain == nullptr,
		"render passes are created once (they don't depend on the framebuffer size)");

	const VkSampleCountFlagBits msaa_samples = static_cast<VkSampleCountFlagBits>(m_msaa_sample_count);
	const VkFormat depth_format = Get_SupportedDepthFormat();

	RMLUI_VK_ASSERTMSG(depth_format != VkFormat::VK_FORMAT_UNDEFINED,
		"can't obtain depth format, your device doesn't support depth/stencil operations");

	// --- layer render passes: (optionally MSAA) color + depth-stencil ---
	// four variants that differ only in the attachment load ops: LOAD/LOAD preserves the contents when an existing
	// layer is re-bound, CLEAR/LOAD clears the color when a layer is (re)pushed or the frame begins (the DX12 renderer
	// clears the layer's RTV there), CLEAR/CLEAR is used by Clear(), LOAD/CLEAR is used when the clip-mask stencil
	// clear happens to be the first command of a fresh pass. The variants are mutually compatible render passes, so
	// the same framebuffers work with all of them.
	for (int variant = 0; variant < 4; ++variant)
	{
		// variant 0 = layer (LOAD/LOAD), 1 = layer_clear (CLEAR color/LOAD DS), 2 = layer_clear_all (CLEAR/CLEAR),
		// 3 = layer_clear_ds (LOAD color/CLEAR DS)
		const bool clear_color = (variant == 1 || variant == 2);
		const bool clear_depth_stencil = (variant >= 2);

		Rml::Array<VkAttachmentDescription, 2> attachments = {};

		attachments[0].format = RMLUI_RENDER_BACKEND_FIELD_COLOR_TEXTURE_FORMAT;
		attachments[0].samples = msaa_samples;
		attachments[0].loadOp = clear_color ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
		attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		attachments[1].format = depth_format;
		attachments[1].samples = msaa_samples;
		attachments[1].loadOp = clear_depth_stencil ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
		attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[1].stencilLoadOp = clear_depth_stencil ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
		attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference ref_color = {};
		ref_color.attachment = 0;
		ref_color.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference ref_depth_stencil = {};
		ref_depth_stencil.attachment = 1;
		ref_depth_stencil.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &ref_color;
		subpass.pDepthStencilAttachment = &ref_depth_stencil;

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
		info.attachmentCount = static_cast<uint32_t>(attachments.size());
		info.pAttachments = attachments.data();
		info.subpassCount = 1;
		info.pSubpasses = &subpass;
		info.dependencyCount = static_cast<uint32_t>(dependencies.size());
		info.pDependencies = dependencies.data();

		VkRenderPass* p_render_pass_slot = (variant == 0) ? &m_p_render_pass_layer
			: (variant == 1)                              ? &m_p_render_pass_layer_clear
			: (variant == 2)                              ? &m_p_render_pass_layer_clear_all
														  : &m_p_render_pass_layer_clear_ds;

		VkResult status = vkCreateRenderPass(m_p_device, &info, nullptr, p_render_pass_slot);
		RMLUI_VK_ASSERTMSG(status == VK_SUCCESS, "failed to vkCreateRenderPass (layer)");
	}

	// --- postprocess render pass: single-sample color only (filters, blur, ...) ---
	{
		VkAttachmentDescription attachment = {};
		attachment.format = RMLUI_RENDER_BACKEND_FIELD_COLOR_TEXTURE_FORMAT;
		attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference ref_color = {};
		ref_color.attachment = 0;
		ref_color.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &ref_color;

		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcAccessMask = 0;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		VkRenderPassCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		info.attachmentCount = 1;
		info.pAttachments = &attachment;
		info.subpassCount = 1;
		info.pSubpasses = &subpass;
		info.dependencyCount = 1;
		info.pDependencies = &dependency;

		VkResult status = vkCreateRenderPass(m_p_device, &info, nullptr, &m_p_render_pass_postprocess);
		RMLUI_VK_ASSERTMSG(status == VK_SUCCESS, "failed to vkCreateRenderPass (postprocess)");
	}

	// --- swapchain render pass: swapchain color (cleared, ends in PRESENT) + main depth-stencil (cleared) ---
	{
		Rml::Array<VkAttachmentDescription, 2> attachments = {};

		attachments[0].format = m_swapchain_format.format;
		attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		attachments[1].format = depth_format;
		attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference ref_color = {};
		ref_color.attachment = 0;
		ref_color.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference ref_depth_stencil = {};
		ref_depth_stencil.attachment = 1;
		ref_depth_stencil.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &ref_color;
		subpass.pDepthStencilAttachment = &ref_depth_stencil;

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
		info.attachmentCount = static_cast<uint32_t>(attachments.size());
		info.pAttachments = attachments.data();
		info.subpassCount = 1;
		info.pSubpasses = &subpass;
		info.dependencyCount = static_cast<uint32_t>(dependencies.size());
		info.pDependencies = dependencies.data();

		VkResult status = vkCreateRenderPass(m_p_device, &info, nullptr, &m_p_render_pass_swapchain);
		RMLUI_VK_ASSERTMSG(status == VK_SUCCESS, "failed to vkCreateRenderPass (swapchain)");
	}
}

void RenderInterface_VK::DestroyRenderPasses() noexcept
{
	RMLUI_VK_ASSERTMSG(m_p_device, "you must have a valid VkDevice here");

	if (m_p_render_pass_layer)
	{
		vkDestroyRenderPass(m_p_device, m_p_render_pass_layer, nullptr);
		m_p_render_pass_layer = nullptr;
	}

	if (m_p_render_pass_layer_clear)
	{
		vkDestroyRenderPass(m_p_device, m_p_render_pass_layer_clear, nullptr);
		m_p_render_pass_layer_clear = nullptr;
	}

	if (m_p_render_pass_layer_clear_all)
	{
		vkDestroyRenderPass(m_p_device, m_p_render_pass_layer_clear_all, nullptr);
		m_p_render_pass_layer_clear_all = nullptr;
	}

	if (m_p_render_pass_layer_clear_ds)
	{
		vkDestroyRenderPass(m_p_device, m_p_render_pass_layer_clear_ds, nullptr);
		m_p_render_pass_layer_clear_ds = nullptr;
	}

	if (m_p_render_pass_postprocess)
	{
		vkDestroyRenderPass(m_p_device, m_p_render_pass_postprocess, nullptr);
		m_p_render_pass_postprocess = nullptr;
	}

	if (m_p_render_pass_swapchain)
	{
		vkDestroyRenderPass(m_p_device, m_p_render_pass_swapchain, nullptr);
		m_p_render_pass_swapchain = nullptr;
	}
}

namespace {
// Everything needed to build one of the 23 graphics pipelines; all other state is identical across programs (mirrors
// the DX12 renderer's PSO matrix).
struct PipelineStateDesc {
	VkShaderModule p_shader_vertex = nullptr;
	VkShaderModule p_shader_fragment = nullptr;
	VkPipelineLayout p_pipeline_layout = nullptr;
	VkRenderPass p_render_pass = nullptr;
	VkSampleCountFlagBits sample_count = VK_SAMPLE_COUNT_1_BIT;
	VkPipelineColorBlendAttachmentState blend_state = {};
	VkBool32 stencil_test_enable = VK_FALSE;
	VkStencilOpState stencil_state = {};
	bool is_dynamic_blend_constants = false;
};
} // namespace

// Premultiplied-alpha blending (ONE, ONE_MINUS_SRC_ALPHA), the DX12 renderer's default UI blend state.
static VkPipelineColorBlendAttachmentState Make_BlendState_PremultipliedAlpha(VkColorComponentFlags color_write_mask)
{
	VkPipelineColorBlendAttachmentState result = {};
	result.blendEnable = VK_TRUE;
	result.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	result.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	result.colorBlendOp = VK_BLEND_OP_ADD;
	result.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	result.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	result.alphaBlendOp = VK_BLEND_OP_ADD;
	result.colorWriteMask = color_write_mask;
	return result;
}

// Opacity filter blending: src * blend_constant (the DX12 renderer's OMSetBlendFactor state, dynamic here).
static VkPipelineColorBlendAttachmentState Make_BlendState_Opacity()
{
	VkPipelineColorBlendAttachmentState result = {};
	result.blendEnable = VK_TRUE;
	result.srcColorBlendFactor = VK_BLEND_FACTOR_CONSTANT_COLOR;
	result.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	result.colorBlendOp = VK_BLEND_OP_ADD;
	result.srcAlphaBlendFactor = VK_BLEND_FACTOR_CONSTANT_ALPHA;
	result.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	result.alphaBlendOp = VK_BLEND_OP_ADD;
	result.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	return result;
}

static VkPipelineColorBlendAttachmentState Make_BlendState_Disabled()
{
	VkPipelineColorBlendAttachmentState result = {};
	result.blendEnable = VK_FALSE;
	result.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	result.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	result.colorBlendOp = VK_BLEND_OP_ADD;
	result.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	result.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	result.alphaBlendOp = VK_BLEND_OP_ADD;
	result.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	return result;
}

// Stencil state (front and back faces are identical everywhere, like the DX12 renderer; depthFailOp is KEEP and the
// reference value is dynamic state, set through vkCmdSetStencilReference).
static VkStencilOpState Make_StencilState(VkStencilOp fail_op, VkStencilOp pass_op, VkCompareOp compare_op, uint32_t write_mask)
{
	VkStencilOpState result = {};
	result.failOp = fail_op;
	result.passOp = pass_op;
	result.depthFailOp = VK_STENCIL_OP_KEEP;
	result.compareOp = compare_op;
	result.compareMask = 0xff;
	result.writeMask = write_mask;
	result.reference = 0;
	return result;
}

static void Create_GraphicsPipeline(VkDevice p_device, const PipelineStateDesc& desc, VkPipeline* p_out_pipeline)
{
	RMLUI_VK_ASSERTMSG(p_device, "must be valid device");
	RMLUI_VK_ASSERTMSG(desc.p_shader_vertex && desc.p_shader_fragment, "must be valid shader modules");
	RMLUI_VK_ASSERTMSG(desc.p_pipeline_layout, "must be valid pipeline layout");
	RMLUI_VK_ASSERTMSG(desc.p_render_pass, "must be valid render pass");
	RMLUI_VK_ASSERTMSG(p_out_pipeline, "must be valid out pointer");

	static_assert(sizeof(Rml::Vertex) == 20, "the vertex input layout below assumes sizeof(Rml::Vertex) == 20");

	Rml::Array<VkPipelineShaderStageCreateInfo, 2> stages = {};
	stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	stages[0].module = desc.p_shader_vertex;
	stages[0].pName = "main";
	stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	stages[1].module = desc.p_shader_fragment;
	stages[1].pName = "main";

	VkVertexInputBindingDescription binding_description = {};
	binding_description.binding = 0;
	binding_description.stride = sizeof(Rml::Vertex);
	binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	Rml::Array<VkVertexInputAttributeDescription, 3> attribute_descriptions = {};
	// describe info about our vertex and what is used in vertex shader as "layout(location = X) in"
	attribute_descriptions[0].location = 0;
	attribute_descriptions[0].binding = 0;
	attribute_descriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
	attribute_descriptions[0].offset = offsetof(Rml::Vertex, position);

	attribute_descriptions[1].location = 1;
	attribute_descriptions[1].binding = 0;
	attribute_descriptions[1].format = VK_FORMAT_R8G8B8A8_UNORM;
	attribute_descriptions[1].offset = offsetof(Rml::Vertex, colour);

	attribute_descriptions[2].location = 2;
	attribute_descriptions[2].binding = 0;
	attribute_descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
	attribute_descriptions[2].offset = offsetof(Rml::Vertex, tex_coord);

	VkPipelineVertexInputStateCreateInfo info_vertex = {};
	info_vertex.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	info_vertex.pVertexBindingDescriptions = &binding_description;
	info_vertex.vertexBindingDescriptionCount = 1;
	info_vertex.pVertexAttributeDescriptions = attribute_descriptions.data();
	info_vertex.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size());

	VkPipelineInputAssemblyStateCreateInfo info_assembly_state = {};
	info_assembly_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	info_assembly_state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	info_assembly_state.primitiveRestartEnable = VK_FALSE;

	VkPipelineRasterizationStateCreateInfo info_raster_state = {};
	info_raster_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	info_raster_state.depthClampEnable = VK_FALSE;
	info_raster_state.rasterizerDiscardEnable = VK_FALSE;
	info_raster_state.polygonMode = VK_POLYGON_MODE_FILL;
	info_raster_state.cullMode = VK_CULL_MODE_NONE;
	info_raster_state.frontFace = VK_FRONT_FACE_CLOCKWISE;
	info_raster_state.depthBiasEnable = VK_FALSE;
	info_raster_state.lineWidth = 1.0f;

	VkPipelineColorBlendStateCreateInfo info_color_blend_state = {};
	info_color_blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	info_color_blend_state.attachmentCount = 1;
	info_color_blend_state.pAttachments = &desc.blend_state;

	VkPipelineDepthStencilStateCreateInfo info_depth = {};
	info_depth.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	info_depth.depthTestEnable = VK_FALSE;
	info_depth.depthWriteEnable = VK_FALSE;
	info_depth.depthCompareOp = VK_COMPARE_OP_ALWAYS;
	info_depth.depthBoundsTestEnable = VK_FALSE;
	info_depth.stencilTestEnable = desc.stencil_test_enable;
	info_depth.front = desc.stencil_state;
	info_depth.back = desc.stencil_state;

	VkPipelineViewportStateCreateInfo info_viewport = {};
	info_viewport.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	info_viewport.viewportCount = 1;
	info_viewport.scissorCount = 1;

	VkPipelineMultisampleStateCreateInfo info_multisample = {};
	info_multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	info_multisample.rasterizationSamples = desc.sample_count;
	info_multisample.sampleShadingEnable = VK_FALSE;
	info_multisample.alphaToCoverageEnable = VK_FALSE;

	Rml::Array<VkDynamicState, 4> dynamic_states = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_STENCIL_REFERENCE,
		VK_DYNAMIC_STATE_BLEND_CONSTANTS};

	VkPipelineDynamicStateCreateInfo info_dynamic_state = {};
	info_dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	info_dynamic_state.pDynamicStates = dynamic_states.data();
	info_dynamic_state.dynamicStateCount = desc.is_dynamic_blend_constants ? 4u : 3u;

	VkGraphicsPipelineCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	info.pInputAssemblyState = &info_assembly_state;
	info.pRasterizationState = &info_raster_state;
	info.pColorBlendState = &info_color_blend_state;
	info.pMultisampleState = &info_multisample;
	info.pViewportState = &info_viewport;
	info.pDepthStencilState = &info_depth;
	info.pDynamicState = &info_dynamic_state;
	info.stageCount = static_cast<uint32_t>(stages.size());
	info.pStages = stages.data();
	info.pVertexInputState = &info_vertex;
	info.layout = desc.p_pipeline_layout;
	info.renderPass = desc.p_render_pass;
	info.subpass = 0;

	VkResult status = vkCreateGraphicsPipelines(p_device, nullptr, 1, &info, nullptr, p_out_pipeline);
	RMLUI_VK_ASSERTMSG(status == VkResult::VK_SUCCESS, "failed to vkCreateGraphicsPipelines");
}

void RenderInterface_VK::Create_Pipelines() noexcept
{
	RMLUI_ZoneScopedN("Vulkan - Create_Pipelines");
	RMLUI_VK_ASSERTMSG(m_p_render_pass_layer && m_p_render_pass_layer_clear && m_p_render_pass_layer_clear_all && m_p_render_pass_layer_clear_ds &&
			m_p_render_pass_postprocess && m_p_render_pass_swapchain,
		"render passes must be created first");

	Create_Pipeline_Color();
	Create_Pipeline_Texture();
	Create_Pipeline_Gradient();
	Create_Pipeline_Creation();
	Create_Pipeline_Passthrough();
	Create_Pipeline_Passthrough_NoBlend();
	Create_Pipeline_ColorMatrix();
	Create_Pipeline_BlendMask();
	Create_Pipeline_Blur();
	Create_Pipeline_DropShadow();

	// preallocate the per-slot constant buffer rings (mirrors the DX12 renderer's Create_Resource_Pipelines)
	for (auto& deque_cbs : m_constantbuffers)
	{
		deque_cbs.resize(RMLUI_RENDER_BACKEND_FIELD_PREALLOCATED_CONSTANTBUFFERS);

		for (auto& cb : deque_cbs)
		{
			cb.m_alloc_info = m_manager_buffer.Alloc_ConstantBuffer(&cb, kAllocationSizeMax_ConstantBuffer);
		}
	}

	for (auto& count : m_constant_buffer_count_per_frame)
		count = 0;

	m_pending_for_deletion_geometries.reserve(RMLUI_RENDER_BACKEND_FIELD_PREALLOCATED_CONSTANTBUFFERS);
}

void RenderInterface_VK::Create_Pipeline_Color()
{
	RMLUI_ZoneScopedN("Vulkan - Create_Pipeline_Color");

	PipelineStateDesc desc = {};
	desc.p_shader_vertex = m_shaders[static_cast<int>(eVKShaderID::shader_vert_main)];
	desc.p_shader_fragment = m_shaders[static_cast<int>(eVKShaderID::shader_frag_color)];
	desc.p_pipeline_layout = m_p_pipeline_layout_transform;
	desc.p_render_pass = m_p_render_pass_layer;
	desc.sample_count = static_cast<VkSampleCountFlagBits>(m_msaa_sample_count);

	const VkColorComponentFlags kWriteAll = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	desc.blend_state = Make_BlendState_PremultipliedAlpha(kWriteAll);
	desc.stencil_test_enable = VK_TRUE;

	desc.stencil_state = Make_StencilState(VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_COMPARE_OP_ALWAYS, 0xff);
	Create_GraphicsPipeline(m_p_device, desc, &m_pipelines[static_cast<int>(ProgramId::Color_Stencil_Always)]);

	desc.stencil_state = Make_StencilState(VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_COMPARE_OP_EQUAL, 0xff);
	Create_GraphicsPipeline(m_p_device, desc, &m_pipelines[static_cast<int>(ProgramId::Color_Stencil_Equal)]);

	// the clip-mask writing programs don't write any color (carried-over DX12 quirk), they write stencil only
	desc.blend_state = Make_BlendState_PremultipliedAlpha(0);

	desc.stencil_state = Make_StencilState(VK_STENCIL_OP_KEEP, VK_STENCIL_OP_REPLACE, VK_COMPARE_OP_ALWAYS, 0xff);
	Create_GraphicsPipeline(m_p_device, desc, &m_pipelines[static_cast<int>(ProgramId::Color_Stencil_Set)]);
	Create_GraphicsPipeline(m_p_device, desc, &m_pipelines[static_cast<int>(ProgramId::Color_Stencil_SetInverse)]);

	desc.stencil_state = Make_StencilState(VK_STENCIL_OP_KEEP, VK_STENCIL_OP_INCREMENT_AND_CLAMP, VK_COMPARE_OP_ALWAYS, 0xff);
	Create_GraphicsPipeline(m_p_device, desc, &m_pipelines[static_cast<int>(ProgramId::Color_Stencil_Intersect)]);

	desc.blend_state = Make_BlendState_PremultipliedAlpha(kWriteAll);
	desc.stencil_test_enable = VK_FALSE;
	Create_GraphicsPipeline(m_p_device, desc, &m_pipelines[static_cast<int>(ProgramId::Color_Stencil_Disabled)]);
}

void RenderInterface_VK::Create_Pipeline_Texture()
{
	RMLUI_ZoneScopedN("Vulkan - Create_Pipeline_Texture");

	PipelineStateDesc desc = {};
	desc.p_shader_vertex = m_shaders[static_cast<int>(eVKShaderID::shader_vert_main)];
	desc.p_shader_fragment = m_shaders[static_cast<int>(eVKShaderID::shader_frag_texture)];
	desc.p_pipeline_layout = m_p_pipeline_layout_transform_texture;
	desc.p_render_pass = m_p_render_pass_layer;
	desc.sample_count = static_cast<VkSampleCountFlagBits>(m_msaa_sample_count);
	desc.blend_state =
		Make_BlendState_PremultipliedAlpha(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT);

	desc.stencil_test_enable = VK_TRUE;
	desc.stencil_state = Make_StencilState(VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_COMPARE_OP_ALWAYS, 0xff);
	Create_GraphicsPipeline(m_p_device, desc, &m_pipelines[static_cast<int>(ProgramId::Texture_Stencil_Always)]);

	desc.stencil_state = Make_StencilState(VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_COMPARE_OP_EQUAL, 0xff);
	Create_GraphicsPipeline(m_p_device, desc, &m_pipelines[static_cast<int>(ProgramId::Texture_Stencil_Equal)]);

	desc.stencil_test_enable = VK_FALSE;
	Create_GraphicsPipeline(m_p_device, desc, &m_pipelines[static_cast<int>(ProgramId::Texture_Stencil_Disabled)]);
}

void RenderInterface_VK::Create_Pipeline_Gradient()
{
	RMLUI_ZoneScopedN("Vulkan - Create_Pipeline_Gradient");

	PipelineStateDesc desc = {};
	desc.p_shader_vertex = m_shaders[static_cast<int>(eVKShaderID::shader_vert_main)];
	desc.p_shader_fragment = m_shaders[static_cast<int>(eVKShaderID::shader_frag_gradient)];
	desc.p_pipeline_layout = m_p_pipeline_layout_transform;
	desc.p_render_pass = m_p_render_pass_layer;
	desc.sample_count = static_cast<VkSampleCountFlagBits>(m_msaa_sample_count);
	desc.blend_state =
		Make_BlendState_PremultipliedAlpha(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT);

	desc.stencil_test_enable = VK_TRUE;
	desc.stencil_state = Make_StencilState(VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_COMPARE_OP_ALWAYS, 0x0);
	Create_GraphicsPipeline(m_p_device, desc, &m_pipelines[static_cast<int>(ProgramId::Gradient)]);
}

void RenderInterface_VK::Create_Pipeline_Creation()
{
	RMLUI_ZoneScopedN("Vulkan - Create_Pipeline_Creation");

	PipelineStateDesc desc = {};
	desc.p_shader_vertex = m_shaders[static_cast<int>(eVKShaderID::shader_vert_main)];
	desc.p_shader_fragment = m_shaders[static_cast<int>(eVKShaderID::shader_frag_creation)];
	desc.p_pipeline_layout = m_p_pipeline_layout_transform;
	desc.p_render_pass = m_p_render_pass_layer;
	desc.sample_count = static_cast<VkSampleCountFlagBits>(m_msaa_sample_count);
	desc.blend_state =
		Make_BlendState_PremultipliedAlpha(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT);

	desc.stencil_test_enable = VK_TRUE;
	desc.stencil_state = Make_StencilState(VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_COMPARE_OP_ALWAYS, 0x0);
	Create_GraphicsPipeline(m_p_device, desc, &m_pipelines[static_cast<int>(ProgramId::Creation)]);
}

void RenderInterface_VK::Create_Pipeline_Passthrough()
{
	RMLUI_ZoneScopedN("Vulkan - Create_Pipeline_Passthrough");

	const VkColorComponentFlags kWriteAll = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	PipelineStateDesc desc = {};
	desc.p_shader_vertex = m_shaders[static_cast<int>(eVKShaderID::shader_vert_passthrough)];
	desc.p_shader_fragment = m_shaders[static_cast<int>(eVKShaderID::shader_frag_passthrough)];
	desc.p_pipeline_layout = m_p_pipeline_layout_texture;
	desc.blend_state = Make_BlendState_PremultipliedAlpha(kWriteAll);
	desc.stencil_test_enable = VK_TRUE;

	// final composite to the swapchain (single-sample, has the main depth-stencil attachment)
	desc.p_render_pass = m_p_render_pass_swapchain;
	desc.sample_count = VK_SAMPLE_COUNT_1_BIT;
	desc.stencil_state = Make_StencilState(VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_COMPARE_OP_EQUAL, 0x0);
	Create_GraphicsPipeline(m_p_device, desc, &m_pipelines[static_cast<int>(ProgramId::Passthrough)]);

	// drawing the resolved postprocess texture onto an MSAA layer
	desc.p_render_pass = m_p_render_pass_layer;
	desc.sample_count = static_cast<VkSampleCountFlagBits>(m_msaa_sample_count);
	desc.stencil_state = Make_StencilState(VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_COMPARE_OP_ALWAYS, 0x0);
	Create_GraphicsPipeline(m_p_device, desc, &m_pipelines[static_cast<int>(ProgramId::Passthrough_MSAA)]);

	desc.stencil_state = Make_StencilState(VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_COMPARE_OP_EQUAL, 0x0);
	Create_GraphicsPipeline(m_p_device, desc, &m_pipelines[static_cast<int>(ProgramId::Passthrough_MSAA_Equal)]);

	// postprocess render passes have no depth-stencil attachment, so the stencil test is disabled there
	desc.p_render_pass = m_p_render_pass_postprocess;
	desc.sample_count = VK_SAMPLE_COUNT_1_BIT;
	desc.stencil_test_enable = VK_FALSE;
	Create_GraphicsPipeline(m_p_device, desc, &m_pipelines[static_cast<int>(ProgramId::Passthrough_NoDepthStencil)]);

	desc.blend_state = Make_BlendState_Opacity();
	desc.is_dynamic_blend_constants = true;
	Create_GraphicsPipeline(m_p_device, desc, &m_pipelines[static_cast<int>(ProgramId::Passthrough_Opacity)]);
}

void RenderInterface_VK::Create_Pipeline_Passthrough_NoBlend()
{
	RMLUI_ZoneScopedN("Vulkan - Create_Pipeline_Passthrough_NoBlend");

	PipelineStateDesc desc = {};
	desc.p_shader_vertex = m_shaders[static_cast<int>(eVKShaderID::shader_vert_passthrough)];
	desc.p_shader_fragment = m_shaders[static_cast<int>(eVKShaderID::shader_frag_passthrough)];
	desc.p_pipeline_layout = m_p_pipeline_layout_texture;
	desc.blend_state = Make_BlendState_Disabled();

	// for MSAA RT (composite with 'Replace' blend mode)
	desc.p_render_pass = m_p_render_pass_layer;
	desc.sample_count = static_cast<VkSampleCountFlagBits>(m_msaa_sample_count);
	desc.stencil_test_enable = VK_TRUE;
	desc.stencil_state = Make_StencilState(VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_COMPARE_OP_ALWAYS, 0xff);
	Create_GraphicsPipeline(m_p_device, desc, &m_pipelines[static_cast<int>(ProgramId::Passthrough_NoBlend)]);

	// for RT that's not MSAA (postprocess framebuffers)
	desc.p_render_pass = m_p_render_pass_postprocess;
	desc.sample_count = VK_SAMPLE_COUNT_1_BIT;
	desc.stencil_test_enable = VK_FALSE;
	Create_GraphicsPipeline(m_p_device, desc, &m_pipelines[static_cast<int>(ProgramId::Passthrough_NoBlendAndNoMSAA)]);
}

void RenderInterface_VK::Create_Pipeline_ColorMatrix()
{
	RMLUI_ZoneScopedN("Vulkan - Create_Pipeline_ColorMatrix");

	PipelineStateDesc desc = {};
	desc.p_shader_vertex = m_shaders[static_cast<int>(eVKShaderID::shader_vert_passthrough)];
	desc.p_shader_fragment = m_shaders[static_cast<int>(eVKShaderID::shader_frag_color_matrix)];
	desc.p_pipeline_layout = m_p_pipeline_layout_texture_effect;
	desc.p_render_pass = m_p_render_pass_postprocess;
	desc.sample_count = VK_SAMPLE_COUNT_1_BIT;
	desc.blend_state = Make_BlendState_Disabled();
	desc.stencil_test_enable = VK_FALSE;

	Create_GraphicsPipeline(m_p_device, desc, &m_pipelines[static_cast<int>(ProgramId::ColorMatrix)]);
}

void RenderInterface_VK::Create_Pipeline_BlendMask()
{
	RMLUI_ZoneScopedN("Vulkan - Create_Pipeline_BlendMask");

	PipelineStateDesc desc = {};
	desc.p_shader_vertex = m_shaders[static_cast<int>(eVKShaderID::shader_vert_passthrough)];
	desc.p_shader_fragment = m_shaders[static_cast<int>(eVKShaderID::shader_frag_blend_mask)];
	desc.p_pipeline_layout = m_p_pipeline_layout_blend_mask;
	desc.p_render_pass = m_p_render_pass_postprocess;
	desc.sample_count = VK_SAMPLE_COUNT_1_BIT;
	desc.blend_state = Make_BlendState_Disabled();
	desc.stencil_test_enable = VK_FALSE;

	Create_GraphicsPipeline(m_p_device, desc, &m_pipelines[static_cast<int>(ProgramId::BlendMask)]);
}

void RenderInterface_VK::Create_Pipeline_Blur()
{
	RMLUI_ZoneScopedN("Vulkan - Create_Pipeline_Blur");

	PipelineStateDesc desc = {};
	desc.p_shader_vertex = m_shaders[static_cast<int>(eVKShaderID::shader_vert_blur)];
	desc.p_shader_fragment = m_shaders[static_cast<int>(eVKShaderID::shader_frag_blur)];
	desc.p_pipeline_layout = m_p_pipeline_layout_texture_effect;
	desc.p_render_pass = m_p_render_pass_postprocess;
	desc.sample_count = VK_SAMPLE_COUNT_1_BIT;
	desc.blend_state = Make_BlendState_Disabled();
	desc.stencil_test_enable = VK_FALSE;

	Create_GraphicsPipeline(m_p_device, desc, &m_pipelines[static_cast<int>(ProgramId::Blur)]);
}

void RenderInterface_VK::Create_Pipeline_DropShadow()
{
	RMLUI_ZoneScopedN("Vulkan - Create_Pipeline_DropShadow");

	PipelineStateDesc desc = {};
	desc.p_shader_vertex = m_shaders[static_cast<int>(eVKShaderID::shader_vert_passthrough)];
	desc.p_shader_fragment = m_shaders[static_cast<int>(eVKShaderID::shader_frag_drop_shadow)];
	desc.p_pipeline_layout = m_p_pipeline_layout_texture_effect;
	desc.p_render_pass = m_p_render_pass_postprocess;
	desc.sample_count = VK_SAMPLE_COUNT_1_BIT;
	desc.blend_state = Make_BlendState_Disabled();
	desc.stencil_test_enable = VK_FALSE;

	Create_GraphicsPipeline(m_p_device, desc, &m_pipelines[static_cast<int>(ProgramId::DropShadow)]);
}

void RenderInterface_VK::Destroy_Pipelines() noexcept
{
	RMLUI_ASSERT(m_p_device && "must exist here");

	if (m_p_device == nullptr)
		return;

	for (auto& p_pipeline : m_pipelines)
	{
		if (p_pipeline)
		{
			vkDestroyPipeline(m_p_device, p_pipeline, nullptr);
			p_pipeline = nullptr;
		}
	}
}

void RenderInterface_VK::Create_SwapchainFrameBuffers(const VkExtent2D& real_render_image_size) noexcept
{
	RMLUI_VK_ASSERTMSG(m_p_render_pass_swapchain, "you must create the swapchain VkRenderPass before calling this method");
	RMLUI_VK_ASSERTMSG(m_p_device, "you must have a valid VkDevice here");

	Create_SwapchainImageViews();
	Create_DepthStencilImage();
	Create_DepthStencilImageViews();

	m_swapchain_frame_buffers.resize(m_swapchain_image_views.size());

	Rml::Array<VkImageView, 2> attachments;

	VkFramebufferCreateInfo info = {};
	info.sType = VkStructureType::VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	info.pNext = nullptr;
	info.renderPass = m_p_render_pass_swapchain;
	info.attachmentCount = static_cast<uint32_t>(attachments.size());
	info.pAttachments = attachments.data();
	info.width = real_render_image_size.width;
	info.height = real_render_image_size.height;
	info.layers = 1;

	int index = 0;
	VkResult status = VkResult::VK_SUCCESS;

	attachments[1] = m_texture_depthstencil.Get_ImageView();

	for (auto p_view : m_swapchain_image_views)
	{
		attachments[0] = p_view;

		status = vkCreateFramebuffer(m_p_device, &info, nullptr, &m_swapchain_frame_buffers[index]);

		RMLUI_VK_ASSERTMSG(status == VkResult::VK_SUCCESS, "failed to vkCreateFramebuffer");

		++index;
	}
}

void RenderInterface_VK::Create_SwapchainImages() noexcept
{
	RMLUI_VK_ASSERTMSG(m_p_device, "[Vulkan] you must initialize VkDevice before calling this method");
	RMLUI_VK_ASSERTMSG(m_p_swapchain, "[Vulkan] you must initialize VkSwapchainKHR before calling this method");

	uint32_t count = 0;
	auto status = vkGetSwapchainImagesKHR(m_p_device, m_p_swapchain, &count, nullptr);

	RMLUI_VK_ASSERTMSG(status == VK_SUCCESS, "[Vulkan] failed to vkGetSwapchainImagesKHR (get count)");

	m_swapchain_images.resize(count);

	status = vkGetSwapchainImagesKHR(m_p_device, m_p_swapchain, &count, m_swapchain_images.data());

	RMLUI_VK_ASSERTMSG(status == VK_SUCCESS, "[Vulkan] failed to vkGetSwapchainImagesKHR (filling vector)");

	// one render-finished semaphore per swapchain image (see the header's comment on m_semaphores_finished_render);
	// any previous set was destroyed alongside the previous swapchain's framebuffers
	RMLUI_VK_ASSERTMSG(m_semaphores_finished_render.empty(), "must be destroyed with the previous swapchain's resources");

	m_semaphores_finished_render.resize(m_swapchain_images.size());

	for (auto& p_semaphore : m_semaphores_finished_render)
	{
		VkSemaphoreCreateInfo info_semaphore = {};
		info_semaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		info_semaphore.pNext = nullptr;
		info_semaphore.flags = 0;

		status = vkCreateSemaphore(m_p_device, &info_semaphore, nullptr, &p_semaphore);

		RMLUI_VK_ASSERTMSG(status == VK_SUCCESS, "failed to vkCreateSemaphore (render finished)");
	}
}

void RenderInterface_VK::Create_SwapchainImageViews() noexcept
{
	Create_SwapchainImages();

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
	RMLUI_VK_ASSERTMSG(m_texture_depthstencil.Get_Image() == nullptr, "you should delete texture before create it");

	m_manager_texture.Alloc_Texture(&m_texture_depthstencil, Rml::Vector2i(m_width, m_height), Get_SupportedDepthFormat(), VK_SAMPLE_COUNT_1_BIT, true
#ifdef RMLUI_VK_DEBUG
		,
		"main depth-stencil"
#endif
	);
}

void RenderInterface_VK::Create_DepthStencilImageViews() noexcept
{
	// the depth-stencil image view is created by the texture manager inside Alloc_Texture (see Create_DepthStencilImage)
}

void RenderInterface_VK::Create_ResourcesDependentOnSize(const VkExtent2D& real_render_image_size) noexcept
{
	m_viewport.height = static_cast<float>(real_render_image_size.height);
	m_viewport.width = static_cast<float>(real_render_image_size.width);
	m_viewport.minDepth = 0.0f;
	m_viewport.maxDepth = 1.0f;
	m_viewport.x = 0.0f;
	m_viewport.y = 0.0f;

	m_scissor_original.extent.width = real_render_image_size.width;
	m_scissor_original.extent.height = real_render_image_size.height;
	m_scissor_original.offset.x = 0;
	m_scissor_original.offset.y = 0;

	m_scissor = Rml::Rectanglei::FromCorners(Rml::Vector2i(0, 0),
		Rml::Vector2i(static_cast<int>(real_render_image_size.width), static_cast<int>(real_render_image_size.height)));

	m_projection = Rml::Matrix4f::ProjectOrtho(0.0f, static_cast<float>(m_width), static_cast<float>(m_height), 0.0f, -10000, 10000);

	// https://matthewwellings.com/blog/the-new-vulkan-coordinate-system/
	Rml::Matrix4f correction_matrix;
	correction_matrix.SetColumns(Rml::Vector4f(1.0f, 0.0f, 0.0f, 0.0f), Rml::Vector4f(0.0f, -1.0f, 0.0f, 0.0f), Rml::Vector4f(0.0f, 0.0f, 0.5f, 0.0f),
		Rml::Vector4f(0.0f, 0.0f, 0.5f, 1.0f));

	m_projection = correction_matrix * m_projection;

	SetTransform(nullptr);

	// Render passes and pipelines depend on the swapchain format and the MSAA sample count (not on the framebuffer
	// size), so they are created exactly once here, at the end of the first successful swapchain creation, and are
	// destroyed in Destroy_Resources at Shutdown.
	if (m_p_render_pass_layer == nullptr)
	{
		Create_RenderPasses();
		Create_Pipelines();
	}

	Create_SwapchainFrameBuffers(real_render_image_size);
}

void RenderInterface_VK::DestroyResourcesDependentOnSize() noexcept
{
	DestroySwapchainFrameBuffers();
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
	// the render-finished semaphores are tied to the swapchain images (one per image); the device is idle at this
	// point (SetViewport/Shutdown drain it first), so destroying them here is safe
	for (auto p_semaphore : m_semaphores_finished_render)
	{
		vkDestroySemaphore(m_p_device, p_semaphore, nullptr);
	}

	m_semaphores_finished_render.clear();

	DestroySwapchainImageViews();

	Destroy_Texture(&m_texture_depthstencil);

	for (auto p_frame_buffer : m_swapchain_frame_buffers)
	{
		vkDestroyFramebuffer(m_p_device, p_frame_buffer, nullptr);
	}

	m_swapchain_frame_buffers.clear();
}

void RenderInterface_VK::Destroy_Texture(TextureHandleType* p_texture) noexcept
{
	RMLUI_VK_ASSERTMSG(m_p_allocator, "you must have initialized VmaAllocator");
	RMLUI_VK_ASSERTMSG(m_p_device, "you must have initialized VkDevice");

	// Used for the by-value main depth-stencil texture (m_texture_depthstencil). Unlike heap-allocated texture handles
	// it is reused across recreations (SetViewport), so it must not go through TextureMemoryManager::Free_Texture:
	// that one marks the handle as destroyed, which is only valid once per handle lifetime. The final Mark_Destroyed
	// for the member texture happens in Destroy_Resources. The depth-stencil texture never owns a descriptor set.
	if (p_texture && p_texture->Get_Image())
	{
		if (p_texture->Get_ImageView())
		{
			vkDestroyImageView(m_p_device, p_texture->Get_ImageView(), nullptr);
			p_texture->Set_ImageView(nullptr);
		}

		vmaDestroyImage(m_p_allocator, p_texture->Get_Image(), p_texture->Get_Allocation());

		p_texture->Set_Image(nullptr);
		p_texture->Set_Allocation(nullptr);
		p_texture->Set_Layout(VK_IMAGE_LAYOUT_UNDEFINED);
	}
}

void RenderInterface_VK::Destroy_Geometries() noexcept
{
	for (auto& pair : m_pending_for_deletion_geometries)
	{
		Free_Geometry(pair.first);
	}

	m_pending_for_deletion_geometries.clear();
}

void RenderInterface_VK::Destroy_Textures() noexcept
{
	for (auto& pair : m_pending_for_deletion_textures)
	{
		Free_Texture(pair.first);
	}

	m_pending_for_deletion_textures.clear();
}

void RenderInterface_VK::Free_Geometry(GeometryHandleType* p_handle)
{
	RMLUI_ZoneScopedN("Vulkan - Free_Geometry");
	RMLUI_ASSERTMSG(p_handle, "invalid handle");

	if (p_handle)
	{
		m_manager_buffer.Free_Geometry(p_handle);
		delete p_handle;
	}
}

void RenderInterface_VK::Free_Texture(TextureHandleType* p_handle)
{
	RMLUI_ZoneScopedN("Vulkan - Free_Texture");
	RMLUI_ASSERTMSG(p_handle, "must be valid!");

	if (p_handle)
	{
#ifdef RMLUI_VK_DEBUG
		Rml::Log::Message(Rml::Log::Type::LT_DEBUG, "[Vulkan] Destroyed texture: [%s]", p_handle->Get_ResourceName().c_str());
#endif

		// the manager marks the handle as destroyed inside Free_Texture
		m_manager_texture.Free_Texture(p_handle);
		delete p_handle;
	}
}

void RenderInterface_VK::Update_PendingForDeletion_Geometries() noexcept
{
	RMLUI_ZoneScopedN("Vulkan - Update_PendingForDeletion_Geometries");

	for (auto it = m_pending_for_deletion_geometries.begin(); it != m_pending_for_deletion_geometries.end();)
	{
		// free only once a full swapchain cycle passed, i.e. when the GPU has finished every frame that could reference it
		if (m_frame_counter - it->second >= uint64_t(kSwapchainBackBufferCount))
		{
			Free_Geometry(it->first);
			it = m_pending_for_deletion_geometries.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void RenderInterface_VK::Update_PendingForDeletion_Textures() noexcept
{
	RMLUI_ZoneScopedN("Vulkan - Update_PendingForDeletion_Textures");

	for (auto it = m_pending_for_deletion_textures.begin(); it != m_pending_for_deletion_textures.end();)
	{
		if (m_frame_counter - it->second >= uint64_t(kSwapchainBackBufferCount))
		{
			Free_Texture(it->first);
			it = m_pending_for_deletion_textures.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void RenderInterface_VK::Wait() noexcept
{
	RMLUI_VK_ASSERTMSG(m_p_device, "you must initialize device");
	RMLUI_VK_ASSERTMSG(m_p_swapchain, "you must initialize swapchain");

	constexpr uint64_t kMaxUint64 = std::numeric_limits<uint64_t>::max();

	m_semaphore_index_previous = m_semaphore_index;
	m_semaphore_index = ((m_semaphore_index + 1) % kSwapchainBackBufferCount);

	// slots that were never submitted have an unsignaled fence; waiting on those would deadlock
	if (m_submitted_fences[m_semaphore_index_previous])
	{
		VkResult status = vkWaitForFences(m_p_device, 1, &m_executed_fences[m_semaphore_index_previous], VK_TRUE, kMaxUint64);
		RMLUI_VK_ASSERTMSG(status == VkResult::VK_SUCCESS, "failed to vkWaitForFences (see status)");

		status = vkResetFences(m_p_device, 1, &m_executed_fences[m_semaphore_index_previous]);
		RMLUI_VK_ASSERTMSG(status == VkResult::VK_SUCCESS, "failed to vkResetFences (see status)");

		m_submitted_fences[m_semaphore_index_previous] = false;
	}

	// the acquire happens AFTER the fence wait on purpose: the wait proves the GPU consumed this slot's
	// image-available semaphore (its last wait completed), so re-signaling it here is always safe
	auto status = vkAcquireNextImageKHR(m_p_device, m_p_swapchain, kMaxUint64, m_semaphores_image_available[m_semaphore_index_previous], nullptr,
		&m_image_index);
	RMLUI_VK_ASSERTMSG(status == VkResult::VK_SUCCESS, "failed to vkAcquireNextImageKHR (see status)");
}

void RenderInterface_VK::Submit() noexcept
{
	RMLUI_VK_ASSERTMSG(m_image_index < m_semaphores_finished_render.size(), "image index out of range of the per-image semaphores");

	const VkSemaphore p_semaphores_wait[] = {m_semaphores_image_available[m_semaphore_index_previous]};
	const VkSemaphore p_semaphores_signal[] = {m_semaphores_finished_render[m_image_index]};

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

	m_submitted_fences[m_semaphore_index] = true;
}

void RenderInterface_VK::Present() noexcept
{
	VkPresentInfoKHR info = {};

	info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	info.pNext = nullptr;
	info.waitSemaphoreCount = 1;
	info.pWaitSemaphores = &(m_semaphores_finished_render[m_image_index]);
	info.swapchainCount = 1;
	info.pSwapchains = &m_p_swapchain;
	info.pImageIndices = &m_image_index;
	info.pResults = nullptr;

	VkResult status = vkQueuePresentKHR(m_p_queue_present, &info);

	if (status != VK_SUCCESS)
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

void RenderInterface_VK::Flush() noexcept
{
	RMLUI_ZoneScopedN("Vulkan - Flush");

	// full GPU drain, the analog of the DX12 renderer's Flush (signal + wait on the current slot's fence)
	auto status = vkDeviceWaitIdle(m_p_device);
	RMLUI_VK_ASSERTMSG(status == VkResult::VK_SUCCESS, "failed to vkDeviceWaitIdle");
}

void RenderInterface_VK::EndActiveRenderPass() noexcept
{
	if (m_p_active_render_pass)
	{
		RMLUI_VK_ASSERTMSG(m_p_current_command_buffer, "must be recording a command buffer");
		vkCmdEndRenderPass(m_p_current_command_buffer);
		m_p_active_render_pass = nullptr;
		m_p_active_framebuffer = nullptr;
	}
}

void RenderInterface_VK::BindRenderTarget(const Gfx::FramebufferData& framebuffer, bool depth_included)
{
	(void)depth_included;
	RMLUI_ZoneScopedN("Vulkan - BindRenderTarget");

	if (m_p_active_framebuffer == framebuffer.Get_Framebuffer() && m_p_active_render_pass != nullptr)
		return;

	EndActiveRenderPass();

	// the render pass was stored on the framebuffer when it was created: layer framebuffers use the layer pass (color +
	// shared depth-stencil), postprocess framebuffers use the postprocess pass (color only) — the Vulkan analog of
	// DX12's OMSetRenderTargets with/without DSV.
	VkRenderPass p_render_pass = framebuffer.Get_RenderPass();

	RMLUI_ASSERTMSG(p_render_pass, "the framebuffer has no render pass (the shared depth-stencil block is not a render target)");
	RMLUI_ASSERTMSG(depth_included == (p_render_pass == m_p_render_pass_layer),
		"depth_included must match the framebuffer kind (layers always have depth, postprocess framebuffers never do)");

	VkRenderPassBeginInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	info.pNext = nullptr;
	info.renderPass = p_render_pass;
	info.framebuffer = framebuffer.Get_Framebuffer();
	info.renderArea.offset.x = 0;
	info.renderArea.offset.y = 0;
	info.renderArea.extent.width = static_cast<uint32_t>(framebuffer.Get_Width());
	info.renderArea.extent.height = static_cast<uint32_t>(framebuffer.Get_Height());
	// contents are preserved between passes (load op is LOAD); clears ride the load ops of the clear-variant passes
	// (BindRenderTarget_Clear / Clear), only subregion clears use vkCmdClearAttachments
	info.pClearValues = nullptr;
	info.clearValueCount = 0;

	vkCmdBeginRenderPass(m_p_current_command_buffer, &info, VkSubpassContents::VK_SUBPASS_CONTENTS_INLINE);

	m_p_active_render_pass = p_render_pass;
	m_p_active_framebuffer = framebuffer.Get_Framebuffer();
}

void RenderInterface_VK::BindRenderTarget_Clear(const Gfx::FramebufferData& framebuffer, bool clear_depth_stencil)
{
	RMLUI_ZoneScopedN("Vulkan - BindRenderTarget_Clear");

	// only layer framebuffers have clear-variant render passes (compatible with the load variant, so the same
	// framebuffer object is used); the color attachment is cleared to transparent black on begin (what the DX12
	// renderer does when a layer is pushed), or, when clear_depth_stencil is set, the depth-stencil attachment is
	// cleared instead and the color contents are preserved (clip-mask stencil clear at the start of a fresh pass)
	RMLUI_ASSERTMSG(framebuffer.Get_RenderPass() == m_p_render_pass_layer, "only layer framebuffers have a clear-variant render pass");

	EndActiveRenderPass();

	const VkRenderPass p_render_pass = clear_depth_stencil ? m_p_render_pass_layer_clear_ds : m_p_render_pass_layer_clear;

	VkClearValue clear_values[2] = {};
	clear_values[0].color = {{0.0f, 0.0f, 0.0f, 0.0f}};
	clear_values[1].depthStencil = {RMLUI_RENDER_BACKEND_FIELD_CLEAR_VALUE_DEPTHSTENCIL_DEPTH_VALUE,
		RMLUI_RENDER_BACKEND_FIELD_CLEAR_VALUE_DEPTHSTENCIL_STENCIL_VALUE};

	VkRenderPassBeginInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	info.pNext = nullptr;
	info.renderPass = p_render_pass;
	info.framebuffer = framebuffer.Get_Framebuffer();
	info.renderArea.offset.x = 0;
	info.renderArea.offset.y = 0;
	info.renderArea.extent.width = static_cast<uint32_t>(framebuffer.Get_Width());
	info.renderArea.extent.height = static_cast<uint32_t>(framebuffer.Get_Height());
	info.pClearValues = clear_values;
	info.clearValueCount = 2;

	vkCmdBeginRenderPass(m_p_current_command_buffer, &info, VkSubpassContents::VK_SUBPASS_CONTENTS_INLINE);

	m_p_active_render_pass = p_render_pass;
	m_p_active_framebuffer = framebuffer.Get_Framebuffer();
}

void RenderInterface_VK::TransitionImageLayout(TextureHandleType* p_texture, VkImageLayout new_layout, VkImageAspectFlags aspect_mask) noexcept
{
	RMLUI_VK_ASSERTMSG(p_texture, "you must pass a valid texture");

	if (p_texture == nullptr)
		return;

	const VkImageLayout old_layout = p_texture->Get_Layout();

	if (old_layout == new_layout)
		return;

	// image layout transitions must be recorded outside of a render pass instance
	EndActiveRenderPass();

	VkAccessFlags src_access = 0;
	VkAccessFlags dst_access = 0;
	VkPipelineStageFlags src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	VkPipelineStageFlags dst_stage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

	switch (old_layout)
	{
	case VK_IMAGE_LAYOUT_UNDEFINED:
		src_access = 0;
		src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		break;
	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		src_access = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
		src_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		break;
	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		src_access = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		src_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		break;
	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		src_access = VK_ACCESS_SHADER_READ_BIT;
		src_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		break;
	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		src_access = VK_ACCESS_TRANSFER_READ_BIT;
		src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		break;
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		src_access = VK_ACCESS_TRANSFER_WRITE_BIT;
		src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		break;
	default: RMLUI_ASSERTMSG(!"unhandled source image layout", "unhandled source image layout"); break;
	}

	switch (new_layout)
	{
	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		dst_access = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
		dst_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		break;
	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		dst_access = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		dst_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		break;
	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		dst_access = VK_ACCESS_SHADER_READ_BIT;
		dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		break;
	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		dst_access = VK_ACCESS_TRANSFER_READ_BIT;
		dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		break;
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		dst_access = VK_ACCESS_TRANSFER_WRITE_BIT;
		dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		break;
	default: RMLUI_ASSERTMSG(!"unhandled destination image layout", "unhandled destination image layout"); break;
	}

	VkImageMemoryBarrier info_barrier = {};
	info_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	info_barrier.pNext = nullptr;
	info_barrier.oldLayout = old_layout;
	info_barrier.newLayout = new_layout;
	info_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	info_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	info_barrier.image = p_texture->Get_Image();
	info_barrier.subresourceRange.aspectMask = aspect_mask;
	info_barrier.subresourceRange.baseMipLevel = 0;
	info_barrier.subresourceRange.levelCount = 1;
	info_barrier.subresourceRange.baseArrayLayer = 0;
	info_barrier.subresourceRange.layerCount = 1;
	info_barrier.srcAccessMask = src_access;
	info_barrier.dstAccessMask = dst_access;

	vkCmdPipelineBarrier(m_p_current_command_buffer, src_stage, dst_stage, 0, 0, nullptr, 0, nullptr, 1, &info_barrier);

	p_texture->Set_Layout(new_layout);
}

void RenderInterface_VK::BeginFrame()
{
	RMLUI_ZoneScopedN("Vulkan - BeginFrame");

	Wait();

	// the slot's fence just passed, so entries that were retired a full swapchain cycle ago can't be referenced by the
	// GPU anymore
	Update_PendingForDeletion_Geometries();
	Update_PendingForDeletion_Textures();
	m_manager_buffer.Update_PendingForDeletion_Buffers(m_frame_counter);
	Update_PendingForDeletion_DescriptorSets(m_p_device, &m_manager_descriptors, m_frame_counter, false);

	m_command_buffer_ring.OnBeginFrame();
	m_constant_buffer_count_per_frame[m_command_buffer_ring.Get_ActiveFrameIndex()] = 0;

	m_p_current_command_buffer = m_command_buffer_ring.GetCommandBufferForActiveFrame(CommandBufferName::Primary);

	VkCommandBufferBeginInfo info = {};

	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	info.pInheritanceInfo = nullptr;
	info.pNext = nullptr;
	info.flags = VkCommandBufferUsageFlagBits::VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	auto status = vkBeginCommandBuffer(m_p_current_command_buffer, &info);

	RMLUI_VK_ASSERTMSG(status == VkResult::VK_SUCCESS, "failed to vkBeginCommandBuffer");

	m_stencil_ref_value = 0;
	m_is_scissor_was_set = false;
	m_is_stencil_equal = false;
	m_current_clip_operation = -1;

	SetTransform(nullptr);
	UseProgram(ProgramId::None);

	// destroys/recreates the layer framebuffers when the size changed, then pushes the base layer
	m_manager_render_layer.BeginFrame(m_width, m_height);

	// unlike the old renderer no render pass is active yet at this point; the first bind begins the layer render pass
	// in its CLEAR variant so the base layer starts transparent every frame (whether or not the shell calls Clear())
	BindRenderTarget_Clear(m_manager_render_layer.GetTopLayer());

	vkCmdSetViewport(m_p_current_command_buffer, 0, 1, &m_viewport);
	vkCmdSetScissor(m_p_current_command_buffer, 0, 1, &m_scissor_original);
	vkCmdSetStencilReference(m_p_current_command_buffer, VK_STENCIL_FACE_FRONT_AND_BACK, 0);
}

void RenderInterface_VK::EndFrame()
{
	RMLUI_ZoneScopedN("Vulkan - EndFrame");

	if (m_p_current_command_buffer == nullptr)
		return;

	EndActiveRenderPass();

	const Gfx::FramebufferData& fb_active = m_manager_render_layer.GetTopLayer();
	const Gfx::FramebufferData& fb_postprocess = m_manager_render_layer.GetPostprocessPrimary();

	TextureHandleType* p_layer_texture = fb_active.Get_Texture();
	TextureHandleType* p_postprocess_texture = fb_postprocess.Get_Texture();

	RMLUI_ASSERTMSG(p_layer_texture, "can't be, must be a valid texture!");
	RMLUI_ASSERTMSG(p_postprocess_texture, "can't be, must be a valid texture!");

	RMLUI_ASSERTMSG(fb_active.Get_Width() == fb_postprocess.Get_Width(), "must be same otherwise use blitframebuffer!");
	RMLUI_ASSERTMSG(fb_active.Get_Height() == fb_postprocess.Get_Height(), "must be same otherwise use blitframebuffer!");

	// resolve (MSAA) or copy the top UI layer into the single-sampled postprocess primary texture
	TransitionImageLayout(p_layer_texture, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	TransitionImageLayout(p_postprocess_texture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	VkImageSubresourceLayers subresource_layers = {};
	subresource_layers.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresource_layers.mipLevel = 0;
	subresource_layers.baseArrayLayer = 0;
	subresource_layers.layerCount = 1;

	VkOffset3D offset = {};
	VkExtent3D extent = {static_cast<uint32_t>(fb_active.Get_Width()), static_cast<uint32_t>(fb_active.Get_Height()), 1};

	if (m_is_use_msaa)
	{
		VkImageResolve region = {};
		region.srcSubresource = subresource_layers;
		region.srcOffset = offset;
		region.dstSubresource = subresource_layers;
		region.dstOffset = offset;
		region.extent = extent;

		vkCmdResolveImage(m_p_current_command_buffer, p_layer_texture->Get_Image(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			p_postprocess_texture->Get_Image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
	}
	else
	{
		VkImageCopy region = {};
		region.srcSubresource = subresource_layers;
		region.srcOffset = offset;
		region.dstSubresource = subresource_layers;
		region.dstOffset = offset;
		region.extent = extent;

		vkCmdCopyImage(m_p_current_command_buffer, p_layer_texture->Get_Image(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			p_postprocess_texture->Get_Image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
	}

	TransitionImageLayout(p_layer_texture, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	TransitionImageLayout(p_postprocess_texture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	// final composite onto the swapchain image; the render pass clears the swapchain color and the main depth-stencil
	VkClearValue clear_values[2] = {};
	clear_values[0].color = {{RMLUI_RENDER_BACKEND_FIELD_CLEAR_VALUE_RENDERTARGET_COLOR_VAlUE}};
	clear_values[1].depthStencil = {RMLUI_RENDER_BACKEND_FIELD_CLEAR_VALUE_DEPTHSTENCIL_DEPTH_VALUE,
		RMLUI_RENDER_BACKEND_FIELD_CLEAR_VALUE_DEPTHSTENCIL_STENCIL_VALUE};

	VkRenderPassBeginInfo info_pass = {};

	info_pass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	info_pass.pNext = nullptr;
	info_pass.renderPass = m_p_render_pass_swapchain;
	info_pass.framebuffer = m_swapchain_frame_buffers[m_image_index];
	info_pass.pClearValues = clear_values;
	info_pass.clearValueCount = 2;
	info_pass.renderArea.offset.x = 0;
	info_pass.renderArea.offset.y = 0;
	info_pass.renderArea.extent.width = m_width;
	info_pass.renderArea.extent.height = m_height;

	vkCmdBeginRenderPass(m_p_current_command_buffer, &info_pass, VkSubpassContents::VK_SUBPASS_CONTENTS_INLINE);

	m_p_active_render_pass = m_p_render_pass_swapchain;
	m_p_active_framebuffer = m_swapchain_frame_buffers[m_image_index];

	UseProgram(ProgramId::Passthrough);

	BindTexture(p_postprocess_texture, 0);

	DrawFullscreenQuad();

	EndActiveRenderPass();

	TransitionImageLayout(p_postprocess_texture, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	m_manager_render_layer.EndFrame();

	auto status = vkEndCommandBuffer(m_p_current_command_buffer);

	RMLUI_VK_ASSERTMSG(status == VkResult::VK_SUCCESS, "failed to vkEndCommandBuffer");

	Submit();
	Present();

	++m_frame_counter;

	m_p_current_command_buffer = nullptr;
}

void RenderInterface_VK::Clear()
{
	RMLUI_ZoneScopedN("Vulkan - Clear");
	RMLUI_ASSERT(m_p_current_command_buffer);
	RMLUI_ASSERTMSG(m_p_active_render_pass, "a render pass must be active (BeginFrame binds the top layer)");

	if (m_p_current_command_buffer && m_p_active_render_pass)
	{
		const bool is_layer_pass = (m_p_active_render_pass == m_p_render_pass_layer || m_p_active_render_pass == m_p_render_pass_layer_clear ||
			m_p_active_render_pass == m_p_render_pass_layer_clear_all || m_p_active_render_pass == m_p_render_pass_layer_clear_ds);

		// restart the layer pass in its clear-everything variant: the clear rides the attachment load ops instead of
		// an explicit vkCmdClearAttachments command (the clear-after-load best practice, and cheaper on tilers). Safe
		// even when draws were already recorded — a Clear() call discards them anyway. The common shell flow hits
		// this right after BeginFrame, before anything was drawn.
		if (is_layer_pass)
		{
			// note: grab the framebuffer first, EndActiveRenderPass() resets the active fields
			VkFramebuffer p_active_framebuffer = m_p_active_framebuffer;

			EndActiveRenderPass();

			VkClearValue clear_values[2] = {};
			clear_values[0].color = {{0.0f, 0.0f, 0.0f, 0.0f}};
			clear_values[1].depthStencil = {RMLUI_RENDER_BACKEND_FIELD_CLEAR_VALUE_DEPTHSTENCIL_DEPTH_VALUE,
				RMLUI_RENDER_BACKEND_FIELD_CLEAR_VALUE_DEPTHSTENCIL_STENCIL_VALUE};

			VkRenderPassBeginInfo info_pass = {};
			info_pass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			info_pass.pNext = nullptr;
			info_pass.renderPass = m_p_render_pass_layer_clear_all;
			info_pass.framebuffer = p_active_framebuffer;
			info_pass.renderArea.offset.x = 0;
			info_pass.renderArea.offset.y = 0;
			info_pass.renderArea.extent.width = static_cast<uint32_t>(m_width);
			info_pass.renderArea.extent.height = static_cast<uint32_t>(m_height);
			info_pass.pClearValues = clear_values;
			info_pass.clearValueCount = 2;

			vkCmdBeginRenderPass(m_p_current_command_buffer, &info_pass, VkSubpassContents::VK_SUBPASS_CONTENTS_INLINE);

			m_p_active_render_pass = m_p_render_pass_layer_clear_all;
			m_p_active_framebuffer = p_active_framebuffer;

			return;
		}

		VkClearAttachment attaches[2] = {};

		// The DX12 renderer clears the top layer RTV to transparent black and the backbuffer RTV to the configured
		// clear color; mirror that per active pass (in practice the shell calls Clear() with the layer pass active,
		// right after BeginFrame).
		attaches[0].aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		attaches[0].colorAttachment = 0;
		if (m_p_active_render_pass == m_p_render_pass_swapchain)
			attaches[0].clearValue.color = {{RMLUI_RENDER_BACKEND_FIELD_CLEAR_VALUE_RENDERTARGET_COLOR_VAlUE}};
		else
			attaches[0].clearValue.color = {{0.0f, 0.0f, 0.0f, 0.0f}};

		attaches[1].aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		attaches[1].clearValue.depthStencil = {RMLUI_RENDER_BACKEND_FIELD_CLEAR_VALUE_DEPTHSTENCIL_DEPTH_VALUE,
			RMLUI_RENDER_BACKEND_FIELD_CLEAR_VALUE_DEPTHSTENCIL_STENCIL_VALUE};

		VkClearRect clear_rect = {};
		clear_rect.rect.offset.x = 0;
		clear_rect.rect.offset.y = 0;
		clear_rect.rect.extent.width = static_cast<uint32_t>(m_width);
		clear_rect.rect.extent.height = static_cast<uint32_t>(m_height);
		clear_rect.baseArrayLayer = 0;
		clear_rect.layerCount = 1;

		// only the layer and swapchain passes have a depth-stencil attachment; the postprocess pass does not, and
		// Vulkan validation requires the cleared attachment to exist in the active render pass
		const bool has_depth_stencil_attachment = (m_p_active_render_pass != m_p_render_pass_postprocess);

		vkCmdClearAttachments(m_p_current_command_buffer, has_depth_stencil_attachment ? 2u : 1u, attaches, 1, &clear_rect);
	}
}

void RenderInterface_VK::SetViewport(int width, int height)
{
	RMLUI_ZoneScopedN("Vulkan - SetViewport");

	// the device is drained before touching size-dependent resources, like the DX12 renderer's SetViewport (Flush)
	Flush();

	if (width > 0 && height > 0)
	{
		m_width = width;
		m_height = height;
	}

	if (m_p_swapchain)
	{
		// swapchain image views and framebuffers live in the size-dependent resources and must be destroyed BEFORE
		// the swapchain they reference
		DestroyResourcesDependentOnSize();
		Destroy_Swapchain();
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
	Create_ResourcesDependentOnSize(window_extent);
}

bool RenderInterface_VK::IsSwapchainValid()
{
	RMLUI_ZoneScopedN("Vulkan - IsSwapchainValid");
	return m_p_swapchain != nullptr;
}

void RenderInterface_VK::RecreateSwapchain()
{
	RMLUI_ZoneScopedN("Vulkan - RecreateSwapchain");
	SetViewport(m_width, m_height);
}

void RenderInterface_VK::UseProgram(ProgramId id)
{
	RMLUI_ZoneScopedN("Vulkan - UseProgram");
	RMLUI_ASSERTMSG(id < ProgramId::Count, "overflow, too big value for indexing");

	// the DX12 renderer has no early-out for repeated program selection; keep the same behavior (a plain rebind)
	if (id != ProgramId::None)
	{
		RMLUI_VK_ASSERTMSG(m_pipelines[static_cast<int>(id)], "you forgot to initialize or deleted!");
		vkCmdBindPipeline(m_p_current_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelines[static_cast<int>(id)]);
	}

	m_active_program_id = id;
}

void RenderInterface_VK::SetScissor(Rml::Rectanglei region)
{
	RMLUI_ZoneScopedN("Vulkan - SetScissor");

	if (region.Valid() != m_scissor.Valid())
	{
		if (!region.Valid())
		{
			m_is_scissor_was_set = false;

			if (m_p_current_command_buffer)
			{
				vkCmdSetScissor(m_p_current_command_buffer, 0, 1, &m_scissor_original);
			}

			return;
		}
	}

	// Unlike the DX12 renderer there is no vertically_flip handling here: Vulkan framebuffer space in this backend is
	// already top-left-origin (the projection is pre-multiplied by the GL->Vulkan clip correction), so RmlUi
	// rectangles are used as-is.
	if (region.Valid())
	{
		// like the DX12 renderer the clamped rectangle is what gets stored: consumers of m_scissor (blur windows,
		// SaveLayerAsTexture bounds, filter texcoord limits) expect framebuffer-space coordinates
		const int x_min = Rml::Math::Clamp(region.Left(), 0, m_width);
		const int y_min = Rml::Math::Clamp(region.Top(), 0, m_height);
		const int x_max = Rml::Math::Clamp(region.Right(), 0, m_width);
		const int y_max = Rml::Math::Clamp(region.Bottom(), 0, m_height);

		if (m_p_current_command_buffer)
		{
			VkRect2D scissor = {};
			scissor.offset.x = x_min;
			scissor.offset.y = y_min;
			scissor.extent.width = static_cast<uint32_t>(x_max - x_min);
			scissor.extent.height = static_cast<uint32_t>(y_max - y_min);

			vkCmdSetScissor(m_p_current_command_buffer, 0, 1, &scissor);
			m_is_scissor_was_set = true;
		}

		m_scissor.p0 = Rml::Vector2i(x_min, y_min);
		m_scissor.p1 = Rml::Vector2i(x_max, y_max);
	}
	else
	{
		m_scissor = region;
	}
}

void RenderInterface_VK::EnableScissorRegion(bool enable)
{
	RMLUI_ZoneScopedN("Vulkan - EnableScissorRegion");

	if (!enable)
	{
		SetScissor(Rml::Rectanglei::MakeInvalid());
	}
}

void RenderInterface_VK::SetScissorRegion(Rml::Rectanglei region)
{
	RMLUI_ZoneScopedN("Vulkan - SetScissorRegion");

	SetScissor(region);
}

void RenderInterface_VK::SetTransform(const Rml::Matrix4f* transform)
{
	RMLUI_ZoneScopedN("Vulkan - SetTransform");

	m_is_transform_enabled = !!(transform);
	m_constant_buffer_data_transform = (transform ? m_projection * (*transform) : m_projection);
}

void RenderInterface_VK::SubmitTransformUniform(ConstantBufferType& constant_buffer, const Rml::Vector2f& translation)
{
	RMLUI_ZoneScopedN("Vulkan - SubmitTransformUniform");

	std::uint8_t* p_gpu_binding_start = reinterpret_cast<std::uint8_t*>(constant_buffer.m_p_gpu_start_memory_for_binding_data);

	{
		RMLUI_ASSERTMSG(p_gpu_binding_start,
			"your allocated constant buffer must contain a valid pointer of beginning mapping of its GPU buffer. Otherwise you destroyed it!");

		if (p_gpu_binding_start)
		{
			std::uint8_t* p_gpu_binding_offset_to_transform = p_gpu_binding_start + constant_buffer.m_alloc_info.offset;

			std::memcpy(p_gpu_binding_offset_to_transform, m_constant_buffer_data_transform.data(), sizeof(m_constant_buffer_data_transform));
		}
	}

	if (p_gpu_binding_start)
	{
		std::uint8_t* p_gpu_binding_offset_to_translate =
			p_gpu_binding_start + (constant_buffer.m_alloc_info.offset + sizeof(m_constant_buffer_data_transform));

		std::memcpy(p_gpu_binding_offset_to_translate, &translation.x, sizeof(translation));
	}
}

RenderInterface_VK::ConstantBufferType* RenderInterface_VK::Get_ConstantBuffer(uint32_t current_back_buffer_index)
{
	RMLUI_ZoneScopedN("Vulkan - Get_ConstantBuffer");
	RMLUI_ASSERTMSG(current_back_buffer_index != uint32_t(-1), "invalid index!");

	size_t max_index = RMLUI_RENDER_BACKEND_FIELD_PREALLOCATED_CONSTANTBUFFERS - 1;

	if (m_constantbuffers[current_back_buffer_index].size() > RMLUI_RENDER_BACKEND_FIELD_PREALLOCATED_CONSTANTBUFFERS)
		max_index = m_constantbuffers[current_back_buffer_index].size() - 1;

	const size_t current_constant_buffer_index = m_constant_buffer_count_per_frame[current_back_buffer_index];
	if (current_constant_buffer_index > max_index)
	{
		// resizing...
		for (auto& vec : m_constantbuffers)
		{
			vec.emplace_back(ConstantBufferType());
		}

#ifdef RMLUI_VK_DEBUG
		Rml::Log::Message(Rml::Log::Type::LT_DEBUG, "[Vulkan] allocated new constant buffer instance for frame[%d], current size of storage[%zu]",
			current_back_buffer_index, m_constantbuffers.at(current_back_buffer_index).size());
#endif
	}

	auto& deque_cbs = m_constantbuffers.at(current_back_buffer_index);

	ConstantBufferType* p_result = &deque_cbs[current_constant_buffer_index];
	if (p_result->m_alloc_info.buffer_index == -1)
	{
		p_result->m_alloc_info = m_manager_buffer.Alloc_ConstantBuffer(p_result, kAllocationSizeMax_ConstantBuffer);
	}

	++m_constant_buffer_count_per_frame[current_back_buffer_index];

	return p_result;
}

Rml::CompiledGeometryHandle RenderInterface_VK::CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices)
{
	RMLUI_ZoneScopedN("Vulkan - CompileGeometry");

	GeometryHandleType* p_handle = new GeometryHandleType();

	if (p_handle)
	{
		m_manager_buffer.Alloc_Vertex(vertices.data(), static_cast<int>(vertices.size()), sizeof(Rml::Vertex), p_handle);
		m_manager_buffer.Alloc_Index(indices.data(), static_cast<int>(indices.size()), sizeof(int), p_handle);

		p_handle->Set_HistoryBackBufferFrameIndex(static_cast<int>(m_command_buffer_ring.Get_ActiveFrameIndex()));
	}

	return reinterpret_cast<Rml::CompiledGeometryHandle>(p_handle);
}

void RenderInterface_VK::RenderGeometry(Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation, Rml::TextureHandle texture)
{
	RMLUI_ZoneScopedN("Vulkan - RenderGeometry");

	if (m_p_current_command_buffer == nullptr)
		return;

	GeometryHandleType* p_handle_geometry = reinterpret_cast<GeometryHandleType*>(geometry);
	TextureHandleType* p_handle_texture{};

	RMLUI_ASSERTMSG(p_handle_geometry, "expected valid pointer!");

	ConstantBufferType* p_constant_buffer{};

	if (texture == TexturePostprocess)
	{
		// leave the bound texture and the used program unchanged; the geometry carries an override constant buffer
		// (fullscreen postprocess draws)
		if (p_handle_geometry->Get_ConstantBuffer())
		{
			p_constant_buffer = p_handle_geometry->Get_ConstantBuffer();
		}
	}
	else if (texture)
	{
		p_handle_texture = reinterpret_cast<TextureHandleType*>(texture);
		RMLUI_ASSERTMSG(p_handle_texture, "expected valid pointer!");

		if (m_is_stencil_enabled)
		{
			if (m_is_stencil_equal)
			{
				UseProgram(ProgramId::Texture_Stencil_Equal);
			}
			else
			{
				UseProgram(ProgramId::Texture_Stencil_Always);
			}
		}
		else
		{
			UseProgram(ProgramId::Texture_Stencil_Disabled);
		}

		if (p_handle_geometry->Get_ConstantBuffer() == nullptr)
		{
			p_constant_buffer = Get_ConstantBuffer(m_command_buffer_ring.Get_ActiveFrameIndex());
		}
		else
		{
			p_constant_buffer = p_handle_geometry->Get_ConstantBuffer();
		}

		SubmitTransformUniform(*p_constant_buffer, translation);

		if (texture != TextureEnableWithoutBinding)
		{
			BindTexture(p_handle_texture, 1);
		}
	}
	else
	{
		if (m_current_clip_operation == -1)
		{
			if (m_is_stencil_enabled)
			{
				if (m_is_stencil_equal)
				{
					UseProgram(ProgramId::Color_Stencil_Equal);
				}
				else
				{
					UseProgram(ProgramId::Color_Stencil_Always);
				}
			}
			else
			{
				UseProgram(ProgramId::Color_Stencil_Disabled);
			}
		}
		else if (m_current_clip_operation == static_cast<int>(Rml::ClipMaskOperation::Intersect))
		{
			if (m_is_stencil_enabled)
			{
				UseProgram(ProgramId::Color_Stencil_Intersect);
			}
		}
		else if (m_current_clip_operation == static_cast<int>(Rml::ClipMaskOperation::Set))
		{
			if (m_is_stencil_enabled)
			{
				UseProgram(ProgramId::Color_Stencil_Set);
			}
		}
		else if (m_current_clip_operation == static_cast<int>(Rml::ClipMaskOperation::SetInverse))
		{
			if (m_is_stencil_enabled)
			{
				UseProgram(ProgramId::Color_Stencil_SetInverse);
			}
		}
		else
		{
			RMLUI_ASSERT(!"not reached code point, something is missing or corrupted data"[0]);
		}

		if (p_handle_geometry->Get_ConstantBuffer() == nullptr)
		{
			p_constant_buffer = Get_ConstantBuffer(m_command_buffer_ring.Get_ActiveFrameIndex());
		}
		else
		{
			p_constant_buffer = p_handle_geometry->Get_ConstantBuffer();
		}

		SubmitTransformUniform(*p_constant_buffer, translation);
	}

	if (!m_is_scissor_was_set)
	{
		vkCmdSetScissor(m_p_current_command_buffer, 0, 1, &m_scissor_original);
	}

	if (p_constant_buffer)
	{
		RMLUI_VK_PROGRAM_PIPELINE_LAYOUT_LOOKUP(get_pipeline_layout_for_program);
		VkPipelineLayout p_active_layout = get_pipeline_layout_for_program(m_active_program_id);

		// the constant buffer is bound at set 0 for the transform/transform_texture layouts and at set 1 for the
		// texture_effect layout (one dynamic-UBO binding is visible to both shader stages)
		const uint32_t set_index = (p_active_layout == m_p_pipeline_layout_texture_effect) ? 1u : 0u;

		VkDescriptorSet p_set = m_manager_buffer.Get_ConstantBufferDescriptorSetByIndex(p_constant_buffer->m_alloc_info.buffer_index);
		RMLUI_VK_ASSERTMSG(p_set, "must be valid!");

		const uint32_t dynamic_offset = static_cast<uint32_t>(p_constant_buffer->m_alloc_info.offset);

		vkCmdBindDescriptorSets(m_p_current_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, p_active_layout, set_index, 1, &p_set, 1,
			&dynamic_offset);
	}

	VkBuffer p_buffer_vertex = m_manager_buffer.Get_BufferByIndex(p_handle_geometry->Get_InfoVertex().buffer_index);

	RMLUI_VK_ASSERTMSG(p_buffer_vertex, "must be valid!");

	VkDeviceSize vertex_buffer_offset = p_handle_geometry->Get_InfoVertex().offset;
	vkCmdBindVertexBuffers(m_p_current_command_buffer, 0, 1, &p_buffer_vertex, &vertex_buffer_offset);

	VkBuffer p_buffer_index = m_manager_buffer.Get_BufferByIndex(p_handle_geometry->Get_InfoIndex().buffer_index);

	RMLUI_VK_ASSERTMSG(p_buffer_index, "must be valid!");

	vkCmdBindIndexBuffer(m_p_current_command_buffer, p_buffer_index, p_handle_geometry->Get_InfoIndex().offset, VK_INDEX_TYPE_UINT32);

	vkCmdDrawIndexed(m_p_current_command_buffer, p_handle_geometry->Get_NumIndices(), 1, 0, 0, 0);
}

void RenderInterface_VK::ReleaseGeometry(Rml::CompiledGeometryHandle geometry)
{
	RMLUI_ZoneScopedN("Vulkan - ReleaseGeometry");

	GeometryHandleType* p_handle = reinterpret_cast<GeometryHandleType*>(geometry);

	if (p_handle)
	{
		// defer destruction: the geometry may still be referenced by frames currently in flight on the GPU
		m_pending_for_deletion_geometries.push_back({p_handle, m_frame_counter});
	}
}

void RenderInterface_VK::EnableClipMask(bool enable)
{
	RMLUI_ZoneScopedN("Vulkan - EnableClipMask");

	m_is_stencil_enabled = enable;

	if (m_p_current_command_buffer)
	{
		vkCmdSetStencilReference(m_p_current_command_buffer, VK_STENCIL_FACE_FRONT_AND_BACK, enable ? m_stencil_ref_value : 0);
	}
}

void RenderInterface_VK::RenderToClipMask(Rml::ClipMaskOperation mask_operation, Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation)
{
	RMLUI_ZoneScopedN("Vulkan - RenderToClipMask");
	RMLUI_ASSERTMSG(m_is_stencil_enabled, "must be enabled!");

	const bool clear_stencil = (mask_operation == Rml::ClipMaskOperation::Set || mask_operation == Rml::ClipMaskOperation::SetInverse);

	if (clear_stencil)
	{
		const Gfx::FramebufferData& framebuffer = m_manager_render_layer.GetLayer(m_manager_render_layer.GetTopLayerHandle());
		RMLUI_ASSERTMSG(framebuffer.Get_SharedDepthStencilTexture(), "you have to set shared depth stencil texture for layer!");

		// (re)begin the layer pass in its depth-stencil-clearing variant: the stencil clear rides the attachment load
		// op instead of an explicit vkCmdClearAttachments command (the clear-after-load best practice), and the color
		// contents are preserved via LOAD — draws already recorded into the pass are stored at its end, exactly like
		// the DX12 renderer's ClearDepthStencilView, which doesn't disturb the color target either
		BindRenderTarget_Clear(framebuffer, true);
	}

	switch (mask_operation)
	{
	case Rml::ClipMaskOperation::Set:
	{
		m_current_clip_operation = static_cast<int>(Rml::ClipMaskOperation::Set);
		m_stencil_ref_value = 1;
		break;
	}
	case Rml::ClipMaskOperation::SetInverse:
	{
		m_current_clip_operation = static_cast<int>(Rml::ClipMaskOperation::SetInverse);
		m_stencil_ref_value = 0;
		break;
	}
	case Rml::ClipMaskOperation::Intersect:
	{
		m_current_clip_operation = static_cast<int>(Rml::ClipMaskOperation::Intersect);
		m_stencil_ref_value += 1;
		break;
	}
	}

	vkCmdSetStencilReference(m_p_current_command_buffer, VK_STENCIL_FACE_FRONT_AND_BACK, 1);

	RenderGeometry(geometry, translation, {});

	m_is_stencil_equal = true;
	m_current_clip_operation = -1;
	vkCmdSetStencilReference(m_p_current_command_buffer, VK_STENCIL_FACE_FRONT_AND_BACK, m_stencil_ref_value);
}

Rml::LayerHandle RenderInterface_VK::PushLayer()
{
	RMLUI_ZoneScopedN("Vulkan - PushLayer");

	EndActiveRenderPass();

	const Rml::LayerHandle layer_handle = m_manager_render_layer.PushLayer();

	const auto& framebuffer = m_manager_render_layer.GetLayer(layer_handle);

	RMLUI_ASSERTMSG(framebuffer.Get_SharedDepthStencilTexture(), "you have to set shared depth stencil texture for layer!");

	// begins the layer render pass in its CLEAR variant: the color attachment is cleared to transparent black on
	// begin (the DX12 renderer clears the pushed layer's RTV here)
	BindRenderTarget_Clear(framebuffer);

	return layer_handle;
}

void RenderInterface_VK::PopLayer()
{
	RMLUI_ZoneScopedN("Vulkan - PopLayer");

	m_manager_render_layer.PopLayer();
	BindRenderTarget(m_manager_render_layer.GetTopLayer());
}

void RenderInterface_VK::CompositeLayers(Rml::LayerHandle source, Rml::LayerHandle destination, Rml::BlendMode blend_mode,
	Rml::Span<const Rml::CompiledFilterHandle> filters)
{
	RMLUI_ZoneScopedN("Vulkan - CompositeLayers");

	BlitLayerToPostprocessPrimary(source);

	RenderFilters(filters);

	// Note: no Flush() here (mirrors the DX12 renderer, which removed it). Draining the GPU pipeline on every layer
	// composite is a severe stall, and with fence-stamped deferred deletion of geometry/textures/descriptors it is no
	// longer needed as a workaround to hide premature resource recycling.

	if (blend_mode == Rml::BlendMode::Replace)
	{
		UseProgram(ProgramId::Passthrough_NoBlend);
	}
	else
	{
		// since we use msaa render target we should use appropriate version of pipeline
		if (m_is_stencil_equal && m_is_stencil_enabled)
		{
			UseProgram(ProgramId::Passthrough_MSAA_Equal);
		}
		else
		{
			UseProgram(ProgramId::Passthrough_MSAA);
		}
	}

	TextureHandleType* p_primary_texture = m_manager_render_layer.GetPostprocessPrimary().Get_Texture();
	RMLUI_ASSERTMSG(p_primary_texture, "must be valid!");

	TransitionImageLayout(p_primary_texture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	BindRenderTarget(m_manager_render_layer.GetLayer(destination));
	BindTexture(p_primary_texture, 0);

	DrawFullscreenQuad();

	TransitionImageLayout(p_primary_texture, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	// should we set like return blend state as enabled?
	if (blend_mode == Rml::BlendMode::Replace)
	{
		UseProgram(ProgramId::Passthrough);
	}

	// The transition above ended the active render pass (barriers are recorded outside render passes, unlike DX12 where
	// barriers don't disturb the render-target binding), so the top layer must be bound again unconditionally — even
	// when destination == top — otherwise subsequent draws would be recorded without an active render pass.
	BindRenderTarget(m_manager_render_layer.GetTopLayer());
}

void RenderInterface_VK::BlitLayerToPostprocessPrimary(Rml::LayerHandle layer_id)
{
	RMLUI_ZoneScopedN("Vulkan - BlitLayerToPostprocessPrimary");

	const Gfx::FramebufferData& source_framebuffer = m_manager_render_layer.GetLayer(layer_id);
	const Gfx::FramebufferData& destination_framebuffer = m_manager_render_layer.GetPostprocessPrimary();

	TextureHandleType* p_src = source_framebuffer.Get_Texture();
	TextureHandleType* p_dst = destination_framebuffer.Get_Texture();

	RMLUI_ASSERTMSG(p_src, "texture must be presented when you call this method!");
	RMLUI_ASSERTMSG(p_dst, "texture must be presented when you call this method!");

	RMLUI_ASSERTMSG(source_framebuffer.Get_Width() == destination_framebuffer.Get_Width(), "must be same otherwise use blitframebuffer");
	RMLUI_ASSERTMSG(source_framebuffer.Get_Height() == destination_framebuffer.Get_Height(), "must be same otherwise use blitframebuffer");

	if (!m_p_current_command_buffer)
		return;

	EndActiveRenderPass();

	TransitionImageLayout(p_src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	TransitionImageLayout(p_dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	VkImageSubresourceLayers subresource_layers = {};
	subresource_layers.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresource_layers.mipLevel = 0;
	subresource_layers.baseArrayLayer = 0;
	subresource_layers.layerCount = 1;

	VkOffset3D offset = {};
	VkExtent3D extent = {static_cast<uint32_t>(source_framebuffer.Get_Width()), static_cast<uint32_t>(source_framebuffer.Get_Height()), 1};

	if (m_is_use_msaa)
	{
		VkImageResolve region = {};
		region.srcSubresource = subresource_layers;
		region.srcOffset = offset;
		region.dstSubresource = subresource_layers;
		region.dstOffset = offset;
		region.extent = extent;

		vkCmdResolveImage(m_p_current_command_buffer, p_src->Get_Image(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, p_dst->Get_Image(),
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
	}
	else
	{
		VkImageCopy region = {};
		region.srcSubresource = subresource_layers;
		region.srcOffset = offset;
		region.dstSubresource = subresource_layers;
		region.dstOffset = offset;
		region.extent = extent;

		vkCmdCopyImage(m_p_current_command_buffer, p_src->Get_Image(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, p_dst->Get_Image(),
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
	}

	TransitionImageLayout(p_src, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	TransitionImageLayout(p_dst, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
}

void RenderInterface_VK::RenderFilters(Rml::Span<const Rml::CompiledFilterHandle> filter_handles)
{
	RMLUI_ZoneScopedN("Vulkan - RenderFilters");

	for (const Rml::CompiledFilterHandle filter_handle : filter_handles)
	{
		const CompiledFilter& filter = *reinterpret_cast<const CompiledFilter*>(filter_handle);
		const FilterType type = filter.type;

		switch (type)
		{
		case FilterType::Passthrough:
		{
			const Gfx::FramebufferData& source = m_manager_render_layer.GetPostprocessPrimary();
			const Gfx::FramebufferData& destination = m_manager_render_layer.GetPostprocessSecondary();

			TextureHandleType* p_texture_source = source.Get_Texture();
			RMLUI_ASSERTMSG(p_texture_source, "must be valid pointer!");
			TransitionImageLayout(p_texture_source, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

			BindRenderTarget(destination, false);

			const float blend_factor[] = {filter.blend_factor, filter.blend_factor, filter.blend_factor, filter.blend_factor};
			vkCmdSetBlendConstants(m_p_current_command_buffer, blend_factor);

			UseProgram(ProgramId::Passthrough_Opacity);

			BindTexture(p_texture_source, 0);
			DrawFullscreenQuad();

			TransitionImageLayout(p_texture_source, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

			m_manager_render_layer.SwapPostprocessPrimarySecondary();

			break;
		}
		case FilterType::Blur:
		{
			const Gfx::FramebufferData& source = m_manager_render_layer.GetPostprocessPrimary();
			const Gfx::FramebufferData& temp = m_manager_render_layer.GetPostprocessSecondary();

			RenderBlur(filter.sigma, source, temp, m_scissor);
			break;
		}
		case FilterType::DropShadow:
		{
			UseProgram(ProgramId::DropShadow);

			const Gfx::FramebufferData& primary = m_manager_render_layer.GetPostprocessPrimary();
			const Gfx::FramebufferData& secondary = m_manager_render_layer.GetPostprocessSecondary();

			TransitionImageLayout(primary.Get_Texture(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

			BindRenderTarget(secondary, false);
			BindTexture(primary.Get_Texture(), 0);

			ConstantBufferType* p_cb_dropshadow = Get_ConstantBuffer(m_command_buffer_ring.Get_ActiveFrameIndex());

			RMLUI_ASSERTMSG(p_cb_dropshadow, "failed to obtain constant buffer for drop shadow");

			struct CBV_DropShadow {
				Rml::Vector2f uv_min;
				Rml::Vector2f uv_max;
				Rml::Vector4f color;
			};

			static_assert(sizeof(CBV_DropShadow) == kAllocationSize_ConstantBuffer_Pixel_DropShadow, "must match the shader's uniform block");

			CBV_DropShadow uploading_data = {};

			const Rml::Colourf& color = ConvertToColorf(filter.color);

			uploading_data.color.x = color.red;
			uploading_data.color.y = color.green;
			uploading_data.color.z = color.blue;
			uploading_data.color.w = color.alpha;

			SetTexCoordLimits(uploading_data.uv_min, uploading_data.uv_max, m_scissor, {primary.Get_Width(), primary.Get_Height()});

			if (p_cb_dropshadow)
			{
				std::uint8_t* p_gpu_begin = reinterpret_cast<std::uint8_t*>(p_cb_dropshadow->m_p_gpu_start_memory_for_binding_data);
				RMLUI_ASSERTMSG(p_gpu_begin, "constant buffer must contain information about its GPU location (pointer for data binding/uploading)");

				if (p_gpu_begin)
				{
					// the drop shadow uniform block starts at offset 0 (it has no transform, unlike blur's)
					std::uint8_t* p_gpu_real_begin = p_gpu_begin + p_cb_dropshadow->m_alloc_info.offset;
					std::memcpy(p_gpu_real_begin, &uploading_data, sizeof(uploading_data));
				}
			}

			// the Y sign differs from the DX12 renderer (which divides by -width, +height) because this backend has no
			// UV flip in the shaders
			const Rml::Vector2f& uv_offset = filter.offset / Rml::Vector2f(-(float)m_width, -(float)m_height);
			DrawFullscreenQuad(uv_offset, Rml::Vector2f(1.0f), p_cb_dropshadow);

			if (filter.sigma >= 0.5f)
			{
				const Gfx::FramebufferData& tertiary = m_manager_render_layer.GetPostprocessTertiary();
				RenderBlur(filter.sigma, secondary, tertiary, m_scissor);
			}

			UseProgram(ProgramId::Passthrough_NoDepthStencil);

			BindRenderTarget(secondary, false);
			BindTexture(primary.Get_Texture(), 0);

			DrawFullscreenQuad();

			TransitionImageLayout(primary.Get_Texture(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

			m_manager_render_layer.SwapPostprocessPrimarySecondary();

			break;
		}
		case FilterType::ColorMatrix:
		{
			UseProgram(ProgramId::ColorMatrix);

			const Gfx::FramebufferData& source = m_manager_render_layer.GetPostprocessPrimary();
			const Gfx::FramebufferData& destination = m_manager_render_layer.GetPostprocessSecondary();

			TransitionImageLayout(source.Get_Texture(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

			BindRenderTarget(destination, false);
			BindTexture(source.Get_Texture(), 0);

			ConstantBufferType* p_cb = Get_ConstantBuffer(m_command_buffer_ring.Get_ActiveFrameIndex());
			RMLUI_ASSERTMSG(p_cb, "failed to obtain constant buffer for color matrix");

			if (p_cb)
			{
				std::uint8_t* p_cb_begin = reinterpret_cast<std::uint8_t*>(p_cb->m_p_gpu_start_memory_for_binding_data);
				RMLUI_ASSERTMSG(p_cb_begin, "constant buffer must provide gpu begin binding pointer for uploading data from CPU");

				if (p_cb_begin)
				{
					std::uint8_t* p_cb_real_begin = p_cb_begin + p_cb->m_alloc_info.offset;
					RMLUI_ASSERTMSG(p_cb_real_begin,
						"constant buffer must provide gpu begin binding pointer for upload data from CPU (offset applied)");

					if (p_cb_real_begin)
					{
						constexpr bool is_need_transpose = std::is_same<decltype(filter.color_matrix), Rml::RowMajorMatrix4f>::value;

						const float* p_data = is_need_transpose ? filter.color_matrix.Transpose().data() : filter.color_matrix.data();

						// the color matrix uniform block starts at offset 0 (it has no transform)
						std::memcpy(p_cb_real_begin, p_data, sizeof(filter.color_matrix));
					}
				}
			}

			DrawFullscreenQuad(p_cb);

			TransitionImageLayout(source.Get_Texture(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

			m_manager_render_layer.SwapPostprocessPrimarySecondary();

			break;
		}
		case FilterType::MaskImage:
		{
			UseProgram(ProgramId::BlendMask);

			const Gfx::FramebufferData& source = m_manager_render_layer.GetPostprocessPrimary();
			const Gfx::FramebufferData& blend_mask = m_manager_render_layer.GetBlendMask();
			const Gfx::FramebufferData& destination = m_manager_render_layer.GetPostprocessSecondary();

			RMLUI_ASSERTMSG(source.Get_Texture() && blend_mask.Get_Texture() && destination.Get_Texture(), "must be valid textures!");

			TransitionImageLayout(source.Get_Texture(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			TransitionImageLayout(blend_mask.Get_Texture(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

			BindRenderTarget(destination, false);

			// The blend-mask program takes both textures in one descriptor set (bindings 0 and 1 of the blend_mask
			// layout). A transient set is allocated per filter application and handed to the deferred-deletion list
			// (freed once a full swapchain cycle passed), so a set is never rewritten or released while it can still be
			// referenced by in-flight GPU work.
			VkDescriptorSet p_blend_mask_set = nullptr;
			bool is_allocated = m_manager_descriptors.Alloc_Descriptor(m_p_device, &m_p_descriptor_set_layout_blend_mask, &p_blend_mask_set);
			RMLUI_VK_ASSERTMSG(is_allocated && p_blend_mask_set, "failed to allocate the blend mask descriptor set");

			VkDescriptorImageInfo images[2] = {};
			images[0].sampler = m_p_sampler_linear;
			images[0].imageView = source.Get_Texture()->Get_ImageView();
			images[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			images[1].sampler = m_p_sampler_linear;
			images[1].imageView = blend_mask.Get_Texture()->Get_ImageView();
			images[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			VkWriteDescriptorSet writes[2] = {};
			writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writes[0].dstSet = p_blend_mask_set;
			writes[0].dstBinding = 0;
			writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writes[0].descriptorCount = 1;
			writes[0].pImageInfo = &images[0];
			writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writes[1].dstSet = p_blend_mask_set;
			writes[1].dstBinding = 1;
			writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writes[1].descriptorCount = 1;
			writes[1].pImageInfo = &images[1];

			vkUpdateDescriptorSets(m_p_device, 2, writes, 0, nullptr);

			vkCmdBindDescriptorSets(m_p_current_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_p_pipeline_layout_blend_mask, 0, 1,
				&p_blend_mask_set, 0, nullptr);

			DrawFullscreenQuad();

			if (p_blend_mask_set)
				g_pending_for_deletion_descriptor_sets.push_back({p_blend_mask_set, m_frame_counter, &m_manager_descriptors});

			TransitionImageLayout(source.Get_Texture(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
			TransitionImageLayout(blend_mask.Get_Texture(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

			m_manager_render_layer.SwapPostprocessPrimarySecondary();

			break;
		}
		case FilterType::Invalid:
		{
			Rml::Log::Message(Rml::Log::LT_WARNING, "Invalid (Unhandled) render filter: %d", static_cast<int>(type));
			break;
		}
		default:
		{
			Rml::Log::Message(Rml::Log::LT_WARNING, "Unknown render filter: %d", static_cast<int>(type));
			break;
		}
		}
	}
}

void RenderInterface_VK::RenderBlur(float sigma, const Gfx::FramebufferData& source_destination, const Gfx::FramebufferData& temp,
	const Rml::Rectanglei window)
{
	RMLUI_ZoneScopedN("Vulkan - RenderBlur");
	RMLUI_ASSERTMSG(&source_destination != &temp, "you can't pass the same object to source_destination and temp arguments!");
	RMLUI_ASSERTMSG(source_destination.Get_Width() == temp.Get_Width(), "must be equal to the same size!");
	RMLUI_ASSERTMSG(source_destination.Get_Height() == temp.Get_Height(), "must be equal to the same size!");
	RMLUI_ASSERTMSG(window.Valid(), "must be valid!");

	int pass_level = 0;
	SigmaToParameters(sigma, pass_level, sigma);

	if (sigma == 0)
		return;

	const Rml::Rectanglei original_scissor = m_scissor;
	Rml::Rectanglei scissor = window;

	// probably we expect only textures with sample count = 1 otherwise it is MSAA and we should dynamically determine which pipeline state we should
	// use in UseProgram method
	UseProgram(ProgramId::Passthrough_NoBlendAndNoMSAA);
	SetScissor(scissor);

	VkViewport vp = {};

	// unlike DX12 there is no vertical flip anywhere in this backend, so the quarter-size downsample target lives in
	// the TOP-LEFT corner of the framebuffer (y = 0), matching the straight halved scissor rectangles below
	vp.x = 0;
	vp.y = 0;
	vp.width = source_destination.Get_Width() / 2.0f;
	vp.height = source_destination.Get_Height() / 2.0f;
	vp.minDepth = 0.0f;
	vp.maxDepth = 1.0f;

	vkCmdSetViewport(m_p_current_command_buffer, 0, 1, &vp);

	const Rml::Vector2f uv_scaling = {(source_destination.Get_Width() % 2 == 1) ? (1.f - 1.f / float(source_destination.Get_Width())) : 1.f,
		(source_destination.Get_Height() % 2 == 1) ? (1.f - 1.f / float(source_destination.Get_Height())) : 1.f};

	for (int i = 0; i < pass_level; i++)
	{
		scissor.p0 = (scissor.p0 + Rml::Vector2i(1)) / 2;
		scissor.p1 = Rml::Math::Max(scissor.p1 / 2, scissor.p0);
		const bool from_source = (i % 2 == 0);

		const Gfx::FramebufferData& fb_source = from_source ? source_destination : temp;
		const Gfx::FramebufferData& fb_destination = from_source ? temp : source_destination;

		TransitionImageLayout(fb_source.Get_Texture(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		BindRenderTarget(fb_destination, false);
		BindTexture(fb_source.Get_Texture(), 0);

		SetScissor(scissor);

		DrawFullscreenQuad({}, uv_scaling);

		TransitionImageLayout(fb_source.Get_Texture(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	}

	// restore the full-size viewport of the framebuffers
	vp.x = 0;
	vp.y = 0;
	vp.width = static_cast<float>(source_destination.Get_Width());
	vp.height = static_cast<float>(source_destination.Get_Height());
	vp.minDepth = 0.0f;
	vp.maxDepth = 1.0f;

	vkCmdSetViewport(m_p_current_command_buffer, 0, 1, &vp);

	const bool transfer_to_temp_buffer = (pass_level % 2 == 0);

	if (transfer_to_temp_buffer)
	{
		TransitionImageLayout(source_destination.Get_Texture(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		BindRenderTarget(temp, false);
		BindTexture(source_destination.Get_Texture(), 0);

		DrawFullscreenQuad();
	}

	UseProgram(ProgramId::Blur);

	ConstantBufferType* p_cb = Get_ConstantBuffer(m_command_buffer_ring.Get_ActiveFrameIndex());

	RMLUI_ASSERTMSG(p_cb, "unable to allocate constant buffer for blur!");
	std::uint8_t* p_cb_begin = reinterpret_cast<std::uint8_t*>(p_cb->m_p_gpu_start_memory_for_binding_data);

	RMLUI_ASSERTMSG(p_cb_begin, "must be valid pointer of buffer where CB was allocated!");

	// the blur uniform block carries transform+translate first (only m_texelOffset and below are used by the shaders),
	// so the payload lives at offset +72; the transform area is left stale, exactly like the DX12 renderer
	std::uint8_t* p_cb_real_begin = p_cb_begin + p_cb->m_alloc_info.offset + sizeof(m_constant_buffer_data_transform) + sizeof(Rml::Vector2f);

	// sadly but here we can't optimize and upload directly using pointer from GPU
	// keep allocating on stack still fastest way possible to handle a such small data for uploading :)
	struct {
		Rml::Vector2f texel_offset;
		Rml::Vector4f weights;
		Rml::Vector2f texcoord_min;
		Rml::Vector2f texcoord_max;
	} ShaderConstantBufferMapping_Blur;

	SetBlurWeights(ShaderConstantBufferMapping_Blur.weights, sigma);
	SetTexCoordLimits(ShaderConstantBufferMapping_Blur.texcoord_min, ShaderConstantBufferMapping_Blur.texcoord_max, scissor,
		{source_destination.Get_Width(), source_destination.Get_Height()});

	ShaderConstantBufferMapping_Blur.texel_offset = Rml::Vector2f(0.f, 1.f) * (1.0f / float(temp.Get_Height()));

	std::memcpy(p_cb_real_begin, &ShaderConstantBufferMapping_Blur, sizeof(ShaderConstantBufferMapping_Blur));

	if (transfer_to_temp_buffer)
	{
		TransitionImageLayout(source_destination.Get_Texture(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	}

	// vertical blur pass: temp -> source_destination
	TransitionImageLayout(temp.Get_Texture(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	BindRenderTarget(source_destination, false);
	BindTexture(temp.Get_Texture(), 0);
	DrawFullscreenQuad(p_cb);

	// horizontal blur pass: source_destination -> temp
	TransitionImageLayout(temp.Get_Texture(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	TransitionImageLayout(source_destination.Get_Texture(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	BindRenderTarget(temp, false);
	BindTexture(source_destination.Get_Texture(), 0);

	// Add a 1px transparent border around the blur region by first clearing with a padded scissor. This helps prevent
	// artifacts when upscaling the blur result in the later step. On Intel and AMD, we have observed that during
	// blitting with linear filtering, pixels outside the 'src' region can be blended into the output. On the other
	// hand, it looks like Nvidia clamps the pixels to the source edge, which is what we really want. Regardless, we
	// work around the issue with this extra step.
	Rml::Rectanglei scissor_ext = scissor.Extend(1);

	// Some render APIs don't like offscreen positions (WebGL in particular), so clamp them to the viewport.
	const int x_min = Rml::Math::Clamp(scissor_ext.Left(), 0, m_width);
	const int y_min = Rml::Math::Clamp(scissor_ext.Top(), 0, m_height);
	const int x_max = Rml::Math::Clamp(scissor_ext.Right(), 0, m_width);
	const int y_max = Rml::Math::Clamp(scissor_ext.Bottom(), 0, m_height);

	// vkCmdClearAttachments is not affected by the scissor state, so the extended rect is passed directly
	// (a fully offscreen or degenerate window can collapse to an empty rect after clamping; clearing it is a no-op
	// and Vulkan forbids zero-sized clear rectangles, so skip it)
	if (x_max > x_min && y_max > y_min)
	{
		VkClearAttachment clear_attachment = {};
		clear_attachment.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		clear_attachment.colorAttachment = 0;
		clear_attachment.clearValue.color = {{0.0f, 0.0f, 0.0f, 0.0f}};

		VkClearRect clear_rect = {};
		clear_rect.rect.offset.x = x_min;
		clear_rect.rect.offset.y = y_min;
		clear_rect.rect.extent.width = static_cast<uint32_t>(x_max - x_min);
		clear_rect.rect.extent.height = static_cast<uint32_t>(y_max - y_min);
		clear_rect.baseArrayLayer = 0;
		clear_rect.layerCount = 1;

		vkCmdClearAttachments(m_p_current_command_buffer, 1, &clear_attachment, 1, &clear_rect);
	}

	SetScissor(scissor);

	ShaderConstantBufferMapping_Blur.texel_offset = Rml::Vector2f(1.0f, 0.0f) * (1.0f / float(source_destination.Get_Width()));

	p_cb = Get_ConstantBuffer(m_command_buffer_ring.Get_ActiveFrameIndex());

	RMLUI_ASSERTMSG(p_cb, "unable to allocate constant buffer for blur!");
	p_cb_begin = reinterpret_cast<std::uint8_t*>(p_cb->m_p_gpu_start_memory_for_binding_data);

	RMLUI_ASSERTMSG(p_cb_begin, "must be valid pointer of buffer where CB was allocated!");

	p_cb_real_begin = p_cb_begin + p_cb->m_alloc_info.offset + sizeof(m_constant_buffer_data_transform) + sizeof(Rml::Vector2f);
	std::memcpy(p_cb_real_begin, &ShaderConstantBufferMapping_Blur, sizeof(ShaderConstantBufferMapping_Blur));
	DrawFullscreenQuad(p_cb);

	TransitionImageLayout(temp.Get_Texture(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	TransitionImageLayout(source_destination.Get_Texture(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	// Blit the blurred image to the scissor region with upscaling (no Y flip, unlike the DX12 renderer).
	SetScissor(window);

	const Rml::Vector2i src_min = scissor.p0;
	const Rml::Vector2i src_max = scissor.p1;
	const Rml::Vector2i dst_min = window.p0;
	const Rml::Vector2i dst_max = window.p1;

	BlitFramebuffer(temp, source_destination, src_min.x, src_min.y, src_max.x, src_max.y, dst_min.x, dst_min.y, dst_max.x, dst_max.y);

	// DirectX 12 implementation notice: The following note is not implemented for this renderer, see the OpenGL 3 reference for details.
	//   The above upscale blit might be jittery at low resolutions (large pass levels). This is especially noticeable
	//   when moving an element with backdrop blur around or when trying to click/hover an element within a blurred
	//   region since it may be rendered at an offset. For more stable and accurate rendering we next upscale the blur
	//   image by an exact power-of-two. However, this may not fill the edges completely so we need to do the above
	//   first. Note that this strategy may sometimes result in visible seams. Alternatively, we could try to enlarge
	//   the window to the next power-of-two size and then downsample and blur that.

	TransitionImageLayout(temp.Get_Texture(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	// restore things
	VkViewport viewport = {};
	viewport.height = static_cast<float>(m_height);
	viewport.width = static_cast<float>(m_width);
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(m_p_current_command_buffer, 0, 1, &viewport);
	SetScissor(original_scissor);
}

void RenderInterface_VK::DrawFullscreenQuad(ConstantBufferType* p_override_constant_buffer)
{
	RMLUI_ZoneScopedN("Vulkan - DrawFullscreenQuad()");
	RMLUI_ASSERTMSG(m_p_current_command_buffer, "must be valid!");

	// actually rml doesn't support anything for by passing custom data as additional argument for RenderGeometry method so
	// some kind of variant for resolving a such situation
	OverrideConstantBufferOfGeometry(m_precompiled_fullscreen_quad_geometry, p_override_constant_buffer);

	RenderGeometry(m_precompiled_fullscreen_quad_geometry, {}, TexturePostprocess);

	if (p_override_constant_buffer)
	{
		GeometryHandleType* p_geometry = reinterpret_cast<GeometryHandleType*>(m_precompiled_fullscreen_quad_geometry);
		p_geometry->Reset_ConstantBuffer();
	}
}

void RenderInterface_VK::DrawFullscreenQuad(Rml::Vector2f uv_offset, Rml::Vector2f uv_scaling, ConstantBufferType* p_override_constant_buffer)
{
	RMLUI_ZoneScopedN("Vulkan - DrawFullscreenQuad(uv_offset,uv_scaling)");
	RMLUI_ASSERTMSG(m_p_current_command_buffer, "must be valid!");

	Rml::Mesh mesh;
	Rml::MeshUtilities::GenerateQuad(mesh, Rml::Vector2f(-1), Rml::Vector2f(2), {});
	if (uv_offset != Rml::Vector2f() || uv_scaling != Rml::Vector2f(1.f))
	{
		for (Rml::Vertex& vertex : mesh.vertices)
			vertex.tex_coord = (vertex.tex_coord * uv_scaling) + uv_offset;
	}
	Rml::CompiledGeometryHandle geometry = CompileGeometry(mesh.vertices, mesh.indices);

	OverrideConstantBufferOfGeometry(geometry, p_override_constant_buffer);

	RenderGeometry(geometry, {}, TexturePostprocess);
	ReleaseGeometry(geometry);

	if (p_override_constant_buffer)
	{
		GeometryHandleType* p_geometry = reinterpret_cast<GeometryHandleType*>(geometry);
		p_geometry->Reset_ConstantBuffer();
	}
}

void RenderInterface_VK::BindTexture(TextureHandleType* p_texture, uint32_t set_index)
{
	RMLUI_ZoneScopedN("Vulkan - BindTexture");
	RMLUI_ASSERTMSG(p_texture, "you have to pass a valid pointer!");
	RMLUI_ASSERTMSG(m_p_current_command_buffer, "early calling must be initialized before calling this method!");

	if (m_p_current_command_buffer)
	{
		if (p_texture)
		{
			VkDescriptorSet p_set = p_texture->Get_DescriptorSet();
			RMLUI_VK_ASSERTMSG(p_set, "the texture has no descriptor set (depth-stencil textures can't be bound for sampling)");

			RMLUI_VK_PROGRAM_PIPELINE_LAYOUT_LOOKUP(get_pipeline_layout_for_program);

			vkCmdBindDescriptorSets(m_p_current_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, get_pipeline_layout_for_program(m_active_program_id),
				set_index, 1, &p_set, 0, nullptr);
		}
	}
}

void RenderInterface_VK::OverrideConstantBufferOfGeometry(Rml::CompiledGeometryHandle geometry, ConstantBufferType* p_override_constant_buffer)
{
	RMLUI_ZoneScopedN("Vulkan - OverrideConstantBufferOfGeometry");

	if (p_override_constant_buffer)
	{
		GeometryHandleType* p_geometry = reinterpret_cast<GeometryHandleType*>(geometry);
		p_geometry->Set_ConstantBuffer(p_override_constant_buffer);

		switch (m_active_program_id)
		{
		case ProgramId::Blur:
		case ProgramId::DropShadow:
		case ProgramId::ColorMatrix:
		{
			// unlike the DX12 renderer there is no root-parameter-index bookkeeping here: the single dynamic-UBO
			// binding (set 1 of the texture_effect layout) is visible to both the vertex and the fragment stage
			break;
		}
		default:
		{
			RMLUI_ASSERT(!"FATAL YOU FORGOT TO REGISTER CONSTANT BUFFER OVERRIDE SITUATION!");
			break;
		}
		}
	}
}

void RenderInterface_VK::BlitFramebuffer(const Gfx::FramebufferData& source, const Gfx::FramebufferData& dest, int srcX0, int srcY0, int srcX1,
	int srcY1, int dstX0, int dstY0, int dstX1, int dstY1)
{
	RMLUI_ZoneScopedN("Vulkan - BlitFramebuffer");
	RMLUI_ASSERTMSG(m_p_current_command_buffer, "must be initialized renderer before calling this method!");

	if (!m_p_current_command_buffer)
		return;

	int src_width = srcX1 - srcX0;
	int src_height = srcY1 - srcY0;
	int dest_width = dstX1 - dstX0;
	int dest_height = dstY1 - dstY0;

	// a fully clipped-away region (e.g. an empty scissor window of an offscreen blurred element) produces a
	// degenerate rectangle: nothing to copy, and Vulkan forbids zero-sized copy extents
	if (src_width <= 0 || src_height <= 0 || dest_width <= 0 || dest_height <= 0)
		return;

	const bool is_flipped = src_width < 0 || src_height < 0 || dest_width < 0 || dest_height < 0;
	const bool is_stretched = src_width != dest_width || src_height != dest_height;

	if (is_flipped || is_stretched)
	{
		// Full draw call. Slow path because no equivalent in DX12

#ifdef RMLUI_VK_DEBUG
		TextureHandleType* p_texture_source = source.Get_Texture();
		TextureHandleType* p_texture_destination = dest.Get_Texture();

		RMLUI_ASSERTMSG(p_texture_source, "must be valid pointer!");
		RMLUI_ASSERTMSG(p_texture_destination, "must be valid pointer!");
		RMLUI_ASSERTMSG(p_texture_source->Get_Image(), "must contain a valid resource");
		RMLUI_ASSERTMSG(p_texture_destination->Get_Image(), "must contain a valid resource");
#endif

		VkViewport vp = {};
		vp.x = 0;
		vp.y = 0;
		vp.width = (float)dest.Get_Width();
		vp.height = (float)dest.Get_Height();
		vp.minDepth = 0.0f;
		vp.maxDepth = 1.0f;

		vkCmdSetViewport(m_p_current_command_buffer, 0, 1, &vp);

		UseProgram(ProgramId::Passthrough_NoBlendAndNoMSAA);

		BindRenderTarget(dest, false);
		BindTexture(source.Get_Texture(), 0);

		// Unlike the DX12 renderer there is no Y flip anywhere in the math below: with a positive viewport height
		// Vulkan's NDC y=-1 is the top row of the framebuffer, and the texture space is top-left-origin as well.
		float uv_x_min = float(srcX0) / float(source.Get_Width());
		float uv_y_min = float(srcY0) / float(source.Get_Height());
		float uv_x_max = float(srcX1) / float(source.Get_Width());
		float uv_y_max = float(srcY1) / float(source.Get_Height());

		float pos_x_min = (dstX0 / float(dest.Get_Width())) * 2.0f - 1.0f;
		float pos_y_min = (dstY0 / float(dest.Get_Height())) * 2.0f - 1.0f;
		float pos_x_max = ((dstX0 + dest_width) / float(dest.Get_Width())) * 2.0f - 1.0f;
		float pos_y_max = ((dstY0 + dest_height) / float(dest.Get_Height())) * 2.0f - 1.0f;

		Rml::Array<Rml::Vertex, 4> vertices;
		constexpr int indices[6]{0, 3, 1, 1, 3, 2};

		vertices[0].position.x = pos_x_min;
		vertices[0].position.y = pos_y_min;
		vertices[0].tex_coord.x = uv_x_min;
		vertices[0].tex_coord.y = uv_y_min;

		vertices[1].position.x = pos_x_max;
		vertices[1].position.y = pos_y_min;
		vertices[1].tex_coord.x = uv_x_max;
		vertices[1].tex_coord.y = uv_y_min;

		vertices[2].position.x = pos_x_max;
		vertices[2].position.y = pos_y_max;
		vertices[2].tex_coord.x = uv_x_max;
		vertices[2].tex_coord.y = uv_y_max;

		vertices[3].position.x = pos_x_min;
		vertices[3].position.y = pos_y_max;
		vertices[3].tex_coord.x = uv_x_min;
		vertices[3].tex_coord.y = uv_y_max;

		const Rml::CompiledGeometryHandle geometry =
			CompileGeometry({&vertices[0], sizeof(vertices) / sizeof(vertices[0])}, {&indices[0], sizeof(indices) / sizeof(indices[0])});

		RenderGeometry(geometry, {}, TexturePostprocess);
		ReleaseGeometry(geometry);
	}
	else
	{
		// Full 1:1 copy: use a plain image copy (the analog of DX12's CopyResource) instead of a draw.
		// Only reachable for the single-sampled postprocess framebuffers (vkCmdCopyImage can't blit MSAA images).
		TextureHandleType* p_texture_source = source.Get_Texture();
		TextureHandleType* p_texture_destination = dest.Get_Texture();

		RMLUI_ASSERTMSG(p_texture_source, "must be valid pointer!");
		RMLUI_ASSERTMSG(p_texture_destination, "must be valid pointer!");
		RMLUI_ASSERTMSG(p_texture_source->Get_Image(), "must contain a valid resource");
		RMLUI_ASSERTMSG(p_texture_destination->Get_Image(), "must contain a valid resource");

		EndActiveRenderPass();

		TransitionImageLayout(p_texture_source, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		TransitionImageLayout(p_texture_destination, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		VkImageCopy region = {};
		region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.srcSubresource.mipLevel = 0;
		region.srcSubresource.baseArrayLayer = 0;
		region.srcSubresource.layerCount = 1;
		region.srcOffset = {srcX0, srcY0, 0};
		region.dstSubresource = region.srcSubresource;
		region.dstOffset = {dstX0, dstY0, 0};
		region.extent = {static_cast<uint32_t>(src_width), static_cast<uint32_t>(src_height), 1};

		vkCmdCopyImage(m_p_current_command_buffer, p_texture_source->Get_Image(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			p_texture_destination->Get_Image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		// leave the images in the same layouts the draw path above would leave them in: the source stays sampled and
		// the destination stays a render target
		TransitionImageLayout(p_texture_source, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		TransitionImageLayout(p_texture_destination, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	}
}

Rml::TextureHandle RenderInterface_VK::SaveLayerAsTexture()
{
	RMLUI_ZoneScopedN("Vulkan - SaveLayerAsTexture");
	RMLUI_ASSERT(m_scissor.Valid());

	const Rml::Rectanglei bounds = m_scissor;

	Rml::TextureHandle render_texture = GenerateTexture({}, bounds.Size());
	if (!render_texture)
	{
		return {};
	}

	BlitLayerToPostprocessPrimary(m_manager_render_layer.GetTopLayerHandle());
	EnableScissorRegion(false);

	const Gfx::FramebufferData& source = m_manager_render_layer.GetPostprocessPrimary();
	const Gfx::FramebufferData& destination = m_manager_render_layer.GetPostprocessSecondary();

	TextureHandleType* p_texture_target = reinterpret_cast<TextureHandleType*>(render_texture);

	TransitionImageLayout(source.Get_Texture(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	BlitFramebuffer(source, destination, bounds.Left(), bounds.Top(), bounds.Right(), bounds.Bottom(), 0, 0, bounds.Width(), bounds.Height());

	TransitionImageLayout(source.Get_Texture(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	TransitionImageLayout(destination.Get_Texture(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	TransitionImageLayout(p_texture_target, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	// the blit above wrote the bounds region into the [0, W] x [0, H] area of the secondary framebuffer
	VkImageCopy region = {};
	region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.srcSubresource.mipLevel = 0;
	region.srcSubresource.baseArrayLayer = 0;
	region.srcSubresource.layerCount = 1;
	region.dstSubresource = region.srcSubresource;
	region.extent.width = static_cast<uint32_t>(bounds.Width());
	region.extent.height = static_cast<uint32_t>(bounds.Height());
	region.extent.depth = 1;

	vkCmdCopyImage(m_p_current_command_buffer, destination.Get_Texture()->Get_Image(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		p_texture_target->Get_Image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	TransitionImageLayout(destination.Get_Texture(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	TransitionImageLayout(p_texture_target, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	SetScissor(bounds);
	BindRenderTarget(m_manager_render_layer.GetTopLayer());

	return render_texture;
}

Rml::CompiledFilterHandle RenderInterface_VK::SaveLayerAsMaskImage()
{
	RMLUI_ZoneScopedN("Vulkan - SaveLayerAsMaskImage");

	BlitLayerToPostprocessPrimary(m_manager_render_layer.GetTopLayerHandle());

	const Gfx::FramebufferData& source = m_manager_render_layer.GetPostprocessPrimary();
	const Gfx::FramebufferData& destination = m_manager_render_layer.GetBlendMask();

	TransitionImageLayout(source.Get_Texture(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	UseProgram(ProgramId::Passthrough_NoBlendAndNoMSAA);
	BindRenderTarget(destination, false);
	BindTexture(source.Get_Texture(), 0);

	DrawFullscreenQuad();

	TransitionImageLayout(source.Get_Texture(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	BindRenderTarget(m_manager_render_layer.GetTopLayer());

	CompiledFilter filter = {};
	filter.type = FilterType::MaskImage;

	return reinterpret_cast<Rml::CompiledFilterHandle>(new CompiledFilter(std::move(filter)));
}

Rml::CompiledFilterHandle RenderInterface_VK::CompileFilter(const Rml::String& name, const Rml::Dictionary& parameters)
{
	RMLUI_ZoneScopedN("Vulkan - CompileFilter");
	CompiledFilter filter = {};

	if (name == "opacity")
	{
		filter.type = FilterType::Passthrough;
		filter.blend_factor = Rml::Get(parameters, "value", 1.0f);
	}
	else if (name == "blur")
	{
		filter.type = FilterType::Blur;
		filter.sigma = Rml::Get(parameters, "sigma", 1.0f);
	}
	else if (name == "drop-shadow")
	{
		filter.type = FilterType::DropShadow;
		filter.sigma = Rml::Get(parameters, "sigma", 0.f);
		filter.color = Rml::Get(parameters, "color", Rml::Colourb()).ToPremultiplied();
		filter.offset = Rml::Get(parameters, "offset", Rml::Vector2f(0.f));
	}
	else if (name == "brightness")
	{
		filter.type = FilterType::ColorMatrix;
		const float value = Rml::Get(parameters, "value", 1.0f);
		filter.color_matrix = Rml::Matrix4f::Diag(value, value, value, 1.f);
	}
	else if (name == "contrast")
	{
		filter.type = FilterType::ColorMatrix;
		const float value = Rml::Get(parameters, "value", 1.0f);
		const float grayness = 0.5f - 0.5f * value;
		filter.color_matrix = Rml::Matrix4f::Diag(value, value, value, 1.f);
		filter.color_matrix.SetColumn(3, Rml::Vector4f(grayness, grayness, grayness, 1.f));
	}
	else if (name == "invert")
	{
		filter.type = FilterType::ColorMatrix;
		const float value = Rml::Math::Clamp(Rml::Get(parameters, "value", 1.0f), 0.f, 1.f);
		const float inverted = 1.f - 2.f * value;
		filter.color_matrix = Rml::Matrix4f::Diag(inverted, inverted, inverted, 1.f);
		filter.color_matrix.SetColumn(3, Rml::Vector4f(value, value, value, 1.f));
	}
	else if (name == "grayscale")
	{
		filter.type = FilterType::ColorMatrix;
		const float value = Rml::Get(parameters, "value", 1.0f);
		const float rev_value = 1.f - value;
		const Rml::Vector3f gray = value * Rml::Vector3f(0.2126f, 0.7152f, 0.0722f);
		// clang-format off
		filter.color_matrix = Rml::Matrix4f::FromRows(
			{gray.x + rev_value, gray.y,             gray.z,             0.f},
			{gray.x,             gray.y + rev_value, gray.z,             0.f},
			{gray.x,             gray.y,             gray.z + rev_value, 0.f},
			{0.f,                0.f,                0.f,                1.f}
		);
		// clang-format on
	}
	else if (name == "sepia")
	{
		filter.type = FilterType::ColorMatrix;
		const float value = Rml::Get(parameters, "value", 1.0f);
		const float rev_value = 1.f - value;
		const Rml::Vector3f r_mix = value * Rml::Vector3f(0.393f, 0.769f, 0.189f);
		const Rml::Vector3f g_mix = value * Rml::Vector3f(0.349f, 0.686f, 0.168f);
		const Rml::Vector3f b_mix = value * Rml::Vector3f(0.272f, 0.534f, 0.131f);
		// clang-format off
		filter.color_matrix = Rml::Matrix4f::FromRows(
			{r_mix.x + rev_value, r_mix.y,             r_mix.z,             0.f},
			{g_mix.x,             g_mix.y + rev_value, g_mix.z,             0.f},
			{b_mix.x,             b_mix.y,             b_mix.z + rev_value, 0.f},
			{0.f,                 0.f,                 0.f,                 1.f}
		);
		// clang-format on
	}
	else if (name == "hue-rotate")
	{
		// Hue-rotation and saturation values based on: https://www.w3.org/TR/filter-effects-1/#attr-valuedef-type-huerotate
		filter.type = FilterType::ColorMatrix;
		const float value = Rml::Get(parameters, "value", 1.0f);
		const float s = Rml::Math::Sin(value);
		const float c = Rml::Math::Cos(value);
		// clang-format off
		filter.color_matrix = Rml::Matrix4f::FromRows(
			{0.213f + 0.787f * c - 0.213f * s,  0.715f - 0.715f * c - 0.715f * s,  0.072f - 0.072f * c + 0.928f * s,  0.f},
			{0.213f - 0.213f * c + 0.143f * s,  0.715f + 0.285f * c + 0.140f * s,  0.072f - 0.072f * c - 0.283f * s,  0.f},
			{0.213f - 0.213f * c - 0.787f * s,  0.715f - 0.715f * c + 0.715f * s,  0.072f + 0.928f * c + 0.072f * s,  0.f},
			{0.f,                               0.f,                               0.f,                               1.f}
		);
		// clang-format on
	}
	else if (name == "saturate")
	{
		filter.type = FilterType::ColorMatrix;
		const float value = Rml::Get(parameters, "value", 1.0f);
		// clang-format off
		filter.color_matrix = Rml::Matrix4f::FromRows(
			{0.213f + 0.787f * value,  0.715f - 0.715f * value,  0.072f - 0.072f * value,  0.f},
			{0.213f - 0.213f * value,  0.715f + 0.285f * value,  0.072f - 0.072f * value,  0.f},
			{0.213f - 0.213f * value,  0.715f - 0.715f * value,  0.072f + 0.928f * value,  0.f},
			{0.f,                      0.f,                      0.f,                      1.f}
		);
		// clang-format on
	}

	if (filter.type != FilterType::Invalid)
		return reinterpret_cast<Rml::CompiledFilterHandle>(new CompiledFilter(std::move(filter)));

	Rml::Log::Message(Rml::Log::LT_WARNING, "Unsupported filter type '%s'.", name.c_str());
	return {};
}

void RenderInterface_VK::ReleaseFilter(Rml::CompiledFilterHandle filter)
{
	RMLUI_ZoneScopedN("Vulkan - ReleaseFilter");
	delete reinterpret_cast<CompiledFilter*>(filter);
}

Rml::CompiledShaderHandle RenderInterface_VK::CompileShader(const Rml::String& name, const Rml::Dictionary& parameters)
{
	RMLUI_ZoneScopedN("Vulkan - CompileShader");
	auto ApplyColorStopList = [](CompiledShader& shader, const Rml::Dictionary& shader_parameters) {
		auto it = shader_parameters.find("color_stop_list");
		RMLUI_ASSERT(it != shader_parameters.end() && it->second.GetType() == Rml::Variant::COLORSTOPLIST);
		const Rml::ColorStopList& color_stop_list = it->second.GetReference<Rml::ColorStopList>();
		const int num_stops = Rml::Math::Min((int)color_stop_list.size(), MAX_NUM_STOPS);

		shader.stop_positions.resize(num_stops);
		shader.stop_colors.resize(num_stops);
		for (int i = 0; i < num_stops; i++)
		{
			const Rml::ColorStop& stop = color_stop_list[i];
			RMLUI_ASSERT(stop.position.unit == Rml::Unit::NUMBER);
			shader.stop_positions[i] = stop.position.number;
			shader.stop_colors[i] = ConvertToColorf(stop.color);
		}
	};

	CompiledShader shader = {};

	if (name == "linear-gradient")
	{
		shader.type = CompiledShaderType::Gradient;
		const bool repeating = Rml::Get(parameters, "repeating", false);
		shader.gradient_function = (repeating ? ShaderGradientFunction::RepeatingLinear : ShaderGradientFunction::Linear);
		shader.p = Rml::Get(parameters, "p0", Rml::Vector2f(0.f));
		shader.v = Rml::Get(parameters, "p1", Rml::Vector2f(0.f)) - shader.p;
		ApplyColorStopList(shader, parameters);
	}
	else if (name == "radial-gradient")
	{
		shader.type = CompiledShaderType::Gradient;
		const bool repeating = Rml::Get(parameters, "repeating", false);
		shader.gradient_function = (repeating ? ShaderGradientFunction::RepeatingRadial : ShaderGradientFunction::Radial);
		shader.p = Rml::Get(parameters, "center", Rml::Vector2f(0.f));
		shader.v = Rml::Vector2f(1.f) / Rml::Get(parameters, "radius", Rml::Vector2f(1.f));
		ApplyColorStopList(shader, parameters);
	}
	else if (name == "conic-gradient")
	{
		shader.type = CompiledShaderType::Gradient;
		const bool repeating = Rml::Get(parameters, "repeating", false);
		shader.gradient_function = (repeating ? ShaderGradientFunction::RepeatingConic : ShaderGradientFunction::Conic);
		shader.p = Rml::Get(parameters, "center", Rml::Vector2f(0.f));
		const float angle = Rml::Get(parameters, "angle", 0.f);
		shader.v = {Rml::Math::Cos(angle), Rml::Math::Sin(angle)};
		ApplyColorStopList(shader, parameters);
	}
	else if (name == "shader")
	{
		const Rml::String value = Rml::Get(parameters, "value", Rml::String());
		if (value == "creation")
		{
			shader.type = CompiledShaderType::Creation;
			shader.dimensions = Rml::Get(parameters, "dimensions", Rml::Vector2f(0.f));
		}
	}

	if (shader.type != CompiledShaderType::Invalid)
		return reinterpret_cast<Rml::CompiledShaderHandle>(new CompiledShader(std::move(shader)));

	Rml::Log::Message(Rml::Log::LT_WARNING, "Unsupported shader type '%s'.", name.c_str());
	return {};
}

void RenderInterface_VK::RenderShader(Rml::CompiledShaderHandle shader_handle, Rml::CompiledGeometryHandle geometry_handle, Rml::Vector2f translation,
	Rml::TextureHandle texture)
{
	RMLUI_ZoneScopedN("Vulkan - RenderShader");
	RMLUI_ASSERT(shader_handle && geometry_handle);

	// fixing unreferenced parameter
	(void)(texture);

	const CompiledShader& shader = *reinterpret_cast<CompiledShader*>(shader_handle);
	const CompiledShaderType type = shader.type;
	const GeometryHandleType* geometry = reinterpret_cast<GeometryHandleType*>(geometry_handle);

	// binds the constant buffer (set 0 of the transform layout) and issues the indexed draw; the single dynamic-UBO
	// binding is visible to both shader stages, covering what DX12 does with root CBV indices 0 and 1
	const auto bind_constant_buffer_and_draw = [this, geometry](ConstantBufferType* p_cb) {
		if (p_cb)
		{
			VkDescriptorSet p_set = m_manager_buffer.Get_ConstantBufferDescriptorSetByIndex(p_cb->m_alloc_info.buffer_index);
			RMLUI_VK_ASSERTMSG(p_set, "must be valid!");

			const uint32_t dynamic_offset = static_cast<uint32_t>(p_cb->m_alloc_info.offset);

			vkCmdBindDescriptorSets(m_p_current_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_p_pipeline_layout_transform, 0, 1, &p_set, 1,
				&dynamic_offset);
		}

		VkBuffer p_buffer_vertex = m_manager_buffer.Get_BufferByIndex(geometry->Get_InfoVertex().buffer_index);
		RMLUI_VK_ASSERTMSG(p_buffer_vertex, "must be valid!");

		VkDeviceSize vertex_buffer_offset = geometry->Get_InfoVertex().offset;
		vkCmdBindVertexBuffers(m_p_current_command_buffer, 0, 1, &p_buffer_vertex, &vertex_buffer_offset);

		VkBuffer p_buffer_index = m_manager_buffer.Get_BufferByIndex(geometry->Get_InfoIndex().buffer_index);
		RMLUI_VK_ASSERTMSG(p_buffer_index, "must be valid!");

		vkCmdBindIndexBuffer(m_p_current_command_buffer, p_buffer_index, geometry->Get_InfoIndex().offset, VK_INDEX_TYPE_UINT32);

		vkCmdDrawIndexed(m_p_current_command_buffer, geometry->Get_NumIndices(), 1, 0, 0, 0);
	};

	switch (type)
	{
	case CompiledShaderType::Gradient:
	{
		RMLUI_ASSERT(shader.stop_positions.size() == shader.stop_colors.size());
		const int num_stops = (int)shader.stop_positions.size();

		UseProgram(ProgramId::Gradient);
		ConstantBufferType* p_cb = Get_ConstantBuffer(m_command_buffer_ring.Get_ActiveFrameIndex());
		RMLUI_ASSERTMSG(p_cb, "failed to obtain constant buffer for gradient");

		struct CBV_Gradient {
			Rml::Matrix4f transform;
			Rml::Vector2f translate;

			int func;
			int num_stops;
			Rml::Vector2f starting_point;
			Rml::Vector2f ending_point;
			Rml::Vector4f stop_colors[MAX_NUM_STOPS];
			float stop_positions[MAX_NUM_STOPS];
		};

		static_assert(sizeof(CBV_Gradient) == kAllocationSize_ConstantBuffer_Pixel_Gradient, "must match the shader's uniform block");

		if (p_cb)
		{
			// build the full 416-byte image on the CPU and upload it with one memcpy (like the DX12 renderer)
			CBV_Gradient uploading_data = {};

			uploading_data.transform = m_constant_buffer_data_transform;
			uploading_data.translate = translation;
			uploading_data.func = (int)(shader.gradient_function);
			uploading_data.starting_point = shader.p;
			uploading_data.ending_point = shader.v;
			uploading_data.num_stops = num_stops;

			std::memcpy(&uploading_data.stop_positions, shader.stop_positions.data(), num_stops * sizeof(float));
			std::memcpy(&uploading_data.stop_colors, shader.stop_colors.data(), num_stops * sizeof(Rml::Vector4f));

			std::uint8_t* p_cb_begin = reinterpret_cast<std::uint8_t*>(p_cb->m_p_gpu_start_memory_for_binding_data);
			RMLUI_ASSERTMSG(p_cb_begin, "ConstantBuffer must contain valid pointer to begin of gpu binding pointer for uploading data from CPU!");

			if (p_cb_begin)
			{
				std::uint8_t* p_cb_begin_real = p_cb_begin + p_cb->m_alloc_info.offset;
				RMLUI_ASSERTMSG(p_cb_begin_real, "failed to offset gpu begin pointer for constant buffer!");

				std::memcpy(p_cb_begin_real, &uploading_data, sizeof(uploading_data));
			}
		}

		bind_constant_buffer_and_draw(p_cb);

		break;
	}
	case CompiledShaderType::Creation:
	{
		UseProgram(ProgramId::Creation);
		ConstantBufferType* p_cb = Get_ConstantBuffer(m_command_buffer_ring.Get_ActiveFrameIndex());
		RMLUI_ASSERTMSG(p_cb, "failed to obtain constant buffer for creation");

		struct CBV_Creation {
			Rml::Matrix4f transform;
			Rml::Vector2f translation;
			Rml::Vector2f dimensions;
			float time;
		};

		static_assert(sizeof(CBV_Creation) == kAllocationSize_ConstantBuffer_Pixel_Creation, "must match the shader's uniform block");

		if (p_cb)
		{
			CBV_Creation uploading_data = {};

			uploading_data.transform = m_constant_buffer_data_transform;
			uploading_data.translation = translation;

			const double time = Rml::GetSystemInterface()->GetElapsedTime();
			uploading_data.time = (float)time;
			uploading_data.dimensions = shader.dimensions;

			std::uint8_t* p_cb_begin = reinterpret_cast<std::uint8_t*>(p_cb->m_p_gpu_start_memory_for_binding_data);
			RMLUI_ASSERTMSG(p_cb_begin, "ConstantBuffer must contain valid pointer to begin of gpu binding pointer for uploading data from CPU!");

			if (p_cb_begin)
			{
				std::uint8_t* p_cb_begin_real = p_cb_begin + p_cb->m_alloc_info.offset;
				RMLUI_ASSERTMSG(p_cb_begin_real, "failed to offset gpu begin pointer for constant buffer!");

				std::memcpy(p_cb_begin_real, &uploading_data, sizeof(uploading_data));
			}
		}

		bind_constant_buffer_and_draw(p_cb);

		break;
	}
	case CompiledShaderType::Invalid:
	{
		Rml::Log::Message(Rml::Log::LT_WARNING, "Unhandled render shader %d", (int)type);
		break;
	}
	}
}

void RenderInterface_VK::ReleaseShader(Rml::CompiledShaderHandle effect_handle)
{
	RMLUI_ZoneScopedN("Vulkan - ReleaseShader");
	delete reinterpret_cast<CompiledShader*>(effect_handle);
}

static int nGlobalID{};

Rml::TextureHandle RenderInterface_VK::LoadTexture(Rml::Vector2i& texture_dimensions, const Rml::String& source)
{
	RMLUI_ZoneScopedN("Vulkan - LoadTexture");

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
	RMLUI_ZoneScopedN("Vulkan - GenerateTexture");

	// empty source data is allowed: it creates an uninitialized texture (see SaveLayerAsTexture)
	RMLUI_ASSERTMSG(source_data.empty() || source_data.size() == size_t(source_dimensions.x * source_dimensions.y * 4),
		"source data size doesn't match the given dimensions");
	RMLUI_ASSERTMSG(m_p_allocator, "backend requires initialized allocator, but it is not initialized");

	RMLUI_ASSERTMSG(source_dimensions.x > 0, "width is less or equal to 0");
	RMLUI_ASSERTMSG(source_dimensions.y > 0, "height is less or equal to 0");

	Rml::String source_name = "generated-texture";

#ifdef RMLUI_VK_DEBUG
	source_name += "[" + std::to_string(nGlobalID) + "]";
	++nGlobalID;
#endif

	return CreateTexture(source_data, source_dimensions, source_name);
}

Rml::TextureHandle RenderInterface_VK::CreateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i dimensions, const Rml::String& name)
{
	RMLUI_ZoneScopedN("Vulkan - CreateTexture");

	RMLUI_ASSERT(m_p_allocator && "you have to initialize Vma Allocator for this method");
	(void)name;

	int width = dimensions.x;
	int height = dimensions.y;

	RMLUI_ASSERT(width > 0 && "invalid width");
	RMLUI_ASSERT(height > 0 && "invalid height");

	Rml::TextureHandle temp_result = 0;

	if (m_p_allocator == nullptr)
		return temp_result;

	if (width <= 0)
		return temp_result;

	if (height <= 0)
		return temp_result;

	auto* p_texture = new TextureHandleType{};

	m_manager_texture.Alloc_Texture(p_texture, dimensions, source.empty() ? nullptr : source.data()
#ifdef RMLUI_VK_DEBUG
															   ,
		name
#endif
	);

	return reinterpret_cast<Rml::TextureHandle>(p_texture);
}

void RenderInterface_VK::ReleaseTexture(Rml::TextureHandle texture_handle)
{
	RMLUI_ZoneScopedN("Vulkan - ReleaseTexture");

	TextureHandleType* p_texture = reinterpret_cast<TextureHandleType*>(texture_handle);

	if (p_texture)
	{
#ifdef RMLUI_VK_DEBUG
		Rml::Log::Message(Rml::Log::LT_DEBUG, "Queued texture for destruction: [%s]", p_texture->Get_ResourceName().c_str());
#endif

		// defer destruction: in-flight frames may still sample this texture
		m_pending_for_deletion_textures.push_back({p_texture, m_frame_counter});
	}
}

bool RenderInterface_VK::CaptureScreen(int& width, int& height, int& num_components, Rml::UniquePtr<Rml::byte[]>& data)
{
	RMLUI_ZoneScopedN("Vulkan - CaptureScreen");

	width = -1;
	height = -1;
	num_components = -1;
	data = {};

	if (!m_p_device || !m_p_swapchain || !m_p_allocator)
	{
		RMLUI_ERRORMSG("Early calling");
		return false;
	}

	// the composited frame only exists after the first EndFrame resolved the top layer into the postprocess primary
	if (m_frame_counter == 0)
	{
		RMLUI_ERRORMSG("Cannot capture the screen before the first frame was presented");
		return false;
	}

	const uint32_t image_width = static_cast<uint32_t>(m_width);
	const uint32_t image_height = static_cast<uint32_t>(m_height);
	const VkDeviceSize buffer_size = static_cast<VkDeviceSize>(image_width) * static_cast<VkDeviceSize>(image_height) * 4;

	// host-visible readback buffer
	VkBufferCreateInfo info_buffer = {};
	info_buffer.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	info_buffer.pNext = nullptr;
	info_buffer.size = buffer_size;
	info_buffer.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	info_buffer.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo info_allocation = {};
	info_allocation.usage = VMA_MEMORY_USAGE_GPU_TO_CPU;
	info_allocation.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

	VkBuffer p_readback_buffer = nullptr;
	VmaAllocation p_readback_allocation = nullptr;
	VmaAllocationInfo info_readback_allocation = {};

	VkResult status =
		vmaCreateBuffer(m_p_allocator, &info_buffer, &info_allocation, &p_readback_buffer, &p_readback_allocation, &info_readback_allocation);

	if (status != VK_SUCCESS || p_readback_buffer == nullptr)
	{
		RMLUI_ERRORMSG("failed to create buffer for CPU reading");
		return false;
	}

	// read back the postprocess primary texture: after EndFrame it holds the fully composited frame (exactly what is
	// drawn into the swapchain image), and unlike the swapchain image it is an ordinary image owned by the renderer —
	// a presented image belongs to the presentation engine until it is re-acquired and must not be transitioned here
	TextureHandleType* p_source_texture = m_manager_render_layer.GetPostprocessPrimary().Get_Texture();

	if (p_source_texture == nullptr || p_source_texture->Get_Image() == nullptr)
	{
		RMLUI_ERRORMSG("no composited frame to capture");
		vmaDestroyBuffer(m_p_allocator, p_readback_buffer, p_readback_allocation);
		return false;
	}

	VkImage p_source_image = p_source_texture->Get_Image();

	m_upload_manager.UploadToGPU([p_source_image, p_readback_buffer, image_width, image_height](VkCommandBuffer p_cmd) {
		VkImageMemoryBarrier info_barrier = {};
		info_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		info_barrier.pNext = nullptr;
		info_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		info_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		info_barrier.image = p_source_image;
		info_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		info_barrier.subresourceRange.baseMipLevel = 0;
		info_barrier.subresourceRange.levelCount = 1;
		info_barrier.subresourceRange.baseArrayLayer = 0;
		info_barrier.subresourceRange.layerCount = 1;
		info_barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		info_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		info_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		info_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(p_cmd, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1,
			&info_barrier);

		VkBufferImageCopy region = {};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageOffset = {0, 0, 0};
		region.imageExtent = {image_width, image_height, 1};

		vkCmdCopyImageToBuffer(p_cmd, p_source_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, p_readback_buffer, 1, &region);

		info_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		info_barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		info_barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		info_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;

		vkCmdPipelineBarrier(p_cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1,
			&info_barrier);
	});

	bool result = false;

	if (info_readback_allocation.pMappedData)
	{
		const size_t source_num_components = 4;
		const size_t target_num_components = 3;
		const size_t width_size_t = image_width;
		const size_t height_size_t = image_height;

		const size_t target_row_size_bytes = target_num_components * width_size_t;
		const size_t target_data_size = target_row_size_bytes * height_size_t;
		data.reset(new Rml::byte[target_data_size]);

		for (size_t y = 0; y < height_size_t; y++)
		{
			// the output rows are stored bottom-up, like the DX12 renderer produces; the source is R8G8B8A8, so
			// stripping the alpha byte yields the same RGB byte order the DX12 renderer's readback produces
			const Rml::byte* source_row_origin = reinterpret_cast<const Rml::byte*>(info_readback_allocation.pMappedData) +
				(height_size_t - 1 - y) * width_size_t * source_num_components;
			Rml::byte* target_row_origin = data.get() + y * width_size_t * target_num_components;

			for (size_t x = 0; x < width_size_t; x++)
			{
				const Rml::byte* p_source_pixel = source_row_origin + x * source_num_components;
				Rml::byte* p_target_pixel = target_row_origin + x * target_num_components;

				std::memcpy(p_target_pixel, p_source_pixel, target_num_components);
			}
		}

		width = static_cast<int>(image_width);
		height = static_cast<int>(image_height);
		num_components = static_cast<int>(target_num_components);
		result = true;
	}

	vmaDestroyBuffer(m_p_allocator, p_readback_buffer, p_readback_allocation);

#ifdef RMLUI_VK_DEBUG
	if (result)
		Rml::Log::Message(Rml::Log::Type::LT_DEBUG, "[Vulkan]: successfully captured data of swapchain backbuffer");
#endif

	return result;
}

void RenderInterface_VK::UploadResourceManager::Initialize(VkDevice p_device, VkQueue p_queue, VmaAllocator p_allocator, uint32_t queue_family_index,
	size_t staging_buffer_size)
{
	RMLUI_ZoneScopedN("Vulkan - UploadResourceManager::Initialize");
	RMLUI_VK_ASSERTMSG(p_device, "you must pass a valid VkDevice");
	RMLUI_VK_ASSERTMSG(p_queue, "you must pass a valid VkQueue");
	RMLUI_VK_ASSERTMSG(p_allocator, "you must pass a valid VmaAllocator");

	m_p_device = p_device;
	m_p_graphics_queue = p_queue;

	Create_All(queue_family_index, p_allocator, staging_buffer_size);
}

void RenderInterface_VK::UploadResourceManager::Shutdown(VmaAllocator p_allocator)
{
	RMLUI_ZoneScopedN("Vulkan - UploadResourceManager::Shutdown");

	if (m_upload_buffer.m_p_vk_buffer)
	{
		Destroy_StagingBuffer(p_allocator, m_upload_buffer);
	}

	if (m_p_command_pool)
	{
		// destroying the pool implicitly frees the command buffer allocated from it
		vkDestroyCommandPool(m_p_device, m_p_command_pool, nullptr);
		m_p_command_pool = nullptr;
		m_p_command_buffer = nullptr;
	}

	if (m_p_fence)
	{
		vkDestroyFence(m_p_device, m_p_fence, nullptr);
		m_p_fence = nullptr;
	}

	m_p_device = nullptr;
	m_p_graphics_queue = nullptr;
}

RenderInterface_VK::UploadResourceManager::upload_buffer_data_t RenderInterface_VK::UploadResourceManager::Create_StagingBuffer(
	VmaAllocator p_allocator, size_t requested_size)
{
	RMLUI_ZoneScopedN("Vulkan - UploadResourceManager::Create_StagingBuffer");

	upload_buffer_data_t result = {};
	Create_StagingBuffer(p_allocator, requested_size, &result);

	return result;
}

void RenderInterface_VK::UploadResourceManager::Destroy_StagingBuffer(VmaAllocator p_allocator, upload_buffer_data_t& data)
{
	RMLUI_ZoneScopedN("Vulkan - UploadResourceManager::Destroy_StagingBuffer");

	if (data.m_p_vk_buffer && data.m_p_vma_allocation)
	{
		vmaDestroyBuffer(p_allocator, data.m_p_vk_buffer, data.m_p_vma_allocation);
	}

	data = {};
}

void RenderInterface_VK::UploadResourceManager::Create_Fence() noexcept
{
	VkFenceCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	info.pNext = nullptr;
	info.flags = 0;

	VkResult status = vkCreateFence(m_p_device, &info, nullptr, &m_p_fence);
	RMLUI_VK_ASSERTMSG(status == VkResult::VK_SUCCESS, "failed to vkCreateFence");
}

void RenderInterface_VK::UploadResourceManager::Create_CommandBuffer() noexcept
{
	RMLUI_VK_ASSERTMSG(m_p_command_pool, "you must create the command pool first");

	VkCommandBufferAllocateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	info.pNext = nullptr;
	info.commandPool = m_p_command_pool;
	info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	info.commandBufferCount = 1;

	VkResult status = vkAllocateCommandBuffers(m_p_device, &info, &m_p_command_buffer);
	RMLUI_VK_ASSERTMSG(status == VkResult::VK_SUCCESS, "failed to vkAllocateCommandBuffers");
}

void RenderInterface_VK::UploadResourceManager::Create_StagingBuffer(VmaAllocator p_allocator, size_t requested_size,
	upload_buffer_data_t* init_buffer)
{
	RMLUI_VK_ASSERTMSG(init_buffer, "you must pass a valid pointer");

	VkBufferCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	info.pNext = nullptr;
	info.size = requested_size;
	info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo info_allocation = {};
	info_allocation.usage = VMA_MEMORY_USAGE_CPU_ONLY;

	VmaAllocationInfo info_stats = {};

	VkResult status =
		vmaCreateBuffer(p_allocator, &info, &info_allocation, &init_buffer->m_p_vk_buffer, &init_buffer->m_p_vma_allocation, &info_stats);
	RMLUI_VK_ASSERTMSG(status == VkResult::VK_SUCCESS, "failed to vmaCreateBuffer");

#ifdef RMLUI_VK_DEBUG
	Rml::Log::Message(Rml::Log::LT_DEBUG, "[Vulkan] Allocated staging buffer [%s]", FormatByteSize(info_stats.size).c_str());
#endif

	init_buffer->creation_size = requested_size;
}

void RenderInterface_VK::UploadResourceManager::Create_CommandPool(uint32_t queue_family_index) noexcept
{
	VkCommandPoolCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	info.pNext = nullptr;
	info.queueFamilyIndex = queue_family_index;
	// the one-time command buffer is re-recorded for every upload, which implicitly resets it
	info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	VkResult status = vkCreateCommandPool(m_p_device, &info, nullptr, &m_p_command_pool);
	RMLUI_VK_ASSERTMSG(status == VkResult::VK_SUCCESS, "failed to vkCreateCommandPool");
}

void RenderInterface_VK::UploadResourceManager::Create_All(uint32_t queue_family_index, VmaAllocator p_allocator, size_t staging_buffer_size) noexcept
{
	Create_CommandPool(queue_family_index);
	Create_CommandBuffer();
	Create_Fence();
	Create_StagingBuffer(p_allocator, staging_buffer_size, &m_upload_buffer);
}

void RenderInterface_VK::UploadResourceManager::Wait() noexcept
{
	RMLUI_VK_ASSERTMSG(m_p_fence, "you must initialize VkFence");

	constexpr uint64_t kMaxUint64 = std::numeric_limits<uint64_t>::max();

	VkResult status = vkWaitForFences(m_p_device, 1, &m_p_fence, VK_TRUE, kMaxUint64);
	RMLUI_VK_ASSERTMSG(status == VkResult::VK_SUCCESS, "failed to vkWaitForFences");

	status = vkResetFences(m_p_device, 1, &m_p_fence);
	RMLUI_VK_ASSERTMSG(status == VkResult::VK_SUCCESS, "failed to vkResetFences");
}

void RenderInterface_VK::UploadResourceManager::Submit() noexcept
{
	RMLUI_VK_ASSERTMSG(m_p_command_buffer, "you must initialize VkCommandBuffer");
	RMLUI_VK_ASSERTMSG(m_p_graphics_queue, "you must initialize VkQueue");

	VkSubmitInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	info.pNext = nullptr;
	info.commandBufferCount = 1;
	info.pCommandBuffers = &m_p_command_buffer;

	VkResult status = vkQueueSubmit(m_p_graphics_queue, 1, &info, m_p_fence);
	RMLUI_VK_ASSERTMSG(status == VkResult::VK_SUCCESS, "failed to vkQueueSubmit");
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

void RenderInterface_VK::DescriptorPoolManager::Initialize(VkDevice p_device, uint32_t count_uniform_buffer, uint32_t count_image_sampler,
	uint32_t count_sampler, uint32_t count_storage_buffer) noexcept
{
	RMLUI_VK_ASSERTMSG(p_device, "you must pass a valid VkDevice");
	RMLUI_VK_ASSERTMSG(m_p_descriptor_pool == nullptr, "already initialized");

	Rml::Array<VkDescriptorPoolSize, 4> pool_sizes = {};
	pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	pool_sizes[0].descriptorCount = count_uniform_buffer;
	pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	pool_sizes[1].descriptorCount = count_image_sampler;
	pool_sizes[2].type = VK_DESCRIPTOR_TYPE_SAMPLER;
	pool_sizes[2].descriptorCount = count_sampler;
	pool_sizes[3].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	pool_sizes[3].descriptorCount = count_storage_buffer;

	VkDescriptorPoolCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	info.pNext = nullptr;
	// individual sets are freed (textures, retired pool buffers, transient blend-mask sets)
	info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	info.maxSets = count_uniform_buffer + count_image_sampler + count_sampler + count_storage_buffer;
	info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
	info.pPoolSizes = pool_sizes.data();

	VkResult status = vkCreateDescriptorPool(p_device, &info, nullptr, &m_p_descriptor_pool);
	RMLUI_VK_ASSERTMSG(status == VkResult::VK_SUCCESS, "failed to vkCreateDescriptorPool");
}

void RenderInterface_VK::DescriptorPoolManager::Shutdown(VkDevice p_device)
{
	RMLUI_VK_ASSERTMSG(m_allocated_descriptor_count <= 0, "something is wrong. You didn't free some VkDescriptorSet");

	if (m_p_descriptor_pool)
	{
		vkDestroyDescriptorPool(p_device, m_p_descriptor_pool, nullptr);
		m_p_descriptor_pool = nullptr;
	}

	m_allocated_descriptor_count = 0;
}

bool RenderInterface_VK::DescriptorPoolManager::Alloc_Descriptor(VkDevice p_device, VkDescriptorSetLayout* p_layouts, VkDescriptorSet* p_sets,
	uint32_t descriptor_count_for_creation) noexcept
{
	RMLUI_VK_ASSERTMSG(p_device, "you must pass a valid VkDevice");
	RMLUI_VK_ASSERTMSG(m_p_descriptor_pool, "you must initialize the descriptor pool first");
	RMLUI_VK_ASSERTMSG(p_layouts, "you must pass valid layouts");
	RMLUI_VK_ASSERTMSG(p_sets, "you must pass a valid out pointer");
	RMLUI_VK_ASSERTMSG(descriptor_count_for_creation > 0, "count must be greater than zero");

	VkDescriptorSetAllocateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	info.pNext = nullptr;
	info.descriptorPool = m_p_descriptor_pool;
	info.descriptorSetCount = descriptor_count_for_creation;
	info.pSetLayouts = p_layouts;

	VkResult status = vkAllocateDescriptorSets(p_device, &info, p_sets);
	RMLUI_VK_ASSERTMSG(status == VkResult::VK_SUCCESS, "failed to vkAllocateDescriptorSets (the pool is exhausted?)");

	if (status != VK_SUCCESS)
		return false;

	m_allocated_descriptor_count += static_cast<int>(descriptor_count_for_creation);

	return true;
}

void RenderInterface_VK::DescriptorPoolManager::Free_Descriptors(VkDevice p_device, VkDescriptorSet* p_sets, uint32_t descriptor_count) noexcept
{
	RMLUI_VK_ASSERTMSG(p_device, "you must pass a valid VkDevice");
	RMLUI_VK_ASSERTMSG(m_p_descriptor_pool, "you must initialize the descriptor pool first");
	RMLUI_VK_ASSERTMSG(p_sets, "you must pass a valid pointer");
	RMLUI_VK_ASSERTMSG(descriptor_count > 0, "count must be greater than zero");

	VkResult status = vkFreeDescriptorSets(p_device, m_p_descriptor_pool, descriptor_count, p_sets);
	RMLUI_VK_ASSERTMSG(status == VkResult::VK_SUCCESS, "failed to vkFreeDescriptorSets");

	m_allocated_descriptor_count -= static_cast<int>(descriptor_count);
}

RenderInterface_VK::BufferMemoryManager::BufferMemoryManager() :
	m_p_device{}, m_p_allocator{}, m_p_manager_descriptors{}, m_p_set_layout_constant_buffer{}, m_constant_buffer_alignment{},
	m_size_for_allocation_in_bytes{}, m_frame_counter{}
{
	RMLUI_ZoneScopedN("Vulkan - BufferMemoryManager::Constructor");
}

RenderInterface_VK::BufferMemoryManager::~BufferMemoryManager()
{
	RMLUI_ZoneScopedN("Vulkan - BufferMemoryManager::Destructor");
}

bool RenderInterface_VK::BufferMemoryManager::Is_Initialized() const
{
	RMLUI_ZoneScopedN("Vulkan - BufferMemoryManager::Is_Initialized");
	return static_cast<bool>(m_p_device != nullptr);
}

void RenderInterface_VK::BufferMemoryManager::Initialize(VkDevice p_device, VmaAllocator p_allocator, DescriptorPoolManager* p_manager_descriptors,
	VkDescriptorSetLayout p_set_layout_constant_buffer, VkDeviceSize constant_buffer_alignment, size_t size_for_allocation)
{
	RMLUI_ZoneScopedN("Vulkan - BufferMemoryManager::Initialize");
	RMLUI_ASSERTMSG(p_device, "must be valid!");
	RMLUI_ASSERTMSG(p_allocator, "must be valid!");
	RMLUI_ASSERTMSG(p_manager_descriptors, "must be valid!");
	RMLUI_ASSERTMSG(p_set_layout_constant_buffer, "must be valid!");
	RMLUI_ASSERTMSG(size_for_allocation, "must be greater than 0");
	RMLUI_ASSERTMSG(constant_buffer_alignment, "must be greater than 0");

	m_p_device = p_device;
	m_p_allocator = p_allocator;
	m_p_manager_descriptors = p_manager_descriptors;
	m_p_set_layout_constant_buffer = p_set_layout_constant_buffer;
	m_constant_buffer_alignment = constant_buffer_alignment;
	m_size_for_allocation_in_bytes = size_for_allocation;
	m_frame_counter = 0;

	Alloc_Buffer(size_for_allocation);
}

void RenderInterface_VK::BufferMemoryManager::Shutdown()
{
	RMLUI_ZoneScopedN("Vulkan - BufferMemoryManager::Shutdown");
	RMLUI_VK_ASSERTMSG(m_p_device, "you must have a valid VkDevice here");

	for (size_t i = 0; i < m_buffers.size(); ++i)
	{
		if (m_buffers[i])
		{
			Destroy_BufferAtIndex(static_cast<int>(i));
		}
	}

	m_pending_for_deletion_buffers.clear();

	m_virtual_blocks.clear();
	m_buffers.clear();
	m_buffer_allocations.clear();
	m_buffers_mapped_memory.clear();
	m_cb_descriptor_sets.clear();

	m_p_device = nullptr;
	m_p_allocator = nullptr;
	m_p_manager_descriptors = nullptr;
}

void RenderInterface_VK::BufferMemoryManager::Alloc_Vertex(const void* p_data, int num_vertices, size_t size_of_one_element_in_p_data,
	GeometryHandleType* p_handle)
{
	RMLUI_ZoneScopedN("Vulkan - BufferMemoryManager::Alloc_Vertex");
	RMLUI_ASSERTMSG(p_data, "data for mapping to buffer must valid!");
	RMLUI_ASSERTMSG(num_vertices, "amount of vertices must be greater than zero!");
	RMLUI_ASSERTMSG(size_of_one_element_in_p_data > 0, "size of one element must be greater than 0");
	RMLUI_ASSERTMSG(p_handle, "must be valid!");

	if (p_handle)
	{
		RMLUI_ASSERT(p_handle->Get_InfoVertex().buffer_index == -1 &&
			"info is already initialized that means you didn't destroy your buffer! Something is wrong!");

		p_handle->Set_NumVertices(num_vertices);
		p_handle->Set_SizeOfOneVertex(size_of_one_element_in_p_data);

		GraphicsAllocationInfo info;
		Alloc(info, num_vertices * size_of_one_element_in_p_data);

		void* p_writable_part = Get_WritableMemoryFromBufferByOffset(info);

		RMLUI_ASSERTMSG(p_writable_part, "something is wrong!");

		if (p_writable_part)
		{
			std::memcpy(p_writable_part, p_data, info.size);
		}

		p_handle->Set_InfoVertex(info);
	}
}

void RenderInterface_VK::BufferMemoryManager::Alloc_Index(const void* p_data, int num_indices, size_t size_of_one_element_in_p_data,
	GeometryHandleType* p_handle)
{
	RMLUI_ZoneScopedN("Vulkan - BufferMemoryManager::Alloc_Index");
	RMLUI_ASSERTMSG(p_data, "data for mapping to buffer must valid!");
	RMLUI_ASSERTMSG(num_indices, "amount of indices must be greater than zero!");
	RMLUI_ASSERTMSG(size_of_one_element_in_p_data > 0, "size of one element must be greater than 0");
	RMLUI_ASSERTMSG(p_handle, "must be valid!");

	if (p_handle)
	{
		RMLUI_ASSERT(p_handle->Get_InfoIndex().buffer_index == -1 &&
			"info is already initialized that means you didn't destroy your buffer! Something is wrong!");

		p_handle->Set_NumIndices(num_indices);
		p_handle->Set_SizeOfOneIndex(size_of_one_element_in_p_data);

		GraphicsAllocationInfo info;
		Alloc(info, num_indices * size_of_one_element_in_p_data);

		void* p_writable_part = Get_WritableMemoryFromBufferByOffset(info);

		RMLUI_ASSERTMSG(p_writable_part, "something is wrong!");

		if (p_writable_part)
		{
			std::memcpy(p_writable_part, p_data, info.size);
		}

		p_handle->Set_InfoIndex(info);
	}
}

RenderInterface_VK::GraphicsAllocationInfo RenderInterface_VK::BufferMemoryManager::Alloc_ConstantBuffer(ConstantBufferType* p_resource, size_t size)
{
	RMLUI_ZoneScopedN("Vulkan - BufferMemoryManager::Alloc_ConstantBuffer");
	RMLUI_ASSERTMSG(!m_buffers.empty(), "you forgot to allocate buffer on initialize stage of this manager!");
	RMLUI_ASSERTMSG(p_resource, "must be valid!");

	GraphicsAllocationInfo result;
	auto result_index = Alloc(result, size, static_cast<size_t>(m_constant_buffer_alignment));

	if (p_resource)
	{
		if (result_index != -1)
		{
			p_resource->m_p_gpu_start_memory_for_binding_data = m_buffers_mapped_memory.at(result_index);
		}
	}

	return result;
}

void RenderInterface_VK::BufferMemoryManager::Free_ConstantBuffer(ConstantBufferType* p_constantbuffer)
{
	RMLUI_ZoneScopedN("Vulkan - BufferMemoryManager::Free_ConstantBuffer");
	RMLUI_ASSERTMSG(p_constantbuffer, "you must pass a valid object!");

	if (p_constantbuffer)
	{
		const auto& info = p_constantbuffer->m_alloc_info;
		RMLUI_ASSERTMSG(info.buffer_index != -1, "must be valid data of this info");

		if (m_virtual_blocks.empty() == false)
		{
			if (VmaVirtualBlock p_block = m_virtual_blocks.at(info.buffer_index))
			{
				vmaVirtualFree(p_block, info.alloc_info);
				p_constantbuffer->m_alloc_info = {};
				p_constantbuffer->m_p_gpu_start_memory_for_binding_data = nullptr;
			}
		}
	}
}

void RenderInterface_VK::BufferMemoryManager::Free_Geometry(GeometryHandleType* p_handle)
{
	RMLUI_ZoneScopedN("Vulkan - BufferMemoryManager::Free_Geometry");
	RMLUI_ASSERTMSG(p_handle, "must be valid!");
	RMLUI_ASSERTMSG(p_handle->Get_InfoVertex().buffer_index != -1, "not initialized, maybe you passing twice for deallocation?");
	RMLUI_ASSERTMSG(p_handle->Get_InfoIndex().buffer_index != -1, "not initialized, maybe you passing twice for deallocation?");

	if (p_handle)
	{
		const auto& info_vertex = p_handle->Get_InfoVertex();
		const auto& info_index = p_handle->Get_InfoIndex();

		if (m_virtual_blocks.empty() == false)
		{
			VmaVirtualBlock p_block_vertex = m_virtual_blocks.at(info_vertex.buffer_index);
			VmaVirtualBlock p_block_index = m_virtual_blocks.at(info_index.buffer_index);

			if (p_block_vertex)
			{
				vmaVirtualFree(p_block_vertex, info_vertex.alloc_info);

				GraphicsAllocationInfo invalidate;
				p_handle->Set_InfoVertex(invalidate);
			}

			if (p_block_index)
			{
				vmaVirtualFree(p_block_index, info_index.alloc_info);

				GraphicsAllocationInfo invalidate;
				p_handle->Set_InfoIndex(invalidate);
			}
		}
	}
}

void* RenderInterface_VK::BufferMemoryManager::Get_WritableMemoryFromBufferByOffset(const GraphicsAllocationInfo& info)
{
	RMLUI_ZoneScopedN("Vulkan - BufferMemoryManager::Get_WritableMemoryFromBufferByOffset");
	RMLUI_ASSERTMSG(info.buffer_index != -1, "you pass not initialized graphics allocation info!");

	void* p_result{};

	if (info.buffer_index != -1)
	{
		std::uint8_t* p_begin = reinterpret_cast<std::uint8_t*>(m_buffers_mapped_memory.at(info.buffer_index));

		RMLUI_ASSERTMSG(p_begin, "being pointer is not valid! it's terribly wrong thing!!!!");

		p_result = p_begin + info.offset;
	}

	return p_result;
}

VkBuffer RenderInterface_VK::BufferMemoryManager::Get_BufferByIndex(int buffer_index)
{
	RMLUI_ZoneScopedN("Vulkan - BufferMemoryManager::Get_BufferByIndex");
	RMLUI_ASSERTMSG(buffer_index >= 0, "index must be valid!");
	RMLUI_ASSERTMSG(buffer_index < static_cast<int>(m_buffers.size()), "overflow index!");

	VkBuffer p_result{};

	if (buffer_index >= 0)
	{
		if (buffer_index < static_cast<int>(m_buffers.size()))
		{
			p_result = m_buffers.at(buffer_index);
		}
	}

	return p_result;
}

VkDescriptorSet RenderInterface_VK::BufferMemoryManager::Get_ConstantBufferDescriptorSetByIndex(int buffer_index)
{
	RMLUI_ZoneScopedN("Vulkan - BufferMemoryManager::Get_ConstantBufferDescriptorSetByIndex");
	RMLUI_ASSERTMSG(buffer_index >= 0, "index must be valid!");
	RMLUI_ASSERTMSG(buffer_index < static_cast<int>(m_cb_descriptor_sets.size()), "overflow index!");

	VkDescriptorSet p_result{};

	if (buffer_index >= 0)
	{
		if (buffer_index < static_cast<int>(m_cb_descriptor_sets.size()))
		{
			p_result = m_cb_descriptor_sets.at(buffer_index);
		}
	}

	return p_result;
}

void RenderInterface_VK::BufferMemoryManager::Update_PendingForDeletion_Buffers(uint64_t frame_counter)
{
	RMLUI_ZoneScopedN("Vulkan - BufferMemoryManager::Update_PendingForDeletion_Buffers");

	m_frame_counter = frame_counter;

	for (auto it = m_pending_for_deletion_buffers.begin(); it != m_pending_for_deletion_buffers.end();)
	{
		if (frame_counter - it->second >= uint64_t(RenderInterface_VK::kSwapchainBackBufferCount))
		{
			Destroy_BufferAtIndex(it->first);
			it = m_pending_for_deletion_buffers.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void RenderInterface_VK::BufferMemoryManager::Alloc_Buffer(size_t size)
{
	RMLUI_ZoneScopedN("Vulkan - BufferMemoryManager::Alloc_Buffer");
	RMLUI_ASSERTMSG(size, "must be greater than 0");

	VkBufferCreateInfo info_buffer = {};
	info_buffer.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	info_buffer.pNext = nullptr;
	info_buffer.size = size;
	info_buffer.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	info_buffer.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	// persistently mapped host-visible memory (the analog of the DX12 renderer's UPLOAD heap)
	VmaAllocationCreateInfo info_allocation = {};
	info_allocation.usage = VMA_MEMORY_USAGE_AUTO;
	info_allocation.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	info_allocation.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	info_allocation.preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	VkBuffer p_buffer = nullptr;
	VmaAllocation p_allocation = nullptr;
	VmaAllocationInfo info_stats = {};

	VkResult status = vmaCreateBuffer(m_p_allocator, &info_buffer, &info_allocation, &p_buffer, &p_allocation, &info_stats);
	RMLUI_VK_ASSERTMSG(status == VkResult::VK_SUCCESS, "failed to vmaCreateBuffer");
	RMLUI_VK_ASSERTMSG(info_stats.pMappedData, "the pool buffer must be persistently mapped");

	VmaVirtualBlockCreateInfo info_virtual_block = {};
	info_virtual_block.size = size;
	info_virtual_block.flags = 0;

	VmaVirtualBlock p_block = nullptr;
	status = vmaCreateVirtualBlock(&info_virtual_block, &p_block);
	RMLUI_VK_ASSERTMSG(status == VkResult::VK_SUCCESS, "failed to vmaCreateVirtualBlock");

	// each pool buffer owns one descriptor set of the constant-buffer layout (bound per draw with a dynamic offset, the
	// analog of DX12's root constant buffer views)
	VkDescriptorSet p_cb_set = nullptr;
	bool is_allocated = m_p_manager_descriptors->Alloc_Descriptor(m_p_device, &m_p_set_layout_constant_buffer, &p_cb_set);
	RMLUI_VK_ASSERTMSG(is_allocated && p_cb_set, "failed to allocate the constant buffer descriptor set");

	VkDescriptorBufferInfo info_descriptor_buffer = {};
	info_descriptor_buffer.buffer = p_buffer;
	info_descriptor_buffer.offset = 0;
	info_descriptor_buffer.range = kAllocationSizeMax_ConstantBuffer;

	VkWriteDescriptorSet info_write = {};
	info_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	info_write.pNext = nullptr;
	info_write.dstSet = p_cb_set;
	info_write.descriptorCount = 1;
	info_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	info_write.dstBinding = 0;
	info_write.dstArrayElement = 0;
	info_write.pBufferInfo = &info_descriptor_buffer;

	vkUpdateDescriptorSets(m_p_device, 1, &info_write, 0, nullptr);

	// reuse a slot that was freed by the deferred destruction, otherwise append a new one
	int result_index = -1;
	for (size_t i = 0; i < m_buffers.size(); ++i)
	{
		if (m_buffers[i] == nullptr)
		{
			result_index = static_cast<int>(i);
			break;
		}
	}

	if (result_index == -1)
	{
		result_index = static_cast<int>(m_buffers.size());

		m_buffers.push_back(nullptr);
		m_buffer_allocations.push_back(nullptr);
		m_buffers_mapped_memory.push_back(nullptr);
		m_virtual_blocks.push_back(nullptr);
		m_cb_descriptor_sets.push_back(nullptr);
	}

	m_buffers[result_index] = p_buffer;
	m_buffer_allocations[result_index] = p_allocation;
	m_buffers_mapped_memory[result_index] = info_stats.pMappedData;
	m_virtual_blocks[result_index] = p_block;
	m_cb_descriptor_sets[result_index] = p_cb_set;

#ifdef RMLUI_VK_DEBUG
	Rml::Log::Message(Rml::Log::LT_DEBUG, "[Vulkan] Allocated buffer pool block [%d] [%s]", result_index, FormatByteSize(info_stats.size).c_str());
#endif
}

VmaVirtualBlock_T* RenderInterface_VK::BufferMemoryManager::Get_AvailableBlock(size_t size_for_allocation, int* p_result_buffer_index)
{
	RMLUI_ZoneScopedN("Vulkan - BufferMemoryManager::Get_AvailableBlock");
	RMLUI_ASSERTMSG(p_result_buffer_index, "must be valid part of memory!");

	VmaVirtualBlock p_result{};

	int index{};

	for (auto& p_block : m_virtual_blocks)
	{
		if (p_block)
		{
			bool is_pending_for_deletion = false;
			for (const auto& pair : m_pending_for_deletion_buffers)
			{
				if (pair.first == index)
				{
					is_pending_for_deletion = true;
					break;
				}
			}

			if (!is_pending_for_deletion)
			{
				VmaStatistics stats = {};
				vmaGetVirtualBlockStatistics(p_block, &stats);

				if ((stats.blockBytes - stats.allocationBytes) >= size_for_allocation)
				{
					p_result = p_block;
					*p_result_buffer_index = index;
					break;
				}
			}
		}
		++index;
	}

	return p_result;
}

VmaVirtualBlock_T* RenderInterface_VK::BufferMemoryManager::Get_NotOutOfMemoryAndAvailableBlock(size_t size_for_allocation,
	int* p_result_buffer_index)
{
	RMLUI_ZoneScopedN("Vulkan - BufferMemoryManager::Get_NotOutOfMemoryAndAvailableBlock");
	RMLUI_ASSERTMSG(p_result_buffer_index, "must be valid part of memory!");
	RMLUI_ASSERTMSG(*p_result_buffer_index != -1,
		"use this method when you found of available block then tried to allocate from it but got out of memory status!");

	VmaVirtualBlock p_result{};

	// we skip out of memory block since it shows to us as available
	auto from = *p_result_buffer_index + 1;

	for (int i = from; i < static_cast<int>(m_virtual_blocks.size()); ++i)
	{
		auto& p_block = m_virtual_blocks.at(i);
		if (p_block)
		{
			bool is_pending_for_deletion = false;
			for (const auto& pair : m_pending_for_deletion_buffers)
			{
				if (pair.first == i)
				{
					is_pending_for_deletion = true;
					break;
				}
			}

			if (!is_pending_for_deletion)
			{
				VmaStatistics stats = {};
				vmaGetVirtualBlockStatistics(p_block, &stats);

				if ((stats.blockBytes - stats.allocationBytes) >= size_for_allocation)
				{
					p_result = p_block;
					*p_result_buffer_index = i;
					break;
				}
			}
		}
	}

	return p_result;
}

int RenderInterface_VK::BufferMemoryManager::Alloc(GraphicsAllocationInfo& info, size_t size, size_t alignment)
{
	RMLUI_ZoneScopedN("Vulkan - BufferMemoryManager::Alloc");
	RMLUI_ASSERTMSG(!m_buffers.empty(), "you forgot to allocate buffer on initialize stage of this manager!");

	// we don't want to use any recursions because it is slow af
	constexpr int kHowManyRequestsWeCanDoForResolvingOutOfMemory = 15;
	int result_index{-1};

	if (alignment > 0)
		size = AlignUp(size, alignment);

	VmaVirtualBlock p_block = Get_AvailableBlock(size, &result_index);

	if (p_block == nullptr)
	{
		if (size > m_size_for_allocation_in_bytes)
		{
#ifdef RMLUI_VK_DEBUG
			Rml::Log::Message(Rml::Log::Type::LT_DEBUG, "[Vulkan] auto correction size for buffer from [%zu] to [%zu]",
				m_size_for_allocation_in_bytes, size);
#endif

			m_size_for_allocation_in_bytes = size;
		}

		Alloc_Buffer(m_size_for_allocation_in_bytes);

		result_index = -1;
		p_block = Get_AvailableBlock(size, &result_index);

		RMLUI_ASSERTMSG(p_block, "can't be because in previous line of code you added new allocated fresh buffer!");
	}

	VmaVirtualAllocationCreateInfo desc_alloc = {};
	desc_alloc.size = size;
	desc_alloc.alignment = alignment;

	VmaVirtualAllocation p_alloc = VK_NULL_HANDLE;
	VkDeviceSize offset = 0;

	VkResult status = vmaVirtualAllocate(p_block, &desc_alloc, &p_alloc, &offset);

	// on failure (fragmented block) scan the other blocks, growing the pool when nothing fits
	for (int i = 0; status != VK_SUCCESS && i < kHowManyRequestsWeCanDoForResolvingOutOfMemory; ++i)
	{
		p_block = Get_NotOutOfMemoryAndAvailableBlock(size, &result_index);

		if (!p_block)
		{
			if (size > m_size_for_allocation_in_bytes)
			{
#ifdef RMLUI_VK_DEBUG
				Rml::Log::Message(Rml::Log::Type::LT_DEBUG, "[Vulkan] auto correction size for buffer from [%zu] to [%zu]",
					m_size_for_allocation_in_bytes, size);
#endif

				m_size_for_allocation_in_bytes = size;
			}

			Alloc_Buffer(m_size_for_allocation_in_bytes);

			result_index = -1;
			p_block = Get_AvailableBlock(size, &result_index);

			RMLUI_ASSERTMSG(p_block, "can't be because in previous line of code you added new allocated fresh buffer!");
		}

		status = vmaVirtualAllocate(p_block, &desc_alloc, &p_alloc, &offset);
	}

	RMLUI_VK_ASSERTMSG(status == VK_SUCCESS, "failed to suballocate memory from the buffer pool (out of memory)");

	if (status == VK_SUCCESS)
	{
		info.size = size;
		info.alloc_info = p_alloc;
		info.offset = offset;
		info.buffer_index = result_index;
	}

	TryToFreeAvailableBlock(m_frame_counter);

	return result_index;
}

void RenderInterface_VK::BufferMemoryManager::TryToFreeAvailableBlock(uint64_t frame_counter)
{
	RMLUI_ZoneScopedN("Vulkan - BufferMemoryManager::TryToFreeAvailableBlock");

	// moves at most one completely empty block to the deferred destruction list (like the DX12 renderer)
	int index{};

	for (auto& p_block : m_virtual_blocks)
	{
		if (p_block)
		{
			bool is_pending_for_deletion = false;
			for (const auto& pair : m_pending_for_deletion_buffers)
			{
				if (pair.first == index)
				{
					is_pending_for_deletion = true;
					break;
				}
			}

			if (!is_pending_for_deletion)
			{
				VmaStatistics stats = {};
				vmaGetVirtualBlockStatistics(p_block, &stats);

				if (stats.allocationCount == 0)
				{
					// the block's resources stay alive (the slots are not nulled) until the GPU can't reference them
					// anymore; only then Update_PendingForDeletion_Buffers destroys them
					m_pending_for_deletion_buffers.push_back({index, frame_counter});
					break;
				}
			}
		}
		++index;
	}
}

void RenderInterface_VK::BufferMemoryManager::Destroy_BufferAtIndex(int buffer_index)
{
	RMLUI_ZoneScopedN("Vulkan - BufferMemoryManager::Destroy_BufferAtIndex");
	RMLUI_VK_ASSERTMSG(buffer_index >= 0 && buffer_index < static_cast<int>(m_buffers.size()), "invalid index!");

	if (buffer_index < 0 || buffer_index >= static_cast<int>(m_buffers.size()))
		return;

	if (m_cb_descriptor_sets.at(buffer_index))
	{
		VkDescriptorSet p_set = m_cb_descriptor_sets.at(buffer_index);
		m_p_manager_descriptors->Free_Descriptors(m_p_device, &p_set);
		m_cb_descriptor_sets.at(buffer_index) = nullptr;
	}

	if (m_virtual_blocks.at(buffer_index))
	{
		vmaDestroyVirtualBlock(m_virtual_blocks.at(buffer_index));
		m_virtual_blocks.at(buffer_index) = nullptr;
	}

	if (m_buffers.at(buffer_index))
	{
		vmaDestroyBuffer(m_p_allocator, m_buffers.at(buffer_index), m_buffer_allocations.at(buffer_index));
		m_buffers.at(buffer_index) = nullptr;
		m_buffer_allocations.at(buffer_index) = nullptr;
		m_buffers_mapped_memory.at(buffer_index) = nullptr;
	}
}

RenderInterface_VK::TextureMemoryManager::TextureMemoryManager() :
	m_p_renderer{}, m_p_device{}, m_p_allocator{}, m_p_upload_manager{}, m_p_manager_descriptors{}, m_p_set_layout_texture{}, m_p_sampler_linear{}
{
	RMLUI_ZoneScopedN("Vulkan - TextureMemoryManager::Constructor");
}

RenderInterface_VK::TextureMemoryManager::~TextureMemoryManager()
{
	RMLUI_ZoneScopedN("Vulkan - TextureMemoryManager::Destructor");
}

void RenderInterface_VK::TextureMemoryManager::Initialize(RenderInterface_VK* p_renderer, VkDevice p_device, VmaAllocator p_allocator,
	UploadResourceManager* p_upload_manager, DescriptorPoolManager* p_manager_descriptors, VkDescriptorSetLayout p_set_layout_texture,
	VkSampler p_sampler)
{
	RMLUI_ZoneScopedN("Vulkan - TextureMemoryManager::Initialize");
	RMLUI_ASSERTMSG(p_renderer, "must be valid!");
	RMLUI_ASSERTMSG(p_device, "must be valid!");
	RMLUI_ASSERTMSG(p_allocator, "must be valid!");
	RMLUI_ASSERTMSG(p_upload_manager, "must be valid!");
	RMLUI_ASSERTMSG(p_manager_descriptors, "must be valid!");
	RMLUI_ASSERTMSG(p_set_layout_texture, "must be valid!");
	RMLUI_ASSERTMSG(p_sampler, "must be valid!");

	m_p_renderer = p_renderer;
	m_p_device = p_device;
	m_p_allocator = p_allocator;
	m_p_upload_manager = p_upload_manager;
	m_p_manager_descriptors = p_manager_descriptors;
	m_p_set_layout_texture = p_set_layout_texture;
	m_p_sampler_linear = p_sampler;
}

void RenderInterface_VK::TextureMemoryManager::Shutdown()
{
	RMLUI_ZoneScopedN("Vulkan - TextureMemoryManager::Shutdown");

	m_p_renderer = nullptr;
	m_p_device = nullptr;
	m_p_allocator = nullptr;
	m_p_upload_manager = nullptr;
	m_p_manager_descriptors = nullptr;
	m_p_set_layout_texture = nullptr;
	m_p_sampler_linear = nullptr;
}

bool RenderInterface_VK::TextureMemoryManager::Is_Initialized() const
{
	RMLUI_ZoneScopedN("Vulkan - TextureMemoryManager::Is_Initialized");
	return static_cast<bool>(m_p_device != nullptr);
}

void RenderInterface_VK::TextureMemoryManager::Alloc_Texture(TextureHandleType* p_impl, Rml::Vector2i dimensions, const Rml::byte* p_data
#ifdef RMLUI_VK_DEBUG
	,
	const Rml::String& debug_name
#endif
)
{
	RMLUI_ZoneScopedN("Vulkan - TextureMemoryManager::Alloc_Texture");
	RMLUI_ASSERTMSG(p_impl, "you must pass a valid texture handle");
	RMLUI_ASSERTMSG(dimensions.x > 0 && dimensions.y > 0, "invalid dimensions");

	VkImageCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	info.pNext = nullptr;
	info.imageType = VK_IMAGE_TYPE_2D;
	info.format = RMLUI_RENDER_BACKEND_FIELD_COLOR_TEXTURE_FORMAT;
	info.extent = {static_cast<uint32_t>(dimensions.x), static_cast<uint32_t>(dimensions.y), 1};
	info.mipLevels = 1;
	info.arrayLayers = 1;
	info.samples = VK_SAMPLE_COUNT_1_BIT;
	info.tiling = VK_IMAGE_TILING_OPTIMAL;
	// TRANSFER_SRC/DST so the texture can be copied into (upload) and out of (SaveLayerAsTexture)
	info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo info_allocation = {};
	info_allocation.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	VkImage p_image = nullptr;
	VmaAllocation p_allocation = nullptr;
	VmaAllocationInfo info_stats = {};

	VkResult status = vmaCreateImage(m_p_allocator, &info, &info_allocation, &p_image, &p_allocation, &info_stats);
	RMLUI_VK_ASSERTMSG(status == VkResult::VK_SUCCESS, "failed to vmaCreateImage");

#ifdef RMLUI_VK_DEBUG
	Rml::Log::Message(Rml::Log::LT_DEBUG, "Created texture '%s' [%dx%d, %s]", debug_name.c_str(), dimensions.x, dimensions.y,
		FormatByteSize(info_stats.size).c_str());

	vmaSetAllocationName(m_p_allocator, p_allocation, debug_name.c_str());
	p_impl->Set_ResourceName(debug_name);
#endif

	p_impl->Set_Image(p_image);
	p_impl->Set_Allocation(p_allocation);
	p_impl->Set_Width(dimensions.x);
	p_impl->Set_Height(dimensions.y);
	p_impl->Set_Layout(VK_IMAGE_LAYOUT_UNDEFINED);

	if (p_data)
	{
		Upload(p_impl, dimensions, p_data);
	}
	else
	{
		// no content: initialize the image to transparent black (a bare UNDEFINED -> SHADER_READ transition would
		// leave discarded contents in a read-only layout, which the validation layers flag, and would make the
		// sampled result nondeterministic anyway)
		m_p_upload_manager->UploadToGPU([p_image](VkCommandBuffer p_cmd) {
			VkImageMemoryBarrier info_barrier = {};
			info_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			info_barrier.pNext = nullptr;
			info_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			info_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			info_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			info_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			info_barrier.image = p_image;
			info_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			info_barrier.subresourceRange.baseMipLevel = 0;
			info_barrier.subresourceRange.levelCount = 1;
			info_barrier.subresourceRange.baseArrayLayer = 0;
			info_barrier.subresourceRange.layerCount = 1;
			info_barrier.srcAccessMask = 0;
			info_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			vkCmdPipelineBarrier(p_cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1,
				&info_barrier);

			const VkClearColorValue clear_color = {{0.0f, 0.0f, 0.0f, 0.0f}};
			vkCmdClearColorImage(p_cmd, p_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_color, 1, &info_barrier.subresourceRange);

			info_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			info_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			info_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			info_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(p_cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
				&info_barrier);
		});

		p_impl->Set_Layout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	Create_ImageView(p_impl, RMLUI_RENDER_BACKEND_FIELD_COLOR_TEXTURE_FORMAT, VK_IMAGE_ASPECT_COLOR_BIT, VK_SAMPLE_COUNT_1_BIT);
	Create_DescriptorSet_ForTexture(p_impl);
}

void RenderInterface_VK::TextureMemoryManager::Alloc_Texture(TextureHandleType* p_impl, Rml::Vector2i dimensions, VkFormat format,
	VkSampleCountFlagBits sample_count, bool is_depth_stencil
#ifdef RMLUI_VK_DEBUG
	,
	const Rml::String& debug_name
#endif
)
{
	RMLUI_ZoneScopedN("Vulkan - TextureMemoryManager::Alloc_Texture(RT)");
	RMLUI_ASSERTMSG(p_impl, "you must pass a valid texture handle");
	RMLUI_ASSERTMSG(dimensions.x > 0 && dimensions.y > 0, "invalid dimensions");

	VkImageCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	info.pNext = nullptr;
	info.imageType = VK_IMAGE_TYPE_2D;
	info.format = format;
	info.extent = {static_cast<uint32_t>(dimensions.x), static_cast<uint32_t>(dimensions.y), 1};
	info.mipLevels = 1;
	info.arrayLayers = 1;
	info.samples = sample_count;
	info.tiling = VK_IMAGE_TILING_OPTIMAL;

	if (is_depth_stencil)
	{
		info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	}
	else
	{
		info.usage =
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	}

	info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo info_allocation = {};
	info_allocation.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	VkImage p_image = nullptr;
	VmaAllocation p_allocation = nullptr;
	VmaAllocationInfo info_stats = {};

	VkResult status = vmaCreateImage(m_p_allocator, &info, &info_allocation, &p_image, &p_allocation, &info_stats);
	RMLUI_VK_ASSERTMSG(status == VkResult::VK_SUCCESS, "failed to vmaCreateImage");

#ifdef RMLUI_VK_DEBUG
	Rml::Log::Message(Rml::Log::LT_DEBUG, "Created render-target texture '%s' [%dx%d, %s]", debug_name.c_str(), dimensions.x, dimensions.y,
		FormatByteSize(info_stats.size).c_str());

	vmaSetAllocationName(m_p_allocator, p_allocation, debug_name.c_str());
	p_impl->Set_ResourceName(debug_name);
#endif

	p_impl->Set_Image(p_image);
	p_impl->Set_Allocation(p_allocation);
	p_impl->Set_Width(dimensions.x);
	p_impl->Set_Height(dimensions.y);
	p_impl->Set_Layout(VK_IMAGE_LAYOUT_UNDEFINED);

	VkImageAspectFlags aspect_mask = {};
	VkImageLayout target_layout = {};

	if (is_depth_stencil)
	{
		aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT;

		if (format >= VK_FORMAT_D16_UNORM_S8_UINT)
		{
			aspect_mask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}

		target_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	}
	else
	{
		aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;
		target_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}

	Create_ImageView(p_impl, format, aspect_mask, sample_count);

	// move the image into its attachment layout (one-shot, synchronous)
	m_p_upload_manager->UploadToGPU([p_image, aspect_mask, target_layout, is_depth_stencil](VkCommandBuffer p_cmd) {
		VkImageMemoryBarrier info_barrier = {};
		info_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		info_barrier.pNext = nullptr;
		info_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		info_barrier.newLayout = target_layout;
		info_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		info_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		info_barrier.image = p_image;
		info_barrier.subresourceRange.aspectMask = aspect_mask;
		info_barrier.subresourceRange.baseMipLevel = 0;
		info_barrier.subresourceRange.levelCount = 1;
		info_barrier.subresourceRange.baseArrayLayer = 0;
		info_barrier.subresourceRange.layerCount = 1;
		info_barrier.srcAccessMask = 0;

		VkPipelineStageFlags dst_stage = {};

		if (is_depth_stencil)
		{
			info_barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
			dst_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		}
		else
		{
			info_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
			dst_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		}

		vkCmdPipelineBarrier(p_cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, dst_stage, 0, 0, nullptr, 0, nullptr, 1, &info_barrier);
	});

	p_impl->Set_Layout(target_layout);

	// color render targets are sampled later (filters, the final composite, ...), so they get a descriptor set;
	// depth-stencil textures are never sampled
	if (!is_depth_stencil)
	{
		Create_DescriptorSet_ForTexture(p_impl);
	}
}

void RenderInterface_VK::TextureMemoryManager::Free_Texture(TextureHandleType* p_texture)
{
	RMLUI_ZoneScopedN("Vulkan - TextureMemoryManager::Free_Texture");
	RMLUI_VK_ASSERTMSG(m_p_allocator, "you must have initialized VmaAllocator");
	RMLUI_VK_ASSERTMSG(m_p_device, "you must have initialized VkDevice");

	if (p_texture == nullptr)
		return;

	VkDescriptorSet p_set = p_texture->Get_DescriptorSet();

	if (p_set)
	{
		m_p_manager_descriptors->Free_Descriptors(m_p_device, &p_set);
		p_texture->Set_DescriptorSet(nullptr);
	}

	if (p_texture->Get_ImageView())
	{
		vkDestroyImageView(m_p_device, p_texture->Get_ImageView(), nullptr);
		p_texture->Set_ImageView(nullptr);
	}

	if (p_texture->Get_Image() && p_texture->Get_Allocation())
	{
		vmaDestroyImage(m_p_allocator, p_texture->Get_Image(), p_texture->Get_Allocation());
	}

	p_texture->Set_Image(nullptr);
	p_texture->Set_Allocation(nullptr);
	p_texture->Set_Layout(VK_IMAGE_LAYOUT_UNDEFINED);

	// the handle is dead from here on; heap handles are deleted right after this call by the caller
	p_texture->Mark_Destroyed();
}

void RenderInterface_VK::TextureMemoryManager::Create_ImageView(TextureHandleType* p_impl, VkFormat format, VkImageAspectFlags aspect_mask,
	VkSampleCountFlagBits sample_count)
{
	RMLUI_ZoneScopedN("Vulkan - TextureMemoryManager::Create_ImageView");
	RMLUI_VK_ASSERTMSG(p_impl, "you must pass a valid texture handle");
	RMLUI_VK_ASSERTMSG(p_impl->Get_Image(), "you must create the image first");
	(void)sample_count; // image views don't carry the sample count; kept for parity with the allocation parameters

	VkImageViewCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	info.pNext = nullptr;
	info.image = p_impl->Get_Image();
	info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	info.format = format;
	info.components.r = VK_COMPONENT_SWIZZLE_R;
	info.components.g = VK_COMPONENT_SWIZZLE_G;
	info.components.b = VK_COMPONENT_SWIZZLE_B;
	info.components.a = VK_COMPONENT_SWIZZLE_A;
	info.subresourceRange.baseMipLevel = 0;
	info.subresourceRange.levelCount = 1;
	info.subresourceRange.baseArrayLayer = 0;
	info.subresourceRange.layerCount = 1;
	info.subresourceRange.aspectMask = aspect_mask;

	VkImageView p_image_view = nullptr;
	VkResult status = vkCreateImageView(m_p_device, &info, nullptr, &p_image_view);

	RMLUI_VK_ASSERTMSG(status == VkResult::VK_SUCCESS, "failed to vkCreateImageView");

	p_impl->Set_ImageView(p_image_view);
}

void RenderInterface_VK::TextureMemoryManager::Create_DescriptorSet_ForTexture(TextureHandleType* p_impl)
{
	RMLUI_ZoneScopedN("Vulkan - TextureMemoryManager::Create_DescriptorSet_ForTexture");
	RMLUI_VK_ASSERTMSG(p_impl, "you must pass a valid texture handle");
	RMLUI_VK_ASSERTMSG(p_impl->Get_ImageView(), "you must create the image view first");

	VkDescriptorSet p_set = nullptr;
	bool is_allocated = m_p_manager_descriptors->Alloc_Descriptor(m_p_device, &m_p_set_layout_texture, &p_set);
	RMLUI_VK_ASSERTMSG(is_allocated && p_set, "failed to allocate the texture descriptor set");

	VkDescriptorImageInfo info_image = {};
	info_image.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	info_image.imageView = p_impl->Get_ImageView();
	info_image.sampler = m_p_sampler_linear;

	VkWriteDescriptorSet info_write = {};
	info_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	info_write.pNext = nullptr;
	info_write.dstSet = p_set;
	info_write.descriptorCount = 1;
	info_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	info_write.dstBinding = 0;
	info_write.dstArrayElement = 0;
	info_write.pImageInfo = &info_image;

	vkUpdateDescriptorSets(m_p_device, 1, &info_write, 0, nullptr);

	p_impl->Set_DescriptorSet(p_set);
}

void RenderInterface_VK::TextureMemoryManager::Upload(TextureHandleType* p_texture, Rml::Vector2i dimensions, const Rml::byte* p_data)
{
	RMLUI_ZoneScopedN("Vulkan - TextureMemoryManager::Upload");
	RMLUI_VK_ASSERTMSG(p_texture, "must be valid!");
	RMLUI_VK_ASSERTMSG(p_data, "must be valid!");

	const size_t image_size = static_cast<size_t>(dimensions.x) * static_cast<size_t>(dimensions.y) * 4;

	// the cached staging buffer is reused unless the request exceeds it; upload_buffer_data_t is private to
	// UploadResourceManager, so it is only ever used through 'auto' here
	const auto* p_staging_buffer = &(m_p_upload_manager->Get_UploadBuffer());
	auto temp_staging_buffer = m_p_upload_manager->Get_UploadBuffer();
	bool is_temp_staging = false;

#if RMLUI_RENDER_BACKEND_FIELD_STAGING_BUFFER_CACHE_ENABLED == 1
	if (image_size > m_p_upload_manager->Get_UploadBufferSize())
	{
		Rml::Log::Message(Rml::Log::Type::LT_WARNING,
			"! [Vulkan]: you are trying to upload huge texture = %zu bytes (%zu Mb), so if you can't optimize its size for loading then ignore this "
			"message, otherwise we "
			"expect you to upload using staging buffer from UploadResourceManager instance",
			image_size, image_size / (1024 * 1024));

		temp_staging_buffer = m_p_upload_manager->Create_StagingBuffer(m_p_allocator, image_size);
		p_staging_buffer = &temp_staging_buffer;
		is_temp_staging = true;
	}
#else
	temp_staging_buffer = m_p_upload_manager->Create_StagingBuffer(m_p_allocator, image_size);
	p_staging_buffer = &temp_staging_buffer;
	is_temp_staging = true;
#endif

	RMLUI_ASSERT(p_staging_buffer && "expected to be true otherwise fatal error");

	void* p_mapped = nullptr;
	VkResult status = vmaMapMemory(m_p_allocator, p_staging_buffer->m_p_vma_allocation, &p_mapped);
	RMLUI_VK_ASSERTMSG(status == VkResult::VK_SUCCESS, "failed to vmaMapMemory");
	std::memcpy(p_mapped, p_data, image_size);
	vmaUnmapMemory(m_p_allocator, p_staging_buffer->m_p_vma_allocation);

	VkImage p_image = p_texture->Get_Image();

	m_p_upload_manager->UploadToGPU([p_image, dimensions, p_staging_buffer](VkCommandBuffer p_cmd) {
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
		region.imageExtent = {static_cast<uint32_t>(dimensions.x), static_cast<uint32_t>(dimensions.y), 1};

		vkCmdCopyBufferToImage(p_cmd, p_staging_buffer->m_p_vk_buffer, p_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

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

	if (is_temp_staging)
	{
		m_p_upload_manager->Destroy_StagingBuffer(m_p_allocator, temp_staging_buffer);
	}

	p_texture->Set_Layout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

RenderInterface_VK::RenderLayerStack::RenderLayerStack() :
	m_msaa_sample_count{1}, m_layers_size{}, m_width{}, m_height{}, m_p_owner{}, m_p_shared_depth_stencil_for_layers{}
{
	RMLUI_ZoneScopedN("Vulkan - RenderLayerStack::Constructor");

	m_fb_postprocess.resize(4);

	// in order to prevent calling dtor when doing push_back on m_fb_layers
	// we need to reserve memory, like how much we do expect elements in array (vector)
	// otherwise you will get validation assert in dtor of FramebufferData struct and
	// that validation supposed to be for memory leaks or wrong resource handling (like you forgot to delete resource somehow)
	// if you didn't get it check this: https://en.cppreference.com/w/cpp/container/vector/reserve

	// otherwise if your default implementation requires more layers by default, thus we have a field at compile-time (or at runtime as dynamic
	// extension) RMLUI_RENDER_BACKEND_OVERRIDE_FIELD_RESERVECOUNT_OF_RENDERSTACK_LAYERS
	m_fb_layers.reserve(RMLUI_RENDER_BACKEND_FIELD_RESERVECOUNT_OF_RENDERSTACK_LAYERS);

	m_p_shared_depth_stencil_for_layers = new Gfx::FramebufferData();
	m_p_shared_depth_stencil_for_layers->Set_RenderTarget(false);
}

RenderInterface_VK::RenderLayerStack::~RenderLayerStack()
{
	RMLUI_ZoneScopedN("Vulkan - RenderLayerStack::Destructor");

	m_p_owner = nullptr;

	if (m_p_shared_depth_stencil_for_layers)
	{
		delete m_p_shared_depth_stencil_for_layers;
		m_p_shared_depth_stencil_for_layers = nullptr;
	}
}

void RenderInterface_VK::RenderLayerStack::Initialize(RenderInterface_VK* p_owner)
{
	RMLUI_ZoneScopedN("Vulkan - RenderLayerStack::Initialize");
	RMLUI_ASSERTMSG(p_owner, "you must pass a valid pointer of RenderInterface_VK instance");

	if (p_owner)
	{
		RMLUI_ASSERTMSG(p_owner->m_manager_texture.Is_Initialized(),
			"early call! you must initialize texture memory manager before calling this method!");
		RMLUI_ASSERTMSG(p_owner->m_manager_buffer.Is_Initialized(),
			"early call! you must initialize buffer memory manager before calling this method!");
		RMLUI_ASSERTMSG(p_owner->m_p_device, "you must initialize Vulkan before calling this method! device is nullptr");

		m_p_owner = p_owner;
		m_msaa_sample_count = p_owner->m_msaa_sample_count;
	}

#ifdef RMLUI_DEBUG
	Rml::Log::Message(Rml::Log::Type::LT_DEBUG, "[RenderLayerStack]: msaa sample count = %d", int(m_msaa_sample_count));
#endif
}

void RenderInterface_VK::RenderLayerStack::Shutdown()
{
	RMLUI_ZoneScopedN("Vulkan - RenderLayerStack::Shutdown");

	DestroyFramebuffers();
}

Rml::LayerHandle RenderInterface_VK::RenderLayerStack::PushLayer()
{
	RMLUI_ZoneScopedN("Vulkan - RenderLayerStack::PushLayer");

	RMLUI_ASSERTMSG(m_layers_size <= static_cast<int>(m_fb_layers.size()), "overflow of layers!");
	RMLUI_ASSERTMSG(m_p_shared_depth_stencil_for_layers, "must be valid!");

	if (m_layers_size == static_cast<int>(m_fb_layers.size()))
	{
		if (m_p_shared_depth_stencil_for_layers->Get_Texture() == nullptr)
		{
			CreateFramebuffer(m_p_shared_depth_stencil_for_layers, m_width, m_height, m_msaa_sample_count, true);
		}

		m_fb_layers.push_back(Gfx::FramebufferData{});
		auto* p_buffer = &m_fb_layers.back();

		// the shared depth-stencil must be set BEFORE CreateFramebuffer: its presence decides that a layer framebuffer
		// (with the depth-stencil attachment, on the layer render pass) is created
		p_buffer->Set_SharedDepthStencilTexture(m_p_shared_depth_stencil_for_layers);

		CreateFramebuffer(p_buffer, m_width, m_height, m_msaa_sample_count, false);
		p_buffer->Set_ID(static_cast<int>(m_fb_layers.size() - 1));
	}

	++m_layers_size;

	return GetTopLayerHandle();
}

void RenderInterface_VK::RenderLayerStack::PopLayer()
{
	RMLUI_ZoneScopedN("Vulkan - RenderLayerStack::PopLayer");

	RMLUI_ASSERTMSG(m_layers_size > 0, "calculations are wrong, debug your code please!");
	m_layers_size -= 1;
}

const Gfx::FramebufferData& RenderInterface_VK::RenderLayerStack::GetLayer(Rml::LayerHandle layer) const
{
	RMLUI_ZoneScopedN("Vulkan - RenderLayerStack::GetLayer");

	RMLUI_ASSERT(static_cast<size_t>(layer) < static_cast<size_t>(m_layers_size) &&
		"overflow or not correct calculation or something is broken, debug the code!");
	return m_fb_layers.at(static_cast<size_t>(layer));
}

const Gfx::FramebufferData& RenderInterface_VK::RenderLayerStack::GetTopLayer() const
{
	RMLUI_ZoneScopedN("Vulkan - RenderLayerStack::GetTopLayer");

	RMLUI_ASSERTMSG(m_layers_size > 0, "early calling!");
	return m_fb_layers[m_layers_size - 1];
}

const Gfx::FramebufferData& RenderInterface_VK::RenderLayerStack::Get_SharedDepthStencil_Layers()
{
	RMLUI_ZoneScopedN("Vulkan - RenderLayerStack::Get_SharedDepthStencil_Layers");

	RMLUI_ASSERTMSG(m_p_shared_depth_stencil_for_layers, "early calling!");
	return *m_p_shared_depth_stencil_for_layers;
}

Rml::LayerHandle RenderInterface_VK::RenderLayerStack::GetTopLayerHandle() const
{
	RMLUI_ZoneScopedN("Vulkan - RenderLayerStack::GetTopLayerHandle");

	RMLUI_ASSERTMSG(m_layers_size > 0, "early calling or something is broken!");
	return static_cast<Rml::LayerHandle>(m_layers_size - 1);
}

void RenderInterface_VK::RenderLayerStack::SwapPostprocessPrimarySecondary()
{
	RMLUI_ZoneScopedN("Vulkan - RenderLayerStack::SwapPostprocessPrimarySecondary");

	std::swap(m_fb_postprocess[0], m_fb_postprocess[1]);
}

void RenderInterface_VK::RenderLayerStack::BeginFrame(int new_width, int new_height)
{
	RMLUI_ZoneScopedN("Vulkan - RenderLayerStack::BeginFrame");

	RMLUI_ASSERTMSG(m_layers_size == 0, "something is wrong and you forgot to clear/delete something!");

	if (m_width != new_width || m_height != new_height)
	{
		m_width = new_width;
		m_height = new_height;

		// safe point: this is only reached after a resize, and SetViewport drains the device before changing the size
		DestroyFramebuffers();
	}

	PushLayer();
}

void RenderInterface_VK::RenderLayerStack::EndFrame()
{
	RMLUI_ZoneScopedN("Vulkan - RenderLayerStack::EndFrame");

	RMLUI_ASSERTMSG(m_layers_size == 1, "order is wrong or something is broken!");
	PopLayer();
}

void RenderInterface_VK::RenderLayerStack::DestroyFramebuffers()
{
	RMLUI_ZoneScopedN("Vulkan - RenderLayerStack::DestroyFramebuffers");

	RMLUI_ASSERTMSG(m_layers_size == 0, "Do not call this during frame rendering, that is, between BeginFrame() and EndFrame().");
	RMLUI_ASSERTMSG(m_p_owner, "you must initialize this manager or it is a early calling or it is a late calling, debug please!");

	// deleting shared depth stencil
	if (m_p_shared_depth_stencil_for_layers && m_p_shared_depth_stencil_for_layers->Get_Texture())
	{
		DestroyFramebuffer(m_p_shared_depth_stencil_for_layers);
	}

	for (auto& fb : m_fb_layers)
	{
		if (fb.Get_Texture())
		{
			DestroyFramebuffer(&fb);
		}
	}

	m_fb_layers.clear();

	for (auto& fb : m_fb_postprocess)
	{
		if (fb.Get_Texture())
		{
			DestroyFramebuffer(&fb);
		}
	}
}

const Gfx::FramebufferData& RenderInterface_VK::RenderLayerStack::EnsureFramebufferPostprocess(int index)
{
	RMLUI_ZoneScopedN("Vulkan - RenderLayerStack::EnsureFramebufferPostprocess");

	RMLUI_ASSERTMSG(index < static_cast<int>(m_fb_postprocess.size()), "overflow or wrong calculation, debug the code!");

	Gfx::FramebufferData& fb = m_fb_postprocess.at(index);

	if (!fb.Get_Texture())
	{
		CreateFramebuffer(&fb, m_width, m_height, 1, false);
		fb.Set_ID(index);

#ifdef RMLUI_VK_DEBUG
		fb.m_is_allocated_on_stack = false;
#endif
	}

	return fb;
}

void RenderInterface_VK::RenderLayerStack::CreateFramebuffer(Gfx::FramebufferData* p_result, int width, int height, int sample_count,
	bool is_depth_stencil)
{
	RMLUI_ZoneScopedN("Vulkan - RenderLayerStack::CreateFramebuffer");

	RMLUI_ASSERTMSG(p_result, "you must pass a valid pointer!");
	RMLUI_ASSERTMSG(sample_count > 0, "you must pass a valid value! it must be positive");
	RMLUI_ASSERTMSG(width > 0, "pass a valid width!");
	RMLUI_ASSERTMSG(height > 0, "pass a valid height");
	RMLUI_ASSERTMSG(m_p_owner, "you must register the owner before calling this method");

	auto* p_resource = new RenderInterface_VK::TextureHandleType();

	RMLUI_ASSERTMSG(p_resource, "[OS][ERROR] not enough memory for allocation!");

	p_result->Set_Width(width);
	p_result->Set_Height(height);
	p_result->Set_Texture(p_resource);

	const VkFormat format = is_depth_stencil ? m_p_owner->Get_SupportedDepthFormat() : RMLUI_RENDER_BACKEND_FIELD_COLOR_TEXTURE_FORMAT;

	m_p_owner->m_manager_texture.Alloc_Texture(p_resource, Rml::Vector2i(width, height), format, static_cast<VkSampleCountFlagBits>(sample_count),
		is_depth_stencil
#ifdef RMLUI_VK_DEBUG
		,
		is_depth_stencil ? "Render2Texture|Depth-Stencil" : "Render2Texture|Render-Target"
#endif
	);

	if (!is_depth_stencil)
	{
		Gfx::FramebufferData* p_depth_stencil = p_result->Get_SharedDepthStencilTexture();

		Rml::Array<VkImageView, 2> attachments = {};
		attachments[0] = p_resource->Get_ImageView();

		VkRenderPass p_render_pass = nullptr;
		uint32_t attachment_count = 1;

		if (p_depth_stencil)
		{
			// layer framebuffer: color + shared depth-stencil on the layer render pass
			RMLUI_ASSERTMSG(p_depth_stencil->Get_Texture(), "the shared depth-stencil texture must be created first!");
			attachments[1] = p_depth_stencil->Get_Texture()->Get_ImageView();
			attachment_count = 2;
			p_render_pass = m_p_owner->m_p_render_pass_layer;
		}
		else
		{
			// postprocess framebuffer: single-sample color only
			p_render_pass = m_p_owner->m_p_render_pass_postprocess;
		}

		VkFramebufferCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		info.pNext = nullptr;
		info.renderPass = p_render_pass;
		info.attachmentCount = attachment_count;
		info.pAttachments = attachments.data();
		info.width = static_cast<uint32_t>(width);
		info.height = static_cast<uint32_t>(height);
		info.layers = 1;

		VkFramebuffer p_framebuffer = nullptr;
		VkResult status = vkCreateFramebuffer(m_p_owner->m_p_device, &info, nullptr, &p_framebuffer);
		RMLUI_VK_ASSERTMSG(status == VkResult::VK_SUCCESS, "failed to vkCreateFramebuffer");

		p_result->Set_Framebuffer(p_framebuffer);
		// stored so that BindRenderTarget can begin the matching render pass without re-deriving the framebuffer kind
		p_result->Set_RenderPass(p_render_pass);
	}
}

void RenderInterface_VK::RenderLayerStack::DestroyFramebuffer(Gfx::FramebufferData* p_data)
{
	RMLUI_ZoneScopedN("Vulkan - RenderLayerStack::DestroyFramebuffer");

	RMLUI_ASSERTMSG(p_data, "you must pass a valid data");
	RMLUI_ASSERTMSG(m_p_owner, "early/late calling?");

	if (p_data)
	{
		if (p_data->Get_Framebuffer())
		{
			vkDestroyFramebuffer(m_p_owner->m_p_device, p_data->Get_Framebuffer(), nullptr);
			p_data->Set_Framebuffer(nullptr);
			p_data->Set_RenderPass(nullptr);
		}

		if (auto* p_texture = p_data->Get_Texture())
		{
			m_p_owner->m_manager_texture.Free_Texture(p_texture);
			delete p_texture;
			p_data->Set_Texture(nullptr);
		}

		p_data->Set_Width(-1);
		p_data->Set_Height(-1);
	}
}
