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

#include "Shell.h"
#include <RmlUi/Core.h>
#include "ShellFileInterface.h"
#include "macosx/InputMacOSX.h"
#include <Carbon/Carbon.h>
#include <AGL/agl.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdarg.h>

static const EventTypeSpec INPUT_EVENTS[] = {
	{ kEventClassKeyboard, kEventRawKeyDown },
	{ kEventClassKeyboard, kEventRawKeyUp },
	{ kEventClassKeyboard, kEventRawKeyModifiersChanged },

	{ kEventClassMouse, kEventMouseDown },
	{ kEventClassMouse, kEventMouseUp },
	{ kEventClassMouse, kEventMouseMoved },
	{ kEventClassMouse, kEventMouseDragged },
	{ kEventClassMouse, kEventMouseWheelMoved },
};

static const EventTypeSpec WINDOW_EVENTS[] = {
	{ kEventClassWindow, kEventWindowClose },
	{ kEventClassWindow, kEventWindowBoundsChanged },
};

static Rml::Context* context = nullptr;
static ShellRenderInterfaceExtensions* shell_renderer;
static UniquePtr<ShellFileInterface> file_interface;

static WindowRef window;
static timeval start_time;
static Rml::String clipboard_text;
static int window_width = 0;
static int window_height = 0;

static void IdleTimerCallback(EventLoopTimerRef timer, EventLoopIdleTimerMessage inState, void* p);
static OSStatus EventHandler(EventHandlerCallRef next_handler, EventRef event, void* p);

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
	gettimeofday(&start_time, nullptr);

	InputMacOSX::Initialise();

	Rml::String root = FindSamplesRoot();

	file_interface = Rml::MakeUnique<ShellFileInterface>(root);
	Rml::SetFileInterface(file_interface.get());

	return true;
}

void Shell::Shutdown()
{
	file_interface.reset();
}

Rml::String Shell::FindSamplesRoot()
{
	Rml::String path = "../../Samples/";
	
	// Find the location of the executable.
	CFBundleRef bundle = CFBundleGetMainBundle();
	CFURLRef executable_url = CFBundleCopyExecutableURL(bundle);
	CFStringRef executable_posix_file_name = CFURLCopyFileSystemPath(executable_url, kCFURLPOSIXPathStyle);
	CFIndex max_length = CFStringGetMaximumSizeOfFileSystemRepresentation(executable_posix_file_name);
	char* executable_file_name = new char[max_length];
	if (!CFStringGetFileSystemRepresentation(executable_posix_file_name, executable_file_name, max_length))
		executable_file_name[0] = 0;

	Rml::String executable_path = Rml::String(executable_file_name);
	executable_path = executable_path.substr(0, executable_path.rfind("/") + 1);

	delete[] executable_file_name;
	CFRelease(executable_posix_file_name);
	CFRelease(executable_url);

	return (executable_path + "../../../" + path);
}

bool Shell::OpenWindow(const char* name, ShellRenderInterfaceExtensions *_shell_renderer, unsigned int width, unsigned int height, bool allow_resize)
{
	shell_renderer = _shell_renderer;
	Rect content_bounds = { 60, 20, 60 + height, 20 + width };
	window_width = width;
	window_height = height;

	OSStatus result = CreateNewWindow(kDocumentWindowClass,
									  (allow_resize ? (kWindowStandardDocumentAttributes | kWindowLiveResizeAttribute) :
									   kWindowCloseBoxAttribute) | kWindowStandardHandlerAttribute,
									  &content_bounds,
									  &window);
	if (result != noErr)
		return false;

	CFStringRef window_title = CFStringCreateWithCString(nullptr, name, kCFStringEncodingUTF8);
	if (result != noErr)
		return false;

	result = SetWindowTitleWithCFString(window, window_title);
	if (result != noErr)
	{
		CFRelease(window_title);
		return false;
	}

	CFRelease(window_title);

	ShowWindow(window);
    
	if(shell_renderer != nullptr) {
		shell_renderer->AttachToNative(window);
	}
    return true;
}

