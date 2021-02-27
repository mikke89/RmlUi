/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019 The RmlUi Team, and contributors
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

#include <Shell.h>
#include <RmlUi/Core.h>
#include <win32/InputWin32.h>
#include "ShellFileInterface.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <shlwapi.h>

static LRESULT CALLBACK WindowProcedure(HWND window_handle, UINT message, WPARAM w_param, LPARAM l_param);

static Rml::Context* context = nullptr;
static ShellRenderInterfaceExtensions* shell_renderer = nullptr;

static bool activated = true;
static bool running = false;
static Rml::U16String instance_name;
static HWND window_handle = nullptr;
static HINSTANCE instance_handle = nullptr;

static bool has_dpi_support = false;
static UINT window_dpi = USER_DEFAULT_SCREEN_DPI;
static int window_width = 0;
static int window_height = 0;

static double time_frequency;
static LARGE_INTEGER time_startup;

static Rml::UniquePtr<ShellFileInterface> file_interface;

static HCURSOR cursor_default = nullptr;
static HCURSOR cursor_move = nullptr;
static HCURSOR cursor_pointer = nullptr;
static HCURSOR cursor_resize= nullptr;
static HCURSOR cursor_cross = nullptr;
static HCURSOR cursor_text = nullptr;
static HCURSOR cursor_unavailable = nullptr;


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

static ProcSetProcessDpiAwarenessContext procSetProcessDpiAwarenessContext = NULL;
static ProcGetDpiForWindow procGetDpiForWindow = NULL;
static ProcAdjustWindowRectExForDpi procAdjustWindowRectExForDpi = NULL;


static void UpdateDpi()
{
	if (has_dpi_support)
	{
		UINT dpi = procGetDpiForWindow(window_handle);
		if (dpi != 0)
		{
			window_dpi = dpi;
			if (context)
				context->SetDensityIndependentPixelRatio(Shell::GetDensityIndependentPixelRatio());
		}
	}
}

static void UpdateWindowDimensions(int width = 0, int height = 0)
{
	if (width > 0)
		window_width = width;
	if (height > 0)
		window_height = height;
	if (context)
		context->SetDimensions(Rml::Vector2i(window_width, window_height));
	if (shell_renderer)
		shell_renderer->SetViewport(window_width, window_height);
}

bool Shell::Initialise()
{
	instance_handle = GetModuleHandle(nullptr);
	InputWin32::Initialise();

	LARGE_INTEGER time_ticks_per_second;
	QueryPerformanceFrequency(&time_ticks_per_second);
	QueryPerformanceCounter(&time_startup);

	time_frequency = 1.0 / (double) time_ticks_per_second.QuadPart;

	// Load cursors
	cursor_default = LoadCursor(nullptr, IDC_ARROW);
	cursor_move = LoadCursor(nullptr, IDC_SIZEALL);
	cursor_pointer = LoadCursor(nullptr, IDC_HAND);
	cursor_resize = LoadCursor(nullptr, IDC_SIZENWSE);
	cursor_cross = LoadCursor(nullptr, IDC_CROSS);
	cursor_text = LoadCursor(nullptr, IDC_IBEAM);
	cursor_unavailable = LoadCursor(nullptr, IDC_NO);

	Rml::String root = FindSamplesRoot();
	bool result = !root.empty();
	
	file_interface = Rml::MakeUnique<ShellFileInterface>(root);
	Rml::SetFileInterface(file_interface.get());

	// See if we have Per Monitor V2 DPI awareness. Requires Windows 10, version 1703.
	// Cast function pointers to void* first for MinGW not to emit errors.
	procSetProcessDpiAwarenessContext = (ProcSetProcessDpiAwarenessContext)(void*)GetProcAddress(
		GetModuleHandle(TEXT("User32.dll")),
		"SetProcessDpiAwarenessContext"
	);
	procGetDpiForWindow = (ProcGetDpiForWindow)(void*)GetProcAddress(
		GetModuleHandle(TEXT("User32.dll")),
		"GetDpiForWindow"
	);
	procAdjustWindowRectExForDpi = (ProcAdjustWindowRectExForDpi)(void*)GetProcAddress(
		GetModuleHandle(TEXT("User32.dll")),
		"AdjustWindowRectExForDpi"
	);

	has_dpi_support = (procSetProcessDpiAwarenessContext != NULL && procGetDpiForWindow != NULL && procAdjustWindowRectExForDpi != NULL);

	return result;
}

void Shell::Shutdown()
{
	InputWin32::Shutdown();

	file_interface.reset();
}

