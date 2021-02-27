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

#ifndef RMLUI_SHELL_H
#define RMLUI_SHELL_H

#include <RmlUi/Core/Types.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/SystemInterface.h>
#include "ShellRenderInterfaceExtensions.h"

/**
	Shell functions for creating windows, attaching OpenGL and handling input in a platform independent way.
	@author Lloyd Weehuizen
 */

class Shell
{
public:
	/// Initialise the shell.
	static bool Initialise();
	/// Shutdown the shell.
	static void Shutdown();

	/// Finds the Samples root directory.
	static Rml::String FindSamplesRoot();
	
	/// Loads the default fonts from the given path.
	static void LoadFonts(const char* directory);

	/// List files in the given directory. An initial forward slash '/' makes it relative to the samples root.
	static Rml::StringList ListFiles(const Rml::String& in_directory, const Rml::String& extension = Rml::String());
	/// List subdirectories in the given directory. An initial forward slash '/' makes it relative to the samples root.
	static Rml::StringList ListDirectories(const Rml::String& in_directory);

	/// Open a platform specific window, optionally initialising an OpenGL context on it.
	/// @param[in] title Title of the window.
	/// @param[in] srie Provides the interface for attaching a renderer to the window and performing related bits of interface.
	static bool OpenWindow(const char* title, ShellRenderInterfaceExtensions *_shell_renderer, unsigned int width, unsigned int height, bool allow_resize);
	/// Close the active window.
	static void CloseWindow();

	/// Returns a platform-dependent handle to the window.
	static void* GetWindowHandle();

	/// Run the event loop, calling the idle function every frame.
	using ShellIdleFunction = void(*)();
	static void EventLoop(ShellIdleFunction idle_function);
	static void RequestExit();

	/// Display an error message.
	static void DisplayError(const char* fmt, ...);
	/// Log a message to the debugger.
	static void Log(const char* fmt, ...);

	/// Get the number of seconds that have passed since shell startup.
	static double GetElapsedTime();

	/// Set mouse cursor.
	static void SetMouseCursor(const Rml::String& cursor_name);

	/// Set clipboard text.
	static void SetClipboardText(const Rml::String& text);

	/// Get clipboard text.
	static void GetClipboardText(Rml::String& text);
	
	/// Sets the RmlUi context to send window resized events to.
	static void SetContext(Rml::Context* context);

	/// Return the dp-ratio of the system.
	static float GetDensityIndependentPixelRatio();
};

#include "ShellSystemInterface.h"

#endif
