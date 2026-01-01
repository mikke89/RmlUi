#include "RmlUi_Backend.h"
#include "RmlUi_Include_Xlib.h"
#include "RmlUi_Platform_X11.h"
#include "RmlUi_Renderer_GL2.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Debug.h>
#include <RmlUi/Core/Log.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glx.h>
#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <X11/extensions/xf86vmode.h>
#include <cmath>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

// Attach the OpenGL context to the window.
static bool AttachToNative(GLXContext& out_gl_context, Display* display, Window window, XVisualInfo* visual_info)
{
	GLXContext gl_context = glXCreateContext(display, visual_info, nullptr, GL_TRUE);
	if (!gl_context)
		return false;

	if (!glXMakeCurrent(display, window, gl_context))
		return false;

	if (!glXIsDirect(display, gl_context))
		Rml::Log::Message(Rml::Log::LT_INFO, "OpenGL context does not support direct rendering; performance is likely to be poor.");

	out_gl_context = gl_context;

	Window root_window;
	int x, y;
	unsigned int width, height;
	unsigned int border_width, depth;
	XGetGeometry(display, window, &root_window, &x, &y, &width, &height, &border_width, &depth);

	return true;
}

// Shutdown the OpenGL context.
static void DetachFromNative(GLXContext gl_context, Display* display)
{
	glXMakeCurrent(display, 0L, nullptr);
	glXDestroyContext(display, gl_context);
}

/**
    Global data used by this backend.

    Lifetime governed by the calls to Backend::Initialize() and Backend::Shutdown().
 */
struct BackendData {
	BackendData(Display* display) : system_interface(display) {}

	SystemInterface_X11 system_interface;
	RenderInterface_GL2 render_interface;

	Display* display = nullptr;
	Window window = 0;
	GLXContext gl_context = nullptr;

	bool running = true;
};
static Rml::UniquePtr<BackendData> data;

bool Backend::Initialize(const char* window_name, int width, int height, bool allow_resize)
{
	RMLUI_ASSERT(!data);

	Display* display = XOpenDisplay(0);
	if (!display)
		return false;

	int screen = XDefaultScreen(display);

	// Fetch an appropriate 32-bit visual interface.
	int attribute_list[] = {GLX_RGBA, GLX_DOUBLEBUFFER, GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8, GLX_DEPTH_SIZE, 24, GLX_STENCIL_SIZE, 8,
		0L};

	XVisualInfo* visual_info = glXChooseVisual(display, screen, attribute_list);
	if (!visual_info)
		return false;

	// Build up our window attributes.
	XSetWindowAttributes window_attributes;
	window_attributes.colormap = XCreateColormap(display, RootWindow(display, visual_info->screen), visual_info->visual, AllocNone);
	window_attributes.border_pixel = 0;
	window_attributes.event_mask = ExposureMask | KeyPressMask | ButtonPressMask | StructureNotifyMask;

	// Create the window.
	Window window = XCreateWindow(display, RootWindow(display, visual_info->screen), 0, 0, width, height, 0, visual_info->depth, InputOutput,
		visual_info->visual, CWBorderPixel | CWColormap | CWEventMask, &window_attributes);

	// Handle delete events in windowed mode.
	Atom delete_atom = XInternAtom(display, "WM_DELETE_WINDOW", True);
	XSetWMProtocols(display, window, &delete_atom, 1);

	// Capture the events we're interested in.
	XSelectInput(display, window,
		KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | LeaveWindowMask | PointerMotionMask | StructureNotifyMask);

	if (!allow_resize)
	{
		// Force the window to remain at the fixed size by asking the window manager nicely, it may choose to ignore us
		XSizeHints* win_size_hints = XAllocSizeHints(); // Allocate a size hint structure
		if (!win_size_hints)
		{
			Rml::Log::Message(Rml::Log::LT_ERROR, "XAllocSizeHints - out of memory");
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

			// Pass the size hints to the window manager.
			XSetWMNormalHints(display, window, win_size_hints);

			// Free the size buffer
			XFree(win_size_hints);
		}
	}

	// Set the window title and show the window.
	XSetStandardProperties(display, window, window_name, "", 0L, nullptr, 0, nullptr);
	XMapRaised(display, window);

	GLXContext gl_context = {};
	if (!AttachToNative(gl_context, display, window, visual_info))
		return false;

	data = Rml::MakeUnique<BackendData>(display);

	data->display = display;
	data->window = window;
	data->gl_context = gl_context;

	data->system_interface.SetWindow(window);
	data->render_interface.SetViewport(width, height);

	return true;
}

