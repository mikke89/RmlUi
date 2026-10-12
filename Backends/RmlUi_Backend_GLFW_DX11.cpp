#include "RmlUi_Backend.h"
#include "RmlUi_Platform_GLFW.h"
#include "RmlUi_Renderer_DX11.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Input.h>
#include <RmlUi/Core/Math.h>
#include <RmlUi/Core/Profiling.h>
#include <optional>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#ifdef RMLUI_USE_STB_IMAGE_LOADER
	#define STB_IMAGE_IMPLEMENTATION
	#include "stb_image.h"
	#include <RmlUi/Core.h>
	#include <RmlUi/Core/FileInterface.h>
#endif

/**
    Custom render interface example for the DX11/GLFW backend.

    Overloads the DX11 render interface to load textures through stb_image.
 */
class RenderInterface_DX11_Win32 : public RenderInterface_DX11 {
public:
	RenderInterface_DX11_Win32(ID3D11Device* p_d3d_device) : RenderInterface_DX11(p_d3d_device) {}

#ifdef RMLUI_USE_STB_IMAGE_LOADER
	Rml::TextureHandle LoadTexture(Rml::Vector2i& texture_dimensions, const Rml::String& source) override
	{
		int texture_width = 0, texture_height = 0, num_channels = 0;
		size_t image_size_bytes = 0;
		uint8_t* texture_data = nullptr;

		// Find where on disk the file is
		Rml::FileInterface* file_interface = Rml::GetFileInterface();
		Rml::FileHandle file_handle = file_interface->Open(source);
		if (!file_handle)
		{
			// Tell RmlUI that the image is invalid
			texture_data = nullptr;
			return false;
		}

		// Load the file through stb_image
		texture_data = stbi_load_from_file((FILE*)file_handle, &texture_width, &texture_height, &num_channels, 0);

		// If the file data is correct
		if (texture_data != nullptr)
		{
			// Compute number of elements in texture
			image_size_bytes = texture_width * texture_height * num_channels;

			// Pre-multiply the data
			for (int i = 0; i < image_size_bytes; i += 4)
			{
				texture_data[i + 0] = (uint8_t)((texture_data[i + 0] * texture_data[i + 3]) / 255);
				texture_data[i + 1] = (uint8_t)((texture_data[i + 1] * texture_data[i + 3]) / 255);
				texture_data[i + 2] = (uint8_t)((texture_data[i + 2] * texture_data[i + 3]) / 255);
			}

			texture_dimensions.x = texture_width;
			texture_dimensions.y = texture_height;

			Rml::TextureHandle handle = GenerateTexture({texture_data, image_size_bytes}, texture_dimensions);

			stbi_image_free(texture_data);
			return handle;
		}
		return false;
	}
#endif
};

static void SetupCallbacks(GLFWwindow* window);

static void LogErrorFromGLFW(int error, const char* description)
{
	Rml::Log::Message(Rml::Log::LT_ERROR, "GLFW error (0x%x): %s", error, description);
}

struct DeviceResources {
	ID3D11Device* pd3dDevice = nullptr;
	ID3D11DeviceContext* pd3dDeviceContext = nullptr;
	IDXGISwapChain* pSwapChain = nullptr;
	bool swapchain_occluded = false;
	ID3D11RenderTargetView* pMainRenderTargetView = nullptr;
};

// D3D Creation / Cleanup functions
static bool CreateDeviceD3D(HWND hwnd, DeviceResources& device_resources);
static void CleanupDeviceD3D(DeviceResources& device_resources);
static void CreateRenderTarget(DeviceResources& device_resources);
static void CleanupRenderTarget(DeviceResources& device_resources);

/**
    Global data used by this backend.

    Lifetime governed by the calls to Backend::Initialize() and Backend::Shutdown().
 */
struct BackendData {
	BackendData(GLFWwindow* window) : system_interface(window), window(window) {}

	SystemInterface_GLFW system_interface;
	std::optional<RenderInterface_DX11_Win32> render_interface;
	GLFWwindow* window = nullptr;
	int glfw_active_modifiers = 0;
	bool context_dimensions_dirty = true;

	DeviceResources device_resources;

	// Arguments set during event processing and nulled otherwise.
	Rml::Context* context = nullptr;
	KeyDownCallback key_down_callback = nullptr;
};
static Rml::UniquePtr<BackendData> data;

bool Backend::Initialize(const char* name, int width, int height, bool allow_resize)
{
	RMLUI_ASSERT(!data);

	glfwSetErrorCallback(LogErrorFromGLFW);

	if (!glfwInit())
		return false;

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, allow_resize ? GLFW_TRUE : GLFW_FALSE);
	glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);

	GLFWwindow* window = glfwCreateWindow(width, height, name, nullptr, nullptr);
	if (!window)
		return false;

	// Construct the system and render interface, this includes compiling all the shaders. If this fails, it is likely an error in the shader code.
	data = Rml::MakeUnique<BackendData>(window);
	if (!data)
		return false;

	HWND windowHwnd = glfwGetWin32Window(window);
	if (!CreateDeviceD3D(windowHwnd, data->device_resources))
	{
		Shutdown();
		return false;
	}