void Shell::CloseWindow()
{
	if(shell_renderer) {
		shell_renderer->DetachFromNative();
	}

	// Close the window.
	HideWindow(window);
	ReleaseWindow(window);
}

void Shell::EventLoop(ShellIdleFunction idle_function)
{
	OSStatus error;
	error = InstallApplicationEventHandler(NewEventHandlerUPP(InputMacOSX::EventHandler),
										   GetEventTypeCount(INPUT_EVENTS),
										   INPUT_EVENTS,
										   nullptr,
										   nullptr);
	if (error != noErr)
		DisplayError("Unable to install handler for input events, error: %d.", error);

	error = InstallWindowEventHandler(window,
									  NewEventHandlerUPP(EventHandler),
									  GetEventTypeCount(WINDOW_EVENTS),
									  WINDOW_EVENTS,
									  nullptr,
									  nullptr);
	if (error != noErr)
		DisplayError("Unable to install handler for window events, error: %d.", error);

	EventLoopTimerRef timer;
	error = InstallEventLoopIdleTimer(GetMainEventLoop(),							// inEventLoop
									  0,											// inFireDelay
									  5 * kEventDurationMillisecond,				// inInterval (200 Hz)
									  NewEventLoopIdleTimerUPP(IdleTimerCallback),	// inTimerProc
									  (void*) idle_function,						// inTimerData,
									  &timer										// outTimer
									  );
	if (error != noErr)
		DisplayError("Unable to install Carbon event loop timer, error: %d.", error);

	RunApplicationEventLoop();
}

void Shell::RequestExit()
{
	EventRef event;
	OSStatus result = CreateEvent(nullptr, // default allocator
								  kEventClassApplication, 
								  kEventAppQuit, 
								  0,						  
								  kEventAttributeNone, 
								  &event);

	if (result == noErr)
		PostEventToQueue(GetMainEventQueue(), event, kEventPriorityStandard);
}

void Shell::DisplayError(const char* fmt, ...)
{
	const int buffer_size = 1024;
	char buffer[buffer_size];
	va_list argument_list;

	// Print the message to the buffer.
	va_start(argument_list, fmt);
	int len = vsnprintf(buffer, buffer_size - 2, fmt, argument_list);	
	if (len < 0 || len > buffer_size - 2)
	{
		len = buffer_size - 2;
	}	
	buffer[len] = '\n';
	buffer[len + 1] = '\0';
	va_end(argument_list);

	fprintf(stderr, "%s", buffer);
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

double Shell::GetElapsedTime() 
{
	struct timeval now;

	gettimeofday(&now, nullptr);

	double sec = now.tv_sec - start_time.tv_sec;
	double usec = now.tv_usec;
	double result = sec + (usec / 1000000.0);

	return result;
}

void Shell::SetMouseCursor(const Rml::String& cursor_name)
{
	// Not implemented
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

static void IdleTimerCallback(EventLoopTimerRef timer, EventLoopIdleTimerMessage inState, void* p)
{
	Shell::ShellIdleFunction function = (Shell::ShellIdleFunction) p;
	function();
}

static OSStatus EventHandler(EventHandlerCallRef next_handler, EventRef event, void* p)
{
	switch (GetEventClass(event))
	{
		case kEventClassWindow:
		{
			switch (GetEventKind(event))
			{
				case kEventWindowClose:
					Shell::RequestExit();
					break;
				case kEventWindowBoundsChanged:
					// Window resized, update the rmlui context
					UInt32 attributes;
					GetEventParameter(event, kEventParamAttributes, typeUInt32, nullptr, sizeof(UInt32), nullptr, &attributes);

					if(attributes & kWindowBoundsChangeSizeChanged)
					{
						Rect bounds;
						GetEventParameter(event, kEventParamCurrentBounds, typeQDRectangle, nullptr, sizeof(Rect), nullptr, &bounds);

						UInt32 width = bounds.right - bounds.left;
						UInt32 height = bounds.bottom - bounds.top;

						UpdateWindowDimensions((int)width, (int)height);
					}
					break;
			}
		}
		break;
	}

//	InputMacOSX::ProcessCarbonEvent(event);
	return CallNextEventHandler(next_handler, event);
}