Rml::String Shell::FindSamplesRoot()
{
	const char* candidate_paths[] = { "", "..\\Samples\\", "..\\..\\Samples\\", "..\\..\\..\\Samples\\", "..\\..\\..\\..\\Samples\\" };
	
	// Fetch the path of the executable, test the candidate paths appended to that.
	char executable_file_name[MAX_PATH];
	if (GetModuleFileNameA(instance_handle, executable_file_name, MAX_PATH) >= MAX_PATH &&
		GetLastError() == ERROR_INSUFFICIENT_BUFFER)
	{
		executable_file_name[0] = 0;
	}

	Rml::String executable_path(executable_file_name);
	executable_path = executable_path.substr(0, executable_path.rfind('\\') + 1);

	// We assume we have found the correct path if we can find the lookup file from it
	const char* lookup_file = "assets\\rml.rcss";

	for(const char* relative_path : candidate_paths)
	{
		Rml::String absolute_path = executable_path + relative_path;

		if (PathFileExistsA(Rml::String(absolute_path + lookup_file).c_str()))
		{
			char canonical_path[MAX_PATH];
			if (!PathCanonicalizeA(canonical_path, absolute_path.c_str()))
				canonical_path[0] = 0;

			return Rml::String(canonical_path);
		}
	}

	return Rml::String();
}


bool Shell::OpenWindow(const char* in_name, ShellRenderInterfaceExtensions *_shell_renderer, unsigned int width, unsigned int height, bool allow_resize)
{
	// Activate Per Monitor V2.
	if (has_dpi_support && !procSetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2))
	{
		has_dpi_support = false;
	}

	WNDCLASSW window_class;

	Rml::U16String name = Rml::StringUtilities::ToUTF16(Rml::String(in_name));

	// Fill out the window class struct.
	window_class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	window_class.lpfnWndProc = WindowProcedure;
	window_class.cbClsExtra = 0;
	window_class.cbWndExtra = 0;
	window_class.hInstance = instance_handle;
	window_class.hIcon = LoadIcon(nullptr, IDI_WINLOGO);
	window_class.hCursor = cursor_default;
	window_class.hbrBackground = nullptr;
	window_class.lpszMenuName = nullptr;
	window_class.lpszClassName = (LPCWSTR)name.data();

	if (!RegisterClassW(&window_class))
	{
		DisplayError("Could not register window class.");

		CloseWindow();
		return false;
	}

	window_handle = CreateWindowExW(WS_EX_APPWINDOW | WS_EX_WINDOWEDGE,
								   (LPCWSTR)name.data(),	// Window class name.
								   (LPCWSTR)name.data(),
								   WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_OVERLAPPEDWINDOW,
								   0, 0,	// Window position.
								   0, 0,// Window size.
								   nullptr,
								   nullptr,
								   instance_handle,
								   nullptr);
	if (!window_handle)
	{
		DisplayError("Could not create window.");
		CloseWindow();

		return false;
	}

	window_width = width;
	window_height = height;

	UpdateDpi();
	window_width = (width * window_dpi) / USER_DEFAULT_SCREEN_DPI;
	window_height = (height * window_dpi) / USER_DEFAULT_SCREEN_DPI;

	instance_name = name;

	DWORD style = (allow_resize ? WS_OVERLAPPEDWINDOW : (WS_OVERLAPPEDWINDOW & ~WS_SIZEBOX & ~WS_MAXIMIZEBOX));
	DWORD extended_style = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;

	// Adjust the window size to take into account the edges
	RECT window_rect;
	window_rect.top = 0;
	window_rect.left = 0;
	window_rect.right = window_width;
	window_rect.bottom = window_height;
	if (has_dpi_support)
		procAdjustWindowRectExForDpi(&window_rect, style, FALSE, extended_style, window_dpi);
	else
		AdjustWindowRectEx(&window_rect, style, FALSE, extended_style);

	SetWindowLong(window_handle, GWL_EXSTYLE, extended_style);
	SetWindowLong(window_handle, GWL_STYLE, style);

	if (_shell_renderer != nullptr)
	{
		shell_renderer = _shell_renderer;
		if(!shell_renderer->AttachToNative(window_handle))
		{
			CloseWindow();
			return false;
		}
	}

	UpdateWindowDimensions();

	// Resize the window.
	SetWindowPos(window_handle, HWND_TOP, 0, 0, window_rect.right - window_rect.left, window_rect.bottom - window_rect.top, SWP_NOACTIVATE);

	// Display the new window
	ShowWindow(window_handle, SW_SHOW);
	SetForegroundWindow(window_handle);
	SetFocus(window_handle);

    return true;
}

void Shell::CloseWindow()
{
	if(shell_renderer) {
		shell_renderer->DetachFromNative();
	}

	DestroyWindow(window_handle);  
	UnregisterClassW((LPCWSTR)instance_name.data(), instance_handle);
}

// Returns a platform-dependent handle to the window.
void* Shell::GetWindowHandle()
{
	return window_handle;
}

void Shell::EventLoop(ShellIdleFunction idle_function)
{
	MSG message;
	running = true;

	// Loop on PeekMessage() / GetMessage() until exit has been requested.
	while (running)
	{
		if (PeekMessage(&message, nullptr, 0, 0, PM_NOREMOVE))
		{
			GetMessage(&message, nullptr, 0, 0);

			TranslateMessage(&message);
			DispatchMessage(&message);
		}

		idle_function();
	}
}

void Shell::RequestExit()
{
	running = false;
}

