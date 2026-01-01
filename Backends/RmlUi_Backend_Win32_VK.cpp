#include "RmlUi_Backend.h"
#include "RmlUi_Include_Windows.h"
#include "RmlUi_Platform_Win32.h"
#include "RmlUi_Renderer_VK.h"
#include <RmlUi/Config/Config.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Input.h>
#include <RmlUi/Core/Log.h>
#include <RmlUi/Core/Profiling.h>

/**
    High DPI support using Windows Per Monitor V2 DPI awareness.

    Requires Windows 10, version 1703.
 */
#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
	#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((HANDLE)-4)
#endif
#ifndef WM_DPICHANGED
	#define WM_DPICHANGED 0x02E0
#endif

// Declare pointers to the DPI aware Windows API functions.
using ProcSetProcessDpiAwarenessContext = BOOL(WINAPI*)(HANDLE value);
using ProcGetDpiForWindow = UINT(WINAPI*)(HWND hwnd);
using ProcAdjustWindowRectExForDpi = BOOL(WINAPI*)(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle, UINT dpi);

static bool has_dpi_support = false;
static ProcSetProcessDpiAwarenessContext procSetProcessDpiAwarenessContext = NULL;
static ProcGetDpiForWindow procGetDpiForWindow = NULL;
static ProcAdjustWindowRectExForDpi procAdjustWindowRectExForDpi = NULL;

// Make ourselves DPI aware on supported Windows versions.
static void InitializeDpiSupport()
{
	// Cast function pointers to void* first for MinGW not to emit errors.
	procSetProcessDpiAwarenessContext =
		(ProcSetProcessDpiAwarenessContext)(void*)GetProcAddress(GetModuleHandle(TEXT("User32.dll")), "SetProcessDpiAwarenessContext");
	procGetDpiForWindow = (ProcGetDpiForWindow)(void*)GetProcAddress(GetModuleHandle(TEXT("User32.dll")), "GetDpiForWindow");
	procAdjustWindowRectExForDpi =
		(ProcAdjustWindowRectExForDpi)(void*)GetProcAddress(GetModuleHandle(TEXT("User32.dll")), "AdjustWindowRectExForDpi");

	if (!has_dpi_support && procSetProcessDpiAwarenessContext != NULL && procGetDpiForWindow != NULL && procAdjustWindowRectExForDpi != NULL)
	{
		// Activate Per Monitor V2.
		if (procSetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2))
			has_dpi_support = true;
	}
}

static UINT GetWindowDpi(HWND window_handle)
{
	if (has_dpi_support)
	{
		UINT dpi = procGetDpiForWindow(window_handle);
		if (dpi != 0)
			return dpi;
	}
	return USER_DEFAULT_SCREEN_DPI;
}

static float GetDensityIndependentPixelRatio(HWND window_handle)
{
	return float(GetWindowDpi(window_handle)) / float(USER_DEFAULT_SCREEN_DPI);
}

// Create the window but don't show it yet. Returns the pixel size of the window, which may be different than the passed size due to DPI settings.
static HWND InitializeWindow(HINSTANCE instance_handle, const std::wstring& name, int& inout_width, int& inout_height, bool allow_resize);
// Create the Win32 Vulkan surface.
static bool CreateVulkanSurface(VkInstance instance, VkSurfaceKHR* out_surface);

/**
    Global data used by this backend.

    Lifetime governed by the calls to Backend::Initialize() and Backend::Shutdown().
 */
struct BackendData {
	SystemInterface_Win32 system_interface;
	RenderInterface_VK render_interface;
	TextInputMethodEditor_Win32 text_input_method_editor;

	HINSTANCE instance_handle = nullptr;
	std::wstring instance_name;
	HWND window_handle = nullptr;

	bool context_dimensions_dirty = true;
	Rml::Vector2i window_dimensions;
	bool running = true;

	// Arguments set during event processing and nulled otherwise.
	Rml::Context* context = nullptr;
	KeyDownCallback key_down_callback = nullptr;
};
static Rml::UniquePtr<BackendData> data;

bool Backend::Initialize(const char* window_name, int width, int height, bool allow_resize)
{
	RMLUI_ASSERT(!data);

	const std::wstring name = RmlWin32::ConvertToUTF16(Rml::String(window_name));

	data = Rml::MakeUnique<BackendData>();

	data->instance_handle = GetModuleHandle(nullptr);
	data->instance_name = name;

	InitializeDpiSupport();

	// Initialize the window but don't show it yet.
	HWND window_handle = InitializeWindow(data->instance_handle, name, width, height, allow_resize);
	if (!window_handle)
		return false;

	data->window_handle = window_handle;

	Rml::Vector<const char*> extensions;
	extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
	extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
	if (!data->render_interface.Initialize(std::move(extensions), CreateVulkanSurface))
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "Failed to initialize Vulkan render interface");
		::CloseWindow(window_handle);
		data.reset();
		return false;
	}

	data->system_interface.SetWindow(window_handle);
	data->render_interface.SetViewport(width, height);

	// Now we are ready to show the window.
	::ShowWindow(window_handle, SW_SHOW);
	::SetForegroundWindow(window_handle);
	::SetFocus(window_handle);

	// Provide a backend-specific text input handler to manage the IME.
	Rml::SetTextInputHandler(&data->text_input_method_editor);

	return true;
}

