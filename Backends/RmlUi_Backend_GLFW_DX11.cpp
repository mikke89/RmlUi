/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019-2023 The RmlUi Team, and contributors
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

#include "RmlUi_Backend.h"
#include "RmlUi_Platform_GLFW.h"
#include "RmlUi_Renderer_DX11.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Input.h>
#include <RmlUi/Core/Profiling.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#ifdef RMLUI_USE_STB_IMAGE_LOADER
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <RmlUi/Core/FileInterface.h>

// stb_image based loaders
void LoadTexture(const Rml::String& filename, int* pWidth, int* pHeight, uint8_t** pData, size_t* pDataSize);
void FreeTexture(uint8_t* pData);
#endif

static void SetupCallbacks(GLFWwindow* window);

// D3D Creation / Cleanup functions
static bool CreateDeviceD3D(HWND hwnd);
static void CleanupDeviceD3D();
static void CreateRenderTarget();
static void CleanupRenderTarget();

static void LogErrorFromGLFW(int error, const char* description)
{
	Rml::Log::Message(Rml::Log::LT_ERROR, "GLFW error (0x%x): %s", error, description);
}

/**
    Global data used by this backend.

    Lifetime governed by the calls to Backend::Initialize() and Backend::Shutdown().
 */
struct BackendData {
	SystemInterface_GLFW system_interface;
	RenderInterface_DX11 render_interface;
	GLFWwindow* window = nullptr;
	int glfw_active_modifiers = 0;
	bool context_dimensions_dirty = true;

	// D3D11 resources
	struct {
		ID3D11Device* pd3dDevice = nullptr;
		ID3D11DeviceContext* pd3dDeviceContext = nullptr;
		IDXGISwapChain* pSwapChain = nullptr;
		bool swapchain_occluded = false;
		ID3D11RenderTargetView* pMainRenderTargetView = nullptr;
	} d3d_resources;

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
	data = Rml::MakeUnique<BackendData>();
	if (!data)
		return false;

	data->window = window;
	data->system_interface.SetWindow(window);

	HWND windowHwnd = glfwGetWin32Window(window); 
	if (!CreateDeviceD3D(windowHwnd))
	{
		Shutdown();
		return false;
	}

#ifdef RMLUI_USE_STB_IMAGE_LOADER
	// Assign stb_image loader to render interface
	data->render_interface.LoadTextureFromFileRaw = &LoadTexture;
	data->render_interface.FreeTextureFromFileRaw = &FreeTexture;
	// Disable pre-multiplication because loading images through stb_image will inconsistently return pre-multiplied images (only iPhone PNGs are
	// pre-multipled) We handle pre-multiplication manually on decode
	stbi_set_unpremultiply_on_load(true);
	// iPhone PNGs are encoded as BGRA, this tells stb_image to unpack as RGBA
	stbi_convert_iphone_png_to_rgb(true);
#endif

	data->render_interface.Init(data->d3d_resources.pd3dDevice);

	// The window size may have been scaled by DPI settings, get the actual pixel size.
	glfwGetFramebufferSize(window, &width, &height);
	data->render_interface.SetViewport(width, height);

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
	data->render_interface.Cleanup();

	// Shutdown DirectX11
	CleanupDeviceD3D();

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
	return &data->render_interface;
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

		CleanupRenderTarget();
		data->d3d_resources.pSwapChain->ResizeBuffers(0, window_size.x, window_size.y, DXGI_FORMAT_UNKNOWN, 0);
		CreateRenderTarget();
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
	data->render_interface.BeginFrame(data->d3d_resources.pMainRenderTargetView);
}