void Shell::DisplayError(const char* fmt, ...)
{
	const int buffer_size = 1024;
	char buffer[buffer_size];
	va_list argument_list;

	// Print the message to the buffer.
	va_start(argument_list, fmt);
	int len = vsnprintf(buffer, buffer_size - 2, fmt, argument_list);	
	if ( len < 0 || len > buffer_size - 2 )	
	{
		len = buffer_size - 2;
	}	
	buffer[len] = '\n';
	buffer[len + 1] = '\0';
	va_end(argument_list);

	MessageBoxW(window_handle, (LPCWSTR)Rml::StringUtilities::ToUTF16(buffer).c_str(), L"Shell Error", MB_OK);
}

void Shell::Log(const char* fmt, ...)
{
	const int buffer_size = 1024;
	char buffer[buffer_size];
	va_list argument_list;

	// Print the message to the buffer.
	va_start(argument_list, fmt);
	int len = vsnprintf(buffer, buffer_size - 2, fmt, argument_list);	
	if ( len < 0 || len > buffer_size - 2 )	
	{
		len = buffer_size - 2;
	}	
	buffer[len] = '\n';
	buffer[len + 1] = '\0';
	va_end(argument_list);

	OutputDebugStringW((LPCWSTR)Rml::StringUtilities::ToUTF16(buffer).c_str());
}

double Shell::GetElapsedTime() 
{
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);

	return double(counter.QuadPart - time_startup.QuadPart) * time_frequency;
}

void Shell::SetMouseCursor(const Rml::String& cursor_name)
{
	if (window_handle)
	{
		HCURSOR cursor_handle = nullptr;
		if (cursor_name.empty() || cursor_name == "arrow")
			cursor_handle = cursor_default;
		else if(cursor_name == "move")
			cursor_handle = cursor_move;
		else if (cursor_name == "pointer")
			cursor_handle = cursor_pointer;
		else if (cursor_name == "resize")
			cursor_handle = cursor_resize;
		else if (cursor_name == "cross")
			cursor_handle = cursor_cross;
		else if (cursor_name == "text")
			cursor_handle = cursor_text;
		else if (cursor_name == "unavailable")
			cursor_handle = cursor_unavailable;

		if (cursor_handle)
		{
			SetCursor(cursor_handle);
			SetClassLongPtrA(window_handle, GCLP_HCURSOR, (LONG_PTR)cursor_handle);
		}
	}
}

void Shell::SetClipboardText(const Rml::String& text_utf8)
{
	if (window_handle)
	{
		if (!OpenClipboard(window_handle))
			return;

		EmptyClipboard();

		const Rml::U16String text = Rml::StringUtilities::ToUTF16(text_utf8);

		size_t size = sizeof(char16_t) * (text.size() + 1);

		HGLOBAL clipboard_data = GlobalAlloc(GMEM_FIXED, size);
		memcpy(clipboard_data, text.data(), size);

		if (SetClipboardData(CF_UNICODETEXT, clipboard_data) == nullptr)
		{
			CloseClipboard();
			GlobalFree(clipboard_data);
		}
		else
			CloseClipboard();
	}
}

void Shell::GetClipboardText(Rml::String& text)
{
	if (window_handle)
	{
		if (!OpenClipboard(window_handle))
			return;

		HANDLE clipboard_data = GetClipboardData(CF_UNICODETEXT);
		if (clipboard_data == nullptr)
		{
			CloseClipboard();
			return;
		}

		const char16_t* clipboard_text = (const char16_t*)GlobalLock(clipboard_data);
		if (clipboard_text)
			text = Rml::StringUtilities::ToUTF8(clipboard_text);
		GlobalUnlock(clipboard_data);

		CloseClipboard();
	}
}

void Shell::SetContext(Rml::Context* new_context)
{
	context = new_context;
	UpdateDpi();
	UpdateWindowDimensions();
}

float Shell::GetDensityIndependentPixelRatio()
{
	return float(window_dpi) / float(USER_DEFAULT_SCREEN_DPI);
}

static LRESULT CALLBACK WindowProcedure(HWND local_window_handle, UINT message, WPARAM w_param, LPARAM l_param)
{
	// See what kind of message we've got.
	switch (message)
	{		
		case WM_ACTIVATE:
		{
			if (LOWORD(w_param) != WA_INACTIVE)
			{
				activated = true;
			}
			else
			{
				activated = false;
			}
		}
		break;

		// When the window closes, request exit
		case WM_CLOSE:
		{
			running = false;
			return 0;
		}
		break;

		case WM_SIZE:
		{
			int width = LOWORD(l_param);
			int height = HIWORD(l_param);
			UpdateWindowDimensions(width, height);
		}
		break;

		case WM_DPICHANGED:
		{
			UpdateDpi();

			RECT* const new_pos = (RECT*)l_param;
			SetWindowPos(window_handle,
				NULL,
				new_pos->left,
				new_pos->top,
				new_pos->right - new_pos->left,
				new_pos->bottom - new_pos->top,
				SWP_NOZORDER | SWP_NOACTIVATE);
		}
		break;

		default:
		{
			InputWin32::ProcessWindowsEvent(local_window_handle, message, w_param, l_param);
		}
		break;
	}

	// All unhandled messages go to DefWindowProc.
	return DefWindowProc(local_window_handle, message, w_param, l_param);
}
