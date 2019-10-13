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
#include <stdio.h>
#include <stdarg.h>

static LRESULT CALLBACK WindowProcedure(HWND window_handle, UINT message, WPARAM w_param, LPARAM l_param);

static bool activated = true;
static bool running = false;
static Rml::Core::U16String instance_name;
static HWND window_handle = nullptr;
static HINSTANCE instance_handle = nullptr;

static double time_frequency;
static LARGE_INTEGER time_startup;

static std::unique_ptr<ShellFileInterface> file_interface;

static HCURSOR cursor_default = nullptr;
static HCURSOR cursor_move = nullptr;
static HCURSOR cursor_cross = nullptr;
static HCURSOR cursor_unavailable = nullptr;


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
	cursor_cross = LoadCursor(nullptr, IDC_CROSS);
	cursor_unavailable = LoadCursor(nullptr, IDC_NO);

	Rml::Core::String root = FindSamplesRoot();
	
	file_interface = std::make_unique<ShellFileInterface>(root);
	Rml::Core::SetFileInterface(file_interface.get());

	return true;
}

void Shell::Shutdown()
{
	InputWin32::Shutdown();

	file_interface.reset();
}

Rml::Core::String Shell::FindSamplesRoot()
{
	Rml::Core::String path = "../../Samples/";
	
	// Fetch the path of the executable, append the path onto that.
	char executable_file_name[MAX_PATH];
	if (GetModuleFileNameA(instance_handle, executable_file_name, MAX_PATH) >= MAX_PATH &&
		GetLastError() == ERROR_INSUFFICIENT_BUFFER)
	{
		executable_file_name[0] = 0;
	}

	Rml::Core::String executable_path = Rml::Core::String(executable_file_name);
	executable_path = executable_path.substr(0, executable_path.rfind("\\") + 1);
	
	return executable_path + path;
}

static ShellRenderInterfaceExtensions *shell_renderer = nullptr;
bool Shell::OpenWindow(const char* in_name, ShellRenderInterfaceExtensions *_shell_renderer, unsigned int width, unsigned int height, bool allow_resize)
{
	WNDCLASSW window_class;

	Rml::Core::U16String name = Rml::Core::StringUtilities::ToUTF16(Rml::Core::String(in_name));

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
								   width, height,// Window size.
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

	instance_name = name;

	DWORD style = (allow_resize ? WS_OVERLAPPEDWINDOW : (WS_OVERLAPPEDWINDOW & ~WS_SIZEBOX & ~WS_MAXIMIZEBOX));
	DWORD extended_style = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;

	// Adjust the window size to take into account the edges
	RECT window_rect;
	window_rect.top = 0;
	window_rect.left = 0;
	window_rect.right = width;
	window_rect.bottom = height;
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

	MessageBox(window_handle, (LPCWSTR)Rml::Core::StringUtilities::ToUTF16(buffer).c_str(), L"Shell Error", MB_OK);
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

	OutputDebugString((LPCWSTR)Rml::Core::StringUtilities::ToUTF16(buffer).c_str());
}

double Shell::GetElapsedTime() 
{
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);

	return double(counter.QuadPart - time_startup.QuadPart) * time_frequency;
}

void Shell::SetMouseCursor(const Rml::Core::String& cursor_name)
{
	if (window_handle)
	{
		HCURSOR cursor_handle = nullptr;
		if (cursor_name.empty())
			cursor_handle = cursor_default;
		else if(cursor_name == "move")
			cursor_handle = cursor_move;
		else if (cursor_name == "cross")
			cursor_handle = cursor_cross;
		else if (cursor_name == "unavailable")
			cursor_handle = cursor_unavailable;

		if (cursor_handle)
		{
			SetCursor(cursor_handle);
			SetClassLongPtrA(window_handle, GCLP_HCURSOR, (LONG_PTR)cursor_handle);
		}
	}
}

void Shell::SetClipboardText(const Rml::Core::String& text_utf8)
{
	if (window_handle)
	{
		if (!OpenClipboard(window_handle))
			return;

		EmptyClipboard();

		const Rml::Core::U16String text = Rml::Core::StringUtilities::ToUTF16(text_utf8);

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

void Shell::GetClipboardText(Rml::Core::String& text)
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
			text = Rml::Core::StringUtilities::ToUTF8(clipboard_text);
		GlobalUnlock(clipboard_data);

		CloseClipboard();
	}
}

static LRESULT CALLBACK WindowProcedure(HWND window_handle, UINT message, WPARAM w_param, LPARAM l_param)
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
			shell_renderer->SetViewport(width, height);
		}
		break;

		default:
		{
			InputWin32::ProcessWindowsEvent(message, w_param, l_param);
		}
		break;
	}

	// All unhandled messages go to DefWindowProc.
	return DefWindowProc(window_handle, message, w_param, l_param);
}
