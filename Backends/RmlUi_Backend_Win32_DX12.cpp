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

#include "RmlUi/Config/Config.h"
#include "RmlUi_Backend.h"
#include "RmlUi_Include_Windows.h"
#include "RmlUi_Platform_Win32.h"
#include "RmlUi_Renderer_DX12.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Input.h>
#include <RmlUi/Core/Profiling.h>

/**
    High DPI support using Windows Per Monitor V2 DPI awareness.

    Requires Windows 10, version 1703.
 */
#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
	#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((HANDLE) - 4)
#endif
#ifndef WM_DPICHANGED
	#define WM_DPICHANGED 0x02E0
#endif

#define FIX_WARNING_UNUSED(code) (void)(code);

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

static void DisplayError(HWND window_handle, const Rml::String& msg)
{
	MessageBoxW(window_handle, RmlWin32::ConvertToUTF16(msg).c_str(), L"Backend Error", MB_OK);
}

// Create the window but don't show it yet. Returns the pixel size of the window, which may be different than the passed size due to DPI settings.
static HWND InitializeWindow(HINSTANCE instance_handle, const std::wstring& name, int& inout_width, int& inout_height, bool allow_resize);
// Create the Win32 Vulkan surface.
// static bool CreateVulkanSurface(VkInstance instance, VkSurfaceKHR* out_surface);

/**
    Global data used by this backend.

    Lifetime governed by the calls to Backend::Initialize() and Backend::Shutdown().
 */
