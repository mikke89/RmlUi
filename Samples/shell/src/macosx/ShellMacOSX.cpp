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

#include "Shell.h"
#include <Rocket/Core.h>
#include "ShellFileInterface.h"
#include "macosx/InputMacOSX.h"
#include <Carbon/Carbon.h>
#include <AGL/agl.h>
#include <sys/time.h>
#include <stdio.h>

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

static WindowRef window;
static timeval start_time;

ShellFileInterface* file_interface = NULL;

static void IdleTimerCallback(EventLoopTimerRef timer, EventLoopIdleTimerMessage inState, void* p);
static OSStatus EventHandler(EventHandlerCallRef next_handler, EventRef event, void* p);

bool Shell::Initialise(const Rocket::Core::String& path)
{
	gettimeofday(&start_time, NULL);

	InputMacOSX::Initialise();

	// Find the location of the executable.
	CFBundleRef bundle = CFBundleGetMainBundle();
	CFURLRef executable_url = CFBundleCopyExecutableURL(bundle);
	CFStringRef executable_posix_file_name = CFURLCopyFileSystemPath(executable_url, kCFURLPOSIXPathStyle);
	CFIndex max_length = CFStringGetMaximumSizeOfFileSystemRepresentation(executable_posix_file_name);
	char* executable_file_name = new char[max_length];
	if (!CFStringGetFileSystemRepresentation(executable_posix_file_name, executable_file_name, max_length))
		executable_file_name[0] = 0;

	executable_path = Rocket::Core::String(executable_file_name);
	executable_path = executable_path.Substring(0, executable_path.RFind("/") + 1);

	delete[] executable_file_name;
	CFRelease(executable_posix_file_name);
	CFRelease(executable_url);

	file_interface = new ShellFileInterface(executable_path + "../../../" + path);
	Rocket::Core::SetFileInterface(file_interface);

	return true;
}

void Shell::Shutdown()
{
	delete file_interface;
	file_interface = NULL;
}

static ShellRenderInterfaceExtensions *shell_renderer;

bool Shell::OpenWindow(const char* name, ShellRenderInterfaceExtensions *_shell_renderer, unsigned int width, unsigned int height, bool allow_resize)
{
	shell_renderer = _shell_renderer;
	Rect content_bounds = { 60, 20, 60 + height, 20 + width };

	OSStatus result = CreateNewWindow(kDocumentWindowClass,
									  (allow_resize ? (kWindowStandardDocumentAttributes | kWindowLiveResizeAttribute) :
									   kWindowCloseBoxAttribute) | kWindowStandardHandlerAttribute,
									  &content_bounds,
									  &window);
	if (result != noErr)
		return false;

	CFStringRef window_title = CFStringCreateWithCString(NULL, name, kCFStringEncodingUTF8);
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
    
	if(shell_renderer != NULL) {
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
										   NULL,
										   NULL);
	if (error != noErr)
		DisplayError("Unable to install handler for input events, error: %d.", error);

	error = InstallWindowEventHandler(window,
									  NewEventHandlerUPP(EventHandler),
									  GetEventTypeCount(WINDOW_EVENTS),
									  WINDOW_EVENTS,
									  NULL,
									  NULL);
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
	OSStatus result = CreateEvent(NULL, // default allocator
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

float Shell::GetElapsedTime() 
{
	struct timeval now;

	gettimeofday(&now, NULL);

	double sec = now.tv_sec - start_time.tv_sec;
	double usec = now.tv_usec;
	double result = sec + (usec / 1000000.0);

	return (float) result;
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
					// Window resized, update the rocket context
					UInt32 attributes;
					GetEventParameter(event, kEventParamAttributes, typeUInt32, NULL, sizeof(UInt32), NULL, &attributes);

					if(attributes & kWindowBoundsChangeSizeChanged)
					{
						Rect bounds;
						GetEventParameter(event, kEventParamCurrentBounds, typeQDRectangle, NULL, sizeof(Rect), NULL, &bounds);

						UInt32 width = bounds.right - bounds.left;
						UInt32 height = bounds.bottom - bounds.top;

						shell_renderer->SetViewport(width, height);
					}
					break;
			}
		}
		break;
	}

//	InputMacOSX::ProcessCarbonEvent(event);
	return CallNextEventHandler(next_handler, event);
}
