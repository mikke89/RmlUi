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
#include <ShellOpenGL.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Types.h>
#include "ShellFileInterface.h"
#include <x11/InputX11.h>
#include <X11/Xlib.h>
#include <X11/extensions/xf86vmode.h>
#include <X11/cursorfont.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <limits.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

static Rml::Context* context = nullptr;
static ShellRenderInterfaceExtensions* shell_renderer = nullptr;

static bool running = false;
static int screen = -1;
static timeval start_time;
static Rml::String clipboard_text;
static int window_width = 0;
static int window_height = 0;

static Rml::UniquePtr<ShellFileInterface> file_interface;

static Display* display = nullptr;
static XVisualInfo* visual_info = nullptr;
static Window window = 0;

static Cursor cursor_default = 0;
static Cursor cursor_move = 0;
static Cursor cursor_pointer = 0;
static Cursor cursor_resize = 0;
static Cursor cursor_cross = 0;
static Cursor cursor_text = 0;
static Cursor cursor_unavailable = 0;

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

static bool isRegularFile(const Rml::String& path)
{
	struct stat sb;
	return (stat(path.c_str(), &sb) == 0 && S_ISREG(sb.st_mode));
}

bool Shell::Initialise()
{
	gettimeofday(&start_time, nullptr);
	if (!InputX11::Initialise())
		return false;

	Rml::String root = FindSamplesRoot();
	bool result = !root.empty();

	file_interface = Rml::MakeUnique<ShellFileInterface>(root);
	Rml::SetFileInterface(file_interface.get());

	return result;
}

void Shell::Shutdown()
{
	InputX11::Shutdown();

	file_interface.reset();
}

Rml::String Shell::FindSamplesRoot()
{
	char executable_file_name[PATH_MAX];
	ssize_t len = readlink("/proc/self/exe", executable_file_name, PATH_MAX);
	if (len == -1) {
		printf("Unable to determine the executable path!\n");
		executable_file_name[0] = 0;
	} else {
		// readlink() does not append a null byte to buf.
		executable_file_name[len] = 0;
	}
	Rml::String executable_path = Rml::String(executable_file_name);
	executable_path = executable_path.substr(0, executable_path.rfind("/") + 1);
	
	// We assume we have found the correct path if we can find the lookup file from it.
	const char* lookup_file = "assets/rml.rcss";

	// For "../Samples/" to be valid we must be in the Build directory.
	// If "../" is valid we are probably in the installation directory.
	// Some build setups may nest the executables deeper in a build directory, try them last.
	const char* candidate_paths[] = { "", "../", "../Samples/", "../../Samples/", "../../../Samples/", "../../../../Samples/" };

	for (const char* relative_path : candidate_paths)
	{
		Rml::String absolute_path = executable_path + relative_path;
		Rml::String absolute_lookup_file = absolute_path + lookup_file;

		if (isRegularFile(absolute_lookup_file))
		{
			return absolute_path;
		}
	}

	printf("Unable to find the path to the samples root!\n");

	return Rml::String();
}