struct BackendData {
	SystemInterface_Win32 system_interface;
	// deferred initialization, because of Render
	RenderInterface_DX12* render_interface{};
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

// we need to deallocate manually
Backend::RmlRenderInitInfo* p_legacy_instance{};

bool Backend::Initialize(const char* window_name, int width, int height, bool allow_resize, RmlRenderInitInfo* p_info)
{
	RMLUI_ASSERT(!data);

	___renderer_type = Backend::Type::DirectX_12;

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

	/*
	TODO: [wl] delete
	As a standard we need to use namespace RmlRendererName and call from it Initialize function where we pass p_info structure
	Rml::Vector<const char*> extensions;
	extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
	extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
	if (!data->render_interface.Initialize(std::move(extensions), CreateVulkanSurface))
	{
	    DisplayError(window_handle, "Could not initialize Vulkan render interface.");
	    ::CloseWindow(window_handle);
	    data.reset();
	    return false;
	}
	*/

	if (!p_info)
	{
		// legacy calling, by default we think that user in that case wants to initialize renderer fully
		p_info = new RmlRenderInitInfo;

		p_info->p_native_window_handle = data->window_handle;
		p_info->is_full_initialization = true;
		p_info->settings.vsync = false;

		// remember pointer in order to delete it
		p_legacy_instance = p_info;
	}

	data->render_interface = RmlDX12::Initialize(nullptr, p_info);

	if (!data->render_interface)
	{
		DisplayError(window_handle, "Could not initialize DirectX 12 render interface.");
		::CloseWindow(window_handle);
		data.reset();
		return false;
	}

	data->system_interface.SetWindow(window_handle);
	data->render_interface->SetViewport(width, height);

	// Now we are ready to show the window.
	::ShowWindow(window_handle, SW_SHOW);
	::SetForegroundWindow(window_handle);
	::SetFocus(window_handle);

	return true;
}

RenderInterface_DX12* p_integration_renderer = nullptr;
// todo: mikke89 provide GetType method based on TypeSystemInterface enum in order to validate the real instatiated instance of Rml::SystemInterface
// what we passed as argument and what we expect in requested RmlRenderInitInfo
Rml::SystemInterface* p_integration_system_interface = nullptr;
Rml::FileInterface* p_integration_file_interface = nullptr;

#include "../include/PlatformExtensions.h"
#include "../include/ShellFileInterface.h"
Rml::Context* p_integration_context = nullptr;

Rml::Context* Backend::DX12::Initialize(RmlRenderInitInfo* p_info, Rml::SystemInterface* p_system_interface)
{
	RMLUI_ASSERT(!p_integration_renderer && "don't call twice because you already initialized renderer");
	RMLUI_ASSERT(!p_integration_system_interface && "don't call twice because you already initialized system interface");
	RMLUI_ASSERT(!p_integration_context && "don't call twice because you already initialized Rml::Context");
	RMLUI_ASSERT(!___renderer_context_dpi_enable &&
		"we need to set true only in initialize otherwise it signals that logic of init/shutdown is broken and you got invalid state");

	___renderer_context_dpi_enable = true;
	___renderer_type = Backend::Type::DirectX_12;

	if (p_integration_renderer)
		return p_integration_context;

	if (!p_info)
	{
		RMLUI_ASSERT(p_info && "you can't pass empty data to this function!");
		return p_integration_context;
	}

	if (data && data->render_interface)
	{
		RMLUI_ASSERT(data && data->render_interface &&
			"you have initialized shell's renderer you need to destroy it first then call this method in order to use integration backend");
		return p_integration_context;
	}

	if (p_info->backend_type != static_cast<unsigned char>(Backend::Type::DirectX_12))
	{
		RMLUI_ASSERT(p_info->backend_type == static_cast<unsigned char>(Backend::Type::DirectX_12) &&
			"you passed wrong backend type! Expected only DirectX-12, see enum Backend::Type");
		return p_integration_context;
	}

	if (!p_integration_renderer)
	{
		RMLUI_ASSERT(p_info->p_user_device && "you passed empty render device, failed to init render interface!");

		p_integration_renderer = RmlDX12::Initialize(nullptr, p_info);
	}

	if (!p_integration_renderer)
	{
		RMLUI_ASSERTMSG(false, "FATAL: can't initialize dx12 backend! :(");
		return p_integration_context;
	}

	if (!p_integration_system_interface)
	{
		RMLUI_ASSERT(static_cast<Backend::TypeSystemInterface>(p_info->system_interface_type) != Backend::TypeSystemInterface::Native_Unknown &&
			static_cast<Backend::TypeSystemInterface>(p_info->system_interface_type) != Backend::TypeSystemInterface::Library_Unknown &&
			"invalid system interface type");

		p_integration_system_interface = p_system_interface;
	}

	if (p_integration_renderer && p_integration_system_interface)
	{
		Rml::SetSystemInterface(p_integration_system_interface);

		// todo: mikke89 think about it maybe user can pass it is own file interface through RmlRenderInitInfo ?
		if (!p_integration_file_interface)
		{
			Rml::String root = PlatformExtensions::FindSamplesRoot();

			if (root.empty() == false)
			{
				p_integration_file_interface = new ShellFileInterface(root);
				Rml::SetFileInterface(p_integration_file_interface);
			}
		}

		Rml::SetRenderInterface(p_integration_renderer);
		Rml::Initialise();
	}

	if (!p_integration_context)
	{
		// since we have to update root if we don't alternate dimensions we can't execute code under if statement and thus we can't update our root,
		// todo: mikke89 think about it because in samples like load_document on ::Initialize we SetDimensions but after our window changes its
		// dimensions and we now go again to SetDimensions but for that calling (dpi) we can update root
		p_integration_context = Rml::CreateContext(p_info->context_name, {p_info->initial_width / 2, p_info->initial_height / 2});
	}

	return p_integration_context;
}

void Backend::DX12::Shutdown()
{
	bool only_one{};

	if (p_integration_renderer)
	{
		if (!data || !data->render_interface)
			only_one = true;
	}
	else
	{
		if (!data || data->render_interface)
			only_one = true;
	}

	RMLUI_ASSERTMSG(only_one, "you must have only one backend is for shell or integration you can use both!");

	if (p_integration_renderer)
	{
		Rml::Shutdown();
		if (p_integration_file_interface)
		{
			delete p_integration_file_interface;
			p_integration_file_interface = nullptr;
		}
		RmlDX12::Shutdown(p_integration_renderer);
		delete p_integration_renderer;
		p_integration_renderer = nullptr;

		delete p_integration_system_interface;
		p_integration_system_interface = nullptr;
	}
	else
	{
		RMLUI_ASSERT(data);
		RMLUI_ASSERT(data->render_interface);

		if (p_legacy_instance)
		{
			delete p_legacy_instance;
			p_legacy_instance = nullptr;
		}

		RmlDX12::Shutdown(data->render_interface);

		if (data->render_interface)
		{
			delete data->render_interface;
			data->render_interface = nullptr;
			::DestroyWindow(data->window_handle);
			::UnregisterClassW((LPCWSTR)data->instance_name.data(), data->instance_handle);
			data.reset();
		}
	}

	___renderer_type = Backend::Type::Unknown;
}

Rml::SystemInterface* Backend::GetSystemInterface()
{
	RMLUI_ASSERT(data);
	return &data->system_interface;
}

Rml::RenderInterface* Backend::GetRenderInterface()
{
	RMLUI_ASSERT(data);
	RMLUI_ASSERT(!(data->render_interface && p_integration_renderer) && (data->render_interface || p_integration_renderer) &&
		"you can't have both initialized things or no initialized things at all!");

	Rml::RenderInterface* p_result = nullptr;

	if (data->render_interface)
		p_result = data->render_interface;

	if (p_integration_renderer)
		p_result = p_integration_renderer;

	return p_result;
}

static bool NextEvent(MSG& message, UINT timeout)
{
	if (timeout != 0)
	{
		UINT_PTR timer_id = SetTimer(NULL, NULL, timeout, NULL);
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
	while (has_message || !data->render_interface->IsSwapchainValid())
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
		if (!data->render_interface->IsSwapchainValid())
			data->render_interface->RecreateSwapchain();

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
	RMLUI_ASSERT(data->render_interface);

	if (data->render_interface)
	{
		data->render_interface->BeginFrame();
		data->render_interface->Clear();
	}
}

void Backend::DX12::BeginFrame(void* p_input_rtv, void* p_input_dsv, unsigned char current_framebuffer_index)
{
	RMLUI_ASSERT(p_input_rtv &&
		"you must pass a valid resource based on your backend, this function is for DirectX-12 and expected input argument type is "
		"D3D12_CPU_DESCRIPTOR_HANDLE*");
	RMLUI_ASSERT(p_input_dsv &&
		"you must pass a valid resource based on your backend, this function is for DirectX-12 and expected input argument type is "
		"D3D12_CPU_DESCRIPTOR_HANDLE*");

	RMLUI_ASSERT(p_integration_renderer);

	// in order to indicate about some problem for user we don't render if user failed to call appropriate function, you can't mix callings that don't
	// belong to specific renderer's initialization route
	if (data && data->render_interface)
	{
		RMLUI_ASSERTMSG(false, "you initialize shell but call for integration!");
		return;
	}

	if (p_input_rtv && p_input_dsv)
	{
		if (p_integration_renderer)
		{
			p_integration_renderer->Set_UserFramebufferIndex(current_framebuffer_index);
			p_integration_renderer->Set_UserRenderTarget(p_input_rtv);
			p_integration_renderer->Set_UserDepthStencil(p_input_dsv);

			p_integration_renderer->BeginFrame();
			p_integration_renderer->Clear();
		}
	}
}

void Backend::DX12::ProcessEvents(Rml::Context* context, KeyDownCallback ikey_down_callback, const RmlProcessEventInfo& info, bool power_save)
{
	RMLUI_ASSERT(context);
	RMLUI_ASSERT(static_cast<Backend::Type>(___renderer_type) == Backend::Type::DirectX_12);
	RMLUI_ASSERT(p_integration_renderer);

	FIX_WARNING_UNUSED(ikey_down_callback);
	FIX_WARNING_UNUSED(power_save);

	// The initial window size may have been affected by system DPI settings, apply the actual pixel size and dp-ratio to the context.
	if (___renderer_context_dpi_enable)
	{
		RMLUI_ASSERT(___renderer_copy_info_tsi == Backend::TypeSystemInterface::Native_Win32 && "mismatch and failed init?");

		___renderer_context_dpi_enable = false;
		const float dp_ratio = GetDensityIndependentPixelRatio(static_cast<HWND>(___renderer_context_hwnd));
		context->SetDimensions({___renderer_initial_width, ___renderer_initial_height});
		context->SetDensityIndependentPixelRatio(dp_ratio);
	}

	switch (info.msg)
	{
	case WM_DPICHANGED:
	{
		RECT* new_pos = (RECT*)info.lParam;
		SetWindowPos(info.hwnd, NULL, new_pos->left, new_pos->top, new_pos->right - new_pos->left, new_pos->bottom - new_pos->top,
			SWP_NOZORDER | SWP_NOACTIVATE);
		if (context && has_dpi_support)
			context->SetDensityIndependentPixelRatio(GetDensityIndependentPixelRatio(info.hwnd));

		break;
	}
	case WM_KEYDOWN:
	{
		KeyDownCallback key_down_callback = ___renderer_key_down_callback;

		const Rml::Input::KeyIdentifier rml_key = RmlWin32::ConvertKey((int)info.wParam);
		const int rml_modifier = RmlWin32::GetKeyModifierState();
		const float native_dp_ratio = GetDensityIndependentPixelRatio(info.hwnd);

		// See if we have any global shortcuts that take priority over the context.
		if (key_down_callback && !key_down_callback(context, rml_key, rml_modifier, native_dp_ratio, true))
		{}

		// Otherwise, hand the event over to the context by calling the input handler as normal.
		if (!RmlWin32::WindowProcedure(context, data->text_input_method_editor, info.hwnd, info.msg, info.wParam, info.lParam))
		{}

		// The key was not consumed by the context either, try keyboard shortcuts of lower priority.
		if (key_down_callback && !key_down_callback(context, rml_key, rml_modifier, native_dp_ratio, false))
		{}

		break;
	}
	default:
	{
		// nothing
		break;
	}
	}
}

void Backend::DX12::Resize(Rml::Context* p_context, int width, int height)
{
	RMLUI_ASSERT(p_integration_renderer && "must be valid, early calling?");

	if (data && data->render_interface)
	{
		RMLUI_ASSERTMSG(false,
			"you can't have initailize shell backend renderer while you initialize integration renderer! Destroy some of them and use appropriate "
			"functions for handling rendering");
		return;
	}

	if (p_context)
	{
		p_context->SetDimensions({width, height});
	}

	if (p_integration_renderer)
	{
		p_integration_renderer->SetViewport(width, height);
	}
}

void Backend::PresentFrame()
{
	RMLUI_ASSERT(data);
	RMLUI_ASSERT(data->render_interface);

	// in order to indicate about some problem for user we don't render if user failed to call appropriate function, you can't mix callings that don't
	// belong to specific renderer's initialization route
	if (p_integration_renderer)
	{
		RMLUI_ASSERTMSG(false, "you initialize integration but call for shell!");
		return;
	}

	if (data->render_interface)
	{
		data->render_interface->EndFrame();
	}

	// Optional, used to mark frames during performance profiling.
	RMLUI_FrameMark;
}

void Backend::DX12::EndFrame()
{
	RMLUI_ASSERT(p_integration_renderer);

	// in order to indicate about some problem for user we don't render if user failed to call appropriate function, you can't mix callings that don't
	// belong to specific renderer's initialization route
	if (data && data->render_interface)
	{
		RMLUI_ASSERTMSG(false, "you initialize shell but call for integration!");
		return;
	}

	if (p_integration_renderer)
	{
		p_integration_renderer->EndFrame();
	}

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
			data->render_interface->SetViewport(width, height);
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
		DisplayError(NULL, "Could not register window class.");
		return nullptr;
	}

	HWND window_handle = CreateWindowExW(WS_EX_APPWINDOW | WS_EX_WINDOWEDGE,
		name.data(),                                                                // Window class name.
		name.data(), WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_OVERLAPPEDWINDOW, 0, 0, // Window position.
		0, 0,                                                                       // Window size.
		nullptr, nullptr, instance_handle, nullptr);

	if (!window_handle)
	{
		DisplayError(NULL, "Could not create window.");
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

	// Resize the window.
	SetWindowPos(window_handle, HWND_TOP, 0, 0, window_rect.right - window_rect.left, window_rect.bottom - window_rect.top, SWP_NOACTIVATE);

	return window_handle;
}

/*
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
*/