#ifdef RMLUI_USE_STB_IMAGE_LOADER
	// iPhone PNGs are pre-multiplied. Un-premultiply them because we handle pre-multiplication manually on decode.
	stbi_set_unpremultiply_on_load(true);
	// iPhone PNGs are encoded as BGRA, this tells stb_image to unpack as RGBA.
	stbi_convert_iphone_png_to_rgb(true);
#endif

	data->render_interface.emplace(data->device_resources.pd3dDevice);

	// The window size may have been scaled by DPI settings, get the actual pixel size.
	glfwGetFramebufferSize(window, &width, &height);
	width = Rml::Math::Max(width, 1);
	height = Rml::Math::Max(height, 1);
	data->render_interface->SetViewport(width, height);

	// Receive num lock and caps lock modifiers for proper handling of numpad inputs in text fields.
	glfwSetInputMode(window, GLFW_LOCK_KEY_MODS, GLFW_TRUE);

	// Setup the input and window event callback functions.
	SetupCallbacks(window);

	return true;
}

void Backend::Shutdown()
{
	RMLUI_ASSERT(data);

	// Cleanup renderer resources
	data->render_interface.reset();

	// Shutdown DirectX11
	CleanupDeviceD3D(data->device_resources);

	glfwDestroyWindow(data->window);
	data.reset();
	glfwTerminate();
}

Rml::SystemInterface* Backend::GetSystemInterface()
{
	RMLUI_ASSERT(data);
	return &data->system_interface;
}

Rml::RenderInterface* Backend::GetRenderInterface()
{
	RMLUI_ASSERT(data);
	return &(*data->render_interface);
}

bool Backend::ProcessEvents(Rml::Context* context, KeyDownCallback key_down_callback, bool power_save)
{
	RMLUI_ASSERT(data && context);

	// The initial window size may have been affected by system DPI settings, apply the actual pixel size and dp-ratio to the context.
	if (data->context_dimensions_dirty)
	{
		data->context_dimensions_dirty = false;

		Rml::Vector2i window_size;
		float dp_ratio = 1.f;
		glfwGetFramebufferSize(data->window, &window_size.x, &window_size.y);
		glfwGetWindowContentScale(data->window, &dp_ratio, nullptr);

		context->SetDimensions(window_size);
		context->SetDensityIndependentPixelRatio(dp_ratio);

		CleanupRenderTarget(data->device_resources);
		data->device_resources.pSwapChain->ResizeBuffers(0, window_size.x, window_size.y, DXGI_FORMAT_UNKNOWN, 0);
		CreateRenderTarget(data->device_resources);
	}

	data->context = context;
	data->key_down_callback = key_down_callback;

	if (power_save)
		glfwWaitEventsTimeout(Rml::Math::Min(context->GetNextUpdateDelay(), 10.0));
	else
		glfwPollEvents();

	data->context = nullptr;
	data->key_down_callback = nullptr;

	const bool result = !glfwWindowShouldClose(data->window);
	glfwSetWindowShouldClose(data->window, GLFW_FALSE);
	return result;
}

void Backend::RequestExit()
{
	RMLUI_ASSERT(data);
	glfwSetWindowShouldClose(data->window, GLFW_TRUE);
}

void Backend::BeginFrame()
{
	RMLUI_ASSERT(data);
	data->render_interface->Clear(data->device_resources.pMainRenderTargetView);
	data->render_interface->BeginFrame();
}

void Backend::PresentFrame()
{
	RMLUI_ASSERT(data);
	data->render_interface->EndFrame(data->device_resources.pMainRenderTargetView);

	// Present with vsync enabled, set to zero to disable.
	constexpr unsigned int sync_interval = 1;

	HRESULT hr = data->device_resources.pSwapChain->Present(sync_interval, 0);
	data->device_resources.swapchain_occluded = (hr == DXGI_STATUS_OCCLUDED);

	// Optional, used to mark frames during performance profiling.
	RMLUI_FrameMark;
}