void Backend::Shutdown()
{
	RMLUI_ASSERT(data);

	// As we forcefully override the global text input handler, we must reset it before the data is destroyed to avoid any potential use-after-free.
	if (Rml::GetTextInputHandler() == &data->text_input_method_editor)
		Rml::SetTextInputHandler(nullptr);

	data->render_interface.Shutdown();

	::DestroyWindow(data->window_handle);
	::UnregisterClassW((LPCWSTR)data->instance_name.data(), data->instance_handle);

	data.reset();
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

static bool NextEvent(MSG& message, UINT timeout)
{
	if (timeout != 0)
	{
		UINT_PTR timer_id = SetTimer(NULL, 0, timeout, NULL);
		BOOL res = GetMessage(&message, NULL, 0, 0);
		KillTimer(NULL, timer_id);
		if (message.message != WM_TIMER || message.hwnd != nullptr || message.wParam != timer_id)
			return res;
	}
	return PeekMessage(&message, nullptr, 0, 0, PM_REMOVE);
}

bool Backend::ProcessEvents(Rml::Context* context, KeyDownCallback key_down_callback, bool power_save)
{
	RMLUI_ASSERT(data && context);

	// The initial window size may have been affected by system DPI settings, apply the actual pixel size and dp-ratio to the context.
	if (data->context_dimensions_dirty)
	{
		data->context_dimensions_dirty = false;
		const float dp_ratio = GetDensityIndependentPixelRatio(data->window_handle);
		context->SetDimensions(data->window_dimensions);
		context->SetDensityIndependentPixelRatio(dp_ratio);
	}

	data->context = context;
	data->key_down_callback = key_down_callback;

	MSG message;
	// Process events.
	bool has_message = NextEvent(message, power_save ? static_cast<int>(Rml::Math::Min(context->GetNextUpdateDelay(), 10.0) * 1000.0) : 0);
	while (has_message || !data->render_interface.IsSwapchainValid())
	{
		if (has_message)
		{
			// Dispatch the message to our local event handler below.
			TranslateMessage(&message);
			DispatchMessage(&message);
		}

		// In some situations the swapchain may become invalid, such as when the window is minimized. In this state the renderer cannot accept any
		// render calls. Since we don't have full control over the main loop here we may risk calls to Context::Render if we were to return. Instead,
		// we trap the application inside this loop until we are able to recreate the swapchain and render again.
		if (!data->render_interface.IsSwapchainValid())
			data->render_interface.RecreateSwapchain();

		has_message = NextEvent(message, 0);
	}

	data->context = nullptr;
	data->key_down_callback = nullptr;

	return data->running;
}

void Backend::RequestExit()
{
	RMLUI_ASSERT(data);
	data->running = false;
}

void Backend::BeginFrame()
{
	RMLUI_ASSERT(data);
	data->render_interface.BeginFrame();
}

void Backend::PresentFrame()
{
	RMLUI_ASSERT(data);
	data->render_interface.EndFrame();

	// Optional, used to mark frames during performance profiling.
	RMLUI_FrameMark;
}

// Local event handler for window and input events.
static LRESULT CALLBACK WindowProcedureHandler(HWND window_handle, UINT message, WPARAM w_param, LPARAM l_param)
{
	RMLUI_ASSERT(data);

	switch (message)
	{
	case WM_CLOSE:
	{
		data->running = false;
		return 0;
	}
	break;
	case WM_SIZE:
	{
		const int width = LOWORD(l_param);
		const int height = HIWORD(l_param);
		data->window_dimensions.x = width;
		data->window_dimensions.y = height;
		if (data->context)
		{
			data->render_interface.SetViewport(width, height);
			data->context->SetDimensions(data->window_dimensions);
		}
		return 0;
	}
	break;
	case WM_DPICHANGED:
	{
		RECT* new_pos = (RECT*)l_param;
		SetWindowPos(window_handle, NULL, new_pos->left, new_pos->top, new_pos->right - new_pos->left, new_pos->bottom - new_pos->top,
			SWP_NOZORDER | SWP_NOACTIVATE);
		if (data->context && has_dpi_support)
			data->context->SetDensityIndependentPixelRatio(GetDensityIndependentPixelRatio(window_handle));
		return 0;
	}
	break;
	case WM_KEYDOWN:
	{
		// Override the default key event callback to add global shortcuts for the samples.
		Rml::Context* context = data->context;
		KeyDownCallback key_down_callback = data->key_down_callback;

		const Rml::Input::KeyIdentifier rml_key = RmlWin32::ConvertKey((int)w_param);
		const int rml_modifier = RmlWin32::GetKeyModifierState();
		const float native_dp_ratio = GetDensityIndependentPixelRatio(window_handle);

		// See if we have any global shortcuts that take priority over the context.
		if (key_down_callback && !key_down_callback(context, rml_key, rml_modifier, native_dp_ratio, true))
			return 0;
		// Otherwise, hand the event over to the context by calling the input handler as normal.
		if (!RmlWin32::WindowProcedure(context, data->text_input_method_editor, window_handle, message, w_param, l_param))
			return 0;
		// The key was not consumed by the context either, try keyboard shortcuts of lower priority.
		if (key_down_callback && !key_down_callback(context, rml_key, rml_modifier, native_dp_ratio, false))
			return 0;
		return 0;
	}
	break;
	default:
	{
		// Submit it to the platform handler for default input handling.
		if (!RmlWin32::WindowProcedure(data->context, data->text_input_method_editor, window_handle, message, w_param, l_param))
			return 0;
	}
	break;
	}

	// All unhandled messages go to DefWindowProc.
	return DefWindowProc(window_handle, message, w_param, l_param);
}

static HWND InitializeWindow(HINSTANCE instance_handle, const std::wstring& name, int& inout_width, int& inout_height, bool allow_resize)
{
	// Fill out the window class struct.
	WNDCLASSW window_class;
	window_class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	window_class.lpfnWndProc = &WindowProcedureHandler; // Attach our local event handler.
	window_class.cbClsExtra = 0;
	window_class.cbWndExtra = 0;
	window_class.hInstance = instance_handle;
	window_class.hIcon = LoadIcon(nullptr, IDI_WINLOGO);
	window_class.hCursor = LoadCursor(nullptr, IDC_ARROW);
	window_class.hbrBackground = nullptr;
	window_class.lpszMenuName = nullptr;
	window_class.lpszClassName = name.data();

	if (!RegisterClassW(&window_class))
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "Failed to register window class");
		return nullptr;
	}

	HWND window_handle = CreateWindowExW(WS_EX_APPWINDOW | WS_EX_WINDOWEDGE,
		name.data(),                                                                // Window class name.
		name.data(), WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_OVERLAPPEDWINDOW, 0, 0, // Window position.
		0, 0,                                                                       // Window size.
		nullptr, nullptr, instance_handle, nullptr);

	if (!window_handle)
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "Failed to create window");
		return nullptr;
	}

	UINT window_dpi = GetWindowDpi(window_handle);
	inout_width = (inout_width * (int)window_dpi) / USER_DEFAULT_SCREEN_DPI;
	inout_height = (inout_height * (int)window_dpi) / USER_DEFAULT_SCREEN_DPI;

	DWORD style = (allow_resize ? WS_OVERLAPPEDWINDOW : (WS_OVERLAPPEDWINDOW & ~WS_SIZEBOX & ~WS_MAXIMIZEBOX));
	DWORD extended_style = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;

	// Adjust the window size to take the edges into account.
	RECT window_rect;
	window_rect.top = 0;
	window_rect.left = 0;
	window_rect.right = inout_width;
	window_rect.bottom = inout_height;
	if (has_dpi_support)
		procAdjustWindowRectExForDpi(&window_rect, style, FALSE, extended_style, window_dpi);
	else
		AdjustWindowRectEx(&window_rect, style, FALSE, extended_style);

	SetWindowLong(window_handle, GWL_EXSTYLE, extended_style);
	SetWindowLong(window_handle, GWL_STYLE, style);

	// Resize the window and center it on the screen.
	Rml::Vector2i screen_size = {GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)};
	Rml::Vector2i window_size = {int(window_rect.right - window_rect.left), int(window_rect.bottom - window_rect.top)};
	Rml::Vector2i window_pos = Rml::Math::Max((screen_size - window_size) / 2, Rml::Vector2i(0));

	SetWindowPos(window_handle, HWND_TOP, window_pos.x, window_pos.y, window_size.x, window_size.y, SWP_NOACTIVATE);

	return window_handle;
}

bool CreateVulkanSurface(VkInstance instance, VkSurfaceKHR* out_surface)
{
	VkWin32SurfaceCreateInfoKHR info = {};
	info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	info.hinstance = GetModuleHandle(NULL);
	info.hwnd = data->window_handle;

	VkResult status = vkCreateWin32SurfaceKHR(instance, &info, nullptr, out_surface);

	bool result = (status == VK_SUCCESS);
	RMLUI_VK_ASSERTMSG(result, "Failed to create Win32 Vulkan surface");
	return result;
}