void Backend::PresentFrame()
{
	RMLUI_ASSERT(data);
	data->render_interface.EndFrame();
	
	// Present
	HRESULT hr = data->d3d_resources.pSwapChain->Present(1, 0); // Present with vsync
	// HRESULT hr = g_pSwapChain->Present(0, 0); // Present without vsync
	data->d3d_resources.swapchain_occluded = (hr == DXGI_STATUS_OCCLUDED);

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
		data->render_interface.SetViewport(width, height);
		RmlGLFW::ProcessFramebufferSizeCallback(data->context, width, height);
	});

	glfwSetWindowContentScaleCallback(window,
		[](GLFWwindow* /*window*/, float xscale, float /*yscale*/) { RmlGLFW::ProcessContentScaleCallback(data->context, xscale); });
}

static bool CreateDeviceD3D(HWND hwnd)
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
#ifdef RMLUI_DEBUG
	// Enable debug layer
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
	D3D_FEATURE_LEVEL featureLevel;
	const D3D_FEATURE_LEVEL featureLevelArray[2] = {
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_0,
	};
	HRESULT res =
		D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd,
			&data->d3d_resources.pSwapChain, &data->d3d_resources.pd3dDevice, &featureLevel, &data->d3d_resources.pd3dDeviceContext);
	if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
		res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd,
			&data->d3d_resources.pSwapChain, &data->d3d_resources.pd3dDevice, &featureLevel, &data->d3d_resources.pd3dDeviceContext);
	if (res != S_OK)
		return false;

	CreateRenderTarget();
	return true;
}

static void CleanupDeviceD3D()
{
	CleanupRenderTarget();
	if (data->d3d_resources.pSwapChain)
	{
		data->d3d_resources.pSwapChain->Release();
		data->d3d_resources.pSwapChain = nullptr;
	}
	if (data->d3d_resources.pd3dDeviceContext)
	{
		data->d3d_resources.pd3dDeviceContext->Release();
		data->d3d_resources.pd3dDeviceContext = nullptr;
	}
	if (data->d3d_resources.pd3dDevice)
	{
		data->d3d_resources.pd3dDevice->Release();
		data->d3d_resources.pd3dDevice = nullptr;
	}
}

static void CreateRenderTarget()
{
	ID3D11Texture2D* pBackBuffer = nullptr;
	data->d3d_resources.pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
	data->d3d_resources.pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &data->d3d_resources.pMainRenderTargetView);
	pBackBuffer->Release();
}

static void CleanupRenderTarget()
{
	if (data->d3d_resources.pMainRenderTargetView)
	{
		data->d3d_resources.pMainRenderTargetView->Release();
		data->d3d_resources.pMainRenderTargetView = nullptr;
	}
}

#ifdef RMLUI_USE_STB_IMAGE_LOADER
void LoadTexture(const Rml::String& filename, int* pWidth, int* pHeight, uint8_t** pData, size_t* pDataSize)
{
	// Find where on disk the file is
	Rml::FileInterface* file_interface = Rml::GetFileInterface();
	Rml::FileHandle file_handle = file_interface->Open(filename);
	if (!file_handle)
	{
		// Tell the backend that the data is invalid (*pData = 0), and return
		*pData = nullptr;
		return;
	}

	// Load the file through stb_image
	int texture_width, texture_height, num_channels;
	*pData = stbi_load_from_file((FILE*)file_handle, &texture_width, &texture_height, &num_channels, 0);

	// If the file data is correct
	if (*pData != nullptr)
	{
		// Assign image parameters
		*pWidth = texture_width;
		*pHeight = texture_height;

		// Compute number of elements in texture
		*pDataSize = texture_width * texture_height * num_channels;

		// Pre-multiply the data
		uint8_t* dataView = *pData;
		for (int i = 0; i < *pDataSize; i += 4)
		{
			dataView[i + 0] = (uint8_t)((dataView[i + 0] * dataView[i + 3]) / 255);
			dataView[i + 1] = (uint8_t)((dataView[i + 1] * dataView[i + 3]) / 255);
			dataView[i + 2] = (uint8_t)((dataView[i + 2] * dataView[i + 3]) / 255);
		}
	}
}

void FreeTexture(uint8_t* pData)
{
	// Free the allocated memory once the texture has been uploaded to the GPU
	stbi_image_free(pData);
}
#endif