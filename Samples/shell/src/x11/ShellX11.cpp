/*
 * This source file is part of libRocket, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://www.librocket.com
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
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
#include <Rocket/Core.h>
#include "ShellFileInterface.h"
#include <x11/InputX11.h>
#include <X11/Xlib.h>
#include <X11/extensions/xf86vmode.h>
#include <GL/glx.h>
#include <GL/gl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>

static bool running = false;
static Display* display = NULL;
static int screen = -1;
static XVisualInfo* visual_info = NULL;
static Window window = 0;
static GLXContext gl_context = NULL;
static timeval start_time;

static ShellFileInterface* file_interface = NULL;

bool Shell::Initialise(const Rocket::Core::String& path)
{
	gettimeofday(&start_time, NULL);
	InputX11::Initialise();

	file_interface = new ShellFileInterface(path);
	Rocket::Core::SetFileInterface(file_interface);

	return true;
}

void Shell::Shutdown()
{
	InputX11::Shutdown();

	delete file_interface;
	file_interface = NULL;
}

bool Shell::OpenWindow(const char* name, bool attach_opengl)
{
	display = XOpenDisplay(0);
	if (display == NULL)
		return false;

	screen = XDefaultScreen(display);

	// Fetch an appropriate 32-bit visual interface.
	int attribute_list[] = {GLX_RGBA,
							GLX_DOUBLEBUFFER,
							GLX_RED_SIZE, 8,
							GLX_GREEN_SIZE, 8,
							GLX_BLUE_SIZE, 8,
							GLX_DEPTH_SIZE, 24,
							None};

	visual_info = glXChooseVisual(display, screen, attribute_list);
	if (visual_info == NULL)
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
						   1024, 768,
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

	// Set the window title and show the window.
	XSetStandardProperties(display, window, name, "", None, NULL, 0, NULL);
	XMapRaised(display, window);

	gl_context = glXCreateContext(display, visual_info, NULL, GL_TRUE);
	if (gl_context == NULL)
		return false;

	if (!glXMakeCurrent(display, window, gl_context))
		return false;

	if (!glXIsDirect(display, gl_context))
		Log("OpenGL context does not support direct rendering; performance is likely to be poor.");

	Window root_window;
	int x, y;
	unsigned int width, height;
	unsigned int border_width, depth;
	XGetGeometry(display, window, &root_window, &x, &y, &width, &height, &border_width, &depth);

	// Set up the GL state.
	glClearColor(0, 0, 0, 1);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 1024, 768, 0, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

    return true;
}

void Shell::CloseWindow()
{
	if (gl_context != NULL)
	{
		glXMakeCurrent(display, None, NULL);
		glXDestroyContext(display, gl_context);
		gl_context = NULL;
	}

	if (display != NULL)
	{
		XCloseDisplay(display);
		display = NULL;
	}
}

// Returns a platform-dependent handle to the window.
void* Shell::GetWindowHandle()
{
	return NULL;
}

// Flips the OpenGL buffers.
void Shell::FlipBuffers()
{
	glXSwapBuffers(display, window);
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
			XNextEvent(display, &event);

			switch (event.type)
			{
				case ClientMessage: 
				{
					// The only message we register for is WM_DELETE_WINDOW, so if we receive a client message then the
					// window has been closed.
					if (strcmp(XGetAtomName(display, event.xclient.message_type), "WM_PROTOCOLS") == 0)
						running = false;
				}
				break;

				case ConfigureNotify: 
				{
					int x = event.xconfigure.width;
					int y = event.xconfigure.height;
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
float Shell::GetElapsedTime() 
{
	struct timeval now;

	gettimeofday(&now, NULL);

	double sec = now.tv_sec - start_time.tv_sec;
	double usec = now.tv_usec - start_time.tv_usec;
	double result = sec + (usec / 1000000.0);

	return (float)result;
}