bool Shell::OpenWindow(const char* name, ShellRenderInterfaceExtensions *_shell_renderer, unsigned int width, unsigned int height, bool allow_resize)
{
	display = XOpenDisplay(0);
	if (display == nullptr)
		return false;

	window_width = width;
	window_height = height;

	// This initialise they keyboard to keycode mapping system of X11
	// itself.  It must be done here as it needs to query the connected
	// X server display for information about its install keymap abilities.
	InputX11::InitialiseX11Keymap(display);

	screen = XDefaultScreen(display);

	// Fetch an appropriate 32-bit visual interface.
	int attribute_list[] = {GLX_RGBA,
							GLX_DOUBLEBUFFER,
							GLX_RED_SIZE, 8,
							GLX_GREEN_SIZE, 8,
							GLX_BLUE_SIZE, 8,
							GLX_DEPTH_SIZE, 24,
							GLX_STENCIL_SIZE, 8,
							0L};

	visual_info = glXChooseVisual(display, screen, attribute_list);
	if (visual_info == nullptr)
	{
		return false;
  	}


	// Build up our window attributes.
	XSetWindowAttributes window_attributes;
	window_attributes.colormap = XCreateColormap(display, RootWindow(display, visual_info->screen), visual_info->visual, AllocNone);
	window_attributes.border_pixel = 0;
	window_attributes.event_mask = ExposureMask | KeyPressMask | ButtonPressMask | StructureNotifyMask;

	// Create the window.
	window = XCreateWindow(display,
						   RootWindow(display, visual_info->screen),
						   0, 0,
						   width, height,
						   0,
						   visual_info->depth,
						   InputOutput,
						   visual_info->visual,
						   CWBorderPixel | CWColormap | CWEventMask,
						   &window_attributes);

	// Handle delete events in windowed mode.
	Atom delete_atom = XInternAtom(display, "WM_DELETE_WINDOW", True);
	XSetWMProtocols(display, window, &delete_atom, 1);

	// Capture the events we're interested in.
	XSelectInput(display, window, KeyPressMask |
								  KeyReleaseMask |
								  ButtonPressMask |
								  ButtonReleaseMask |
								  PointerMotionMask |
								  StructureNotifyMask);

	if(!allow_resize)
	{
		// Force the window to remain at the fixed size by asking the window manager nicely, it may choose to ignore us
		XSizeHints* win_size_hints = XAllocSizeHints();		// Allocate a size hint structure
		if (win_size_hints == nullptr)
		{
			fprintf(stderr, "XAllocSizeHints - out of memory\n");
		}
		else
		{
			// Initialize the structure and specify which hints will be providing
			win_size_hints->flags = PSize | PMinSize | PMaxSize;

			// Set the sizes we want the window manager to use
			win_size_hints->base_width = width;
			win_size_hints->base_height = height;
			win_size_hints->min_width = width;
			win_size_hints->min_height = height;
			win_size_hints->max_width = width;
			win_size_hints->max_height = height;

			// {ass the size hints to the window manager.
			XSetWMNormalHints(display, window, win_size_hints);

			// Free the size buffer
			XFree(win_size_hints);
		}
	}

	{
		// Create cursors
		cursor_default = XCreateFontCursor(display, XC_left_ptr);;
		cursor_move = XCreateFontCursor(display, XC_fleur);
		cursor_pointer = XCreateFontCursor(display, XC_hand1);
		cursor_resize = XCreateFontCursor(display, XC_sizing);
		cursor_cross = XCreateFontCursor(display, XC_crosshair);
		cursor_text = XCreateFontCursor(display, XC_xterm);
		cursor_unavailable = XCreateFontCursor(display, XC_X_cursor);
	}

	// Set the window title and show the window.
	XSetStandardProperties(display, window, name, "", 0L, nullptr, 0, nullptr);
	XMapRaised(display, window);

	shell_renderer = _shell_renderer;
	if(shell_renderer != nullptr)
	{
		struct __X11NativeWindowData nwData;
		nwData.display = display;
		nwData.window = window;
		nwData.visual_info = visual_info;
		return shell_renderer->AttachToNative(&nwData);
	}
    return true;
}

void Shell::CloseWindow()
{
	if(shell_renderer != nullptr)
	{
		shell_renderer->DetachFromNative();
	}

	if (display != nullptr)
	{
		XCloseDisplay(display);
		display = nullptr;
	}
}

// Returns a platform-dependent handle to the window.
void* Shell::GetWindowHandle()
{
	return nullptr;
}

void Shell::EventLoop(ShellIdleFunction idle_function)
{
	running = true;

	// Loop on Peek/GetMessage until and exit has been requested
	while (running)
	{
		while (XPending(display) > 0)
		{
			XEvent event;
			char *event_type = nullptr;
			XNextEvent(display, &event);

			switch (event.type)
			{
				case ClientMessage: 
				{
					// The only message we register for is WM_DELETE_WINDOW, so if we receive a client message then the
					// window has been closed.
					event_type = XGetAtomName(display, event.xclient.message_type);
					if (strcmp(event_type, "WM_PROTOCOLS") == 0)
						running = false;
					XFree(event_type);
					event_type = nullptr;
				}
				break;

				case ConfigureNotify: 
				{
					int x = event.xconfigure.width;
					int y = event.xconfigure.height;

					UpdateWindowDimensions(x, y);
				}
				break;
				
				default:
				{
					InputX11::ProcessXEvent(display, event);
				}
			}
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

	printf("%s", buffer);
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

	printf("%s", buffer);
}

// Returns the seconds that have elapsed since program startup.
double Shell::GetElapsedTime() 
{
	struct timeval now;

	gettimeofday(&now, nullptr);

	double sec = now.tv_sec - start_time.tv_sec;
	double usec = now.tv_usec - start_time.tv_usec;
	double result = sec + (usec / 1000000.0);

	return result;
}

void Shell::SetMouseCursor(const Rml::String& cursor_name)
{
	if (display && window)
	{
		Cursor cursor_handle = 0;
		if (cursor_name.empty() || cursor_name == "arrow")
			cursor_handle = cursor_default;
		else if (cursor_name == "move")
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
			XDefineCursor(display, window, cursor_handle);
		}
	}
}

void Shell::SetClipboardText(const Rml::String& text)
{
	// Todo: interface with system clipboard
	clipboard_text = text;
}

void Shell::GetClipboardText(Rml::String& text)
{
	// Todo: interface with system clipboard
	text = clipboard_text;
}

void Shell::SetContext(Rml::Context* new_context)
{
	context = new_context;
	UpdateWindowDimensions();
}