static void SetupCallbacks(GLFWwindow* window)
{
	RMLUI_ASSERT(data);

	// Key input
	glfwSetKeyCallback(window, [](GLFWwindow* /*window*/, int glfw_key, int /*scancode*/, int glfw_action, int glfw_mods) {
		if (!data->context)
			return;

		// Store the active modifiers for later because GLFW doesn't provide them in the callbacks to the mouse input events.
		data->glfw_active_modifiers = glfw_mods;

		// Override the default key event callback to add global shortcuts for the samples.
		Rml::Context* context = data->context;
		KeyDownCallback key_down_callback = data->key_down_callback;

		switch (glfw_action)
		{
		case GLFW_PRESS:
		case GLFW_REPEAT:
		{
			const Rml::Input::KeyIdentifier key = RmlGLFW::ConvertKey(glfw_key);
			const int key_modifier = RmlGLFW::ConvertKeyModifiers(glfw_mods);
			float dp_ratio = 1.f;
			glfwGetWindowContentScale(data->window, &dp_ratio, nullptr);

			// See if we have any global shortcuts that take priority over the context.
			if (key_down_callback && !key_down_callback(context, key, key_modifier, dp_ratio, true))
				break;
			// Otherwise, hand the event over to the context by calling the input handler as normal.
			if (!RmlGLFW::ProcessKeyCallback(context, glfw_key, glfw_action, glfw_mods))
				break;
			// The key was not consumed by the context either, try keyboard shortcuts of lower priority.
			if (key_down_callback && !key_down_callback(context, key, key_modifier, dp_ratio, false))
				break;
		}
		break;
		case GLFW_RELEASE: RmlGLFW::ProcessKeyCallback(context, glfw_key, glfw_action, glfw_mods); break;
		}
	});

	glfwSetCharCallback(window, [](GLFWwindow* /*window*/, unsigned int codepoint) { RmlGLFW::ProcessCharCallback(data->context, codepoint); });

	glfwSetCursorEnterCallback(window, [](GLFWwindow* /*window*/, int entered) { RmlGLFW::ProcessCursorEnterCallback(data->context, entered); });

	// Mouse input
	glfwSetCursorPosCallback(window, [](GLFWwindow* window, double xpos, double ypos) {
		RmlGLFW::ProcessCursorPosCallback(data->context, window, xpos, ypos, data->glfw_active_modifiers);
	});

	glfwSetMouseButtonCallback(window, [](GLFWwindow* /*window*/, int button, int action, int mods) {
		data->glfw_active_modifiers = mods;
		RmlGLFW::ProcessMouseButtonCallback(data->context, button, action, mods);
	});

	glfwSetScrollCallback(window, [](GLFWwindow* /*window*/, double /*xoffset*/, double yoffset) {
		RmlGLFW::ProcessScrollCallback(data->context, yoffset, data->glfw_active_modifiers);
	});

	// Window events
	glfwSetFramebufferSizeCallback(window, [](GLFWwindow* /*window*/, int width, int height) {
		width = Rml::Math::Max(width, 1);
		height = Rml::Math::Max(height, 1);
		data->context_dimensions_dirty = true;
		data->render_interface->SetViewport(width, height);
		RmlGLFW::ProcessFramebufferSizeCallback(data->context, width, height);
	});

	glfwSetWindowContentScaleCallback(window,
		[](GLFWwindow* /*window*/, float xscale, float /*yscale*/) { RmlGLFW::ProcessContentScaleCallback(data->context, xscale); });
}

static bool CreateDeviceD3D(HWND hwnd, DeviceResources& device_resources)
{
	// Setup swap chain
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 2;
	sd.BufferDesc.Width = 0;
	sd.BufferDesc.Height = 0;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hwnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	UINT createDeviceFlags = 0;
#ifdef RMLUI_DX_DEBUG
	// Enable debug layer
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
	D3D_FEATURE_LEVEL featureLevel;
	const D3D_FEATURE_LEVEL featureLevelArray[2] = {
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_0,
	};
	HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2,
		D3D11_SDK_VERSION, &sd, &device_resources.pSwapChain, &device_resources.pd3dDevice, &featureLevel, &device_resources.pd3dDeviceContext);
	if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
		res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd,
			&device_resources.pSwapChain, &device_resources.pd3dDevice, &featureLevel, &device_resources.pd3dDeviceContext);
	if (res != S_OK)
		return false;

	CreateRenderTarget(device_resources);
	return true;
}

static void CleanupDeviceD3D(DeviceResources& device_resources)
{
	CleanupRenderTarget(device_resources);
	if (device_resources.pSwapChain)
	{
		device_resources.pSwapChain->Release();
		device_resources.pSwapChain = nullptr;
	}
	if (device_resources.pd3dDeviceContext)
	{
		device_resources.pd3dDeviceContext->Release();
		device_resources.pd3dDeviceContext = nullptr;
	}
	if (device_resources.pd3dDevice)
	{
		device_resources.pd3dDevice->Release();
		device_resources.pd3dDevice = nullptr;
	}
}

static void CreateRenderTarget(DeviceResources& device_resources)
{
	ID3D11Texture2D* pBackBuffer = nullptr;
	device_resources.pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
	device_resources.pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &device_resources.pMainRenderTargetView);
	pBackBuffer->Release();
}

static void CleanupRenderTarget(DeviceResources& device_resources)
{
	if (device_resources.pMainRenderTargetView)
	{
		device_resources.pMainRenderTargetView->Release();
		device_resources.pMainRenderTargetView = nullptr;
	}
}