void Backend::Shutdown()
{
	RMLUI_ASSERT(data);

	DetachFromNative(data->gl_context, data->display);
	XCloseDisplay(data->display);

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

bool Backend::ProcessEvents(Rml::Context* context, KeyDownCallback key_down_callback, bool power_save)
{
	RMLUI_ASSERT(data && context);

	Display* display = data->display;
	bool result = data->running;
	data->running = true;

	if (power_save && XPending(display) == 0)
	{
		int display_fd = ConnectionNumber(display);
		fd_set fds{};
		FD_ZERO(&fds);
		FD_SET(display_fd, &fds);

		double timeout = Rml::Math::Min(context->GetNextUpdateDelay(), 10.0);
		struct timeval tv {};
		double seconds;
		tv.tv_usec = std::modf(timeout, &seconds) * 1000000.0;
		tv.tv_sec = seconds;

		int ready_fd_count;
		do
		{
			ready_fd_count = select(display_fd + 1, &fds, NULL, NULL, &tv);
			// We don't care about the return value as long as select didn't error out
			RMLUI_ASSERT(ready_fd_count >= 0);
		} while (XPending(display) == 0 && ready_fd_count != 0);
	}

	while (XPending(display) > 0)
	{
		XEvent ev;
		XNextEvent(display, &ev);

		switch (ev.type)
		{
		case ClientMessage:
		{
			// The only message we register for is WM_DELETE_WINDOW, so if we receive a client message then the window has been closed.
			char* event_type = XGetAtomName(display, ev.xclient.message_type);
			if (strcmp(event_type, "WM_PROTOCOLS") == 0)
				data->running = false;
			XFree(event_type);
			event_type = nullptr;
		}
		break;
		case ConfigureNotify:
		{
			int x = ev.xconfigure.width;
			int y = ev.xconfigure.height;

			context->SetDimensions({x, y});
			data->render_interface.SetViewport(x, y);
		}
		break;
		case KeyPress:
		{
			Rml::Input::KeyIdentifier key = RmlX11::ConvertKey(display, ev.xkey.keycode);
			const int key_modifier = RmlX11::GetKeyModifierState(ev.xkey.state);
			const float native_dp_ratio = 1.f;

			// See if we have any global shortcuts that take priority over the context.
			if (key_down_callback && !key_down_callback(context, key, key_modifier, native_dp_ratio, true))
				break;
			// Otherwise, hand the event over to the context by calling the input handler as normal.
			if (!RmlX11::HandleInputEvent(context, display, ev))
				break;
			// The key was not consumed by the context either, try keyboard shortcuts of lower priority.
			if (key_down_callback && !key_down_callback(context, key, key_modifier, native_dp_ratio, false))
				break;
		}
		break;
		case SelectionRequest:
		{
			data->system_interface.HandleSelectionRequest(ev);
		}
		break;
		default:
		{
			// Pass unhandled events to the platform layer's input handler.
			RmlX11::HandleInputEvent(context, display, ev);
		}
		break;
		}
	}

	return result;
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
	data->render_interface.Clear();
}

void Backend::PresentFrame()
{
	RMLUI_ASSERT(data);
	data->render_interface.EndFrame();

	// Flips the OpenGL buffers.
	glXSwapBuffers(data->display, data->window);
}
