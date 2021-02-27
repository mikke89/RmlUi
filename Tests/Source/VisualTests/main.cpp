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

#include "TestConfig.h"
#include "TestViewer.h"
#include "TestNavigator.h"
#include "CaptureScreen.h"
#include "TestSuite.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Debugger.h>
#include <Shell.h>
#include <Input.h>
#include <ShellRenderInterfaceOpenGL.h>


Rml::Context* context = nullptr;
ShellRenderInterfaceOpenGL* shell_renderer = nullptr;
TestNavigator* g_navigator = nullptr;

void GameLoop()
{
	context->Update();

	shell_renderer->PrepareRenderBuffer();
	context->Render();
	shell_renderer->PresentRenderBuffer();

	if (g_navigator)
	{
		g_navigator->Update();
	}
}


#if defined RMLUI_PLATFORM_WIN32
#include <windows.h>
int APIENTRY WinMain(HINSTANCE RMLUI_UNUSED_PARAMETER(instance_handle), HINSTANCE RMLUI_UNUSED_PARAMETER(previous_instance_handle), char* RMLUI_UNUSED_PARAMETER(command_line), int RMLUI_UNUSED_PARAMETER(command_show))
#else
int main(int RMLUI_UNUSED_PARAMETER(argc), char** RMLUI_UNUSED_PARAMETER(argv))
#endif
{
#ifdef RMLUI_PLATFORM_WIN32
	RMLUI_UNUSED(instance_handle);
	RMLUI_UNUSED(previous_instance_handle);
	RMLUI_UNUSED(command_line);
	RMLUI_UNUSED(command_show);
#else
	RMLUI_UNUSED(argc);
	RMLUI_UNUSED(argv);
#endif

	int window_width = 1500;
	int window_height = 800;

	ShellRenderInterfaceOpenGL opengl_renderer;
	shell_renderer = &opengl_renderer;

	// Generic OS initialisation, creates a window and attaches OpenGL.
	if (!Shell::Initialise() ||
		!Shell::OpenWindow("Visual tests", shell_renderer, window_width, window_height, true))
	{
		Shell::Shutdown();
		return -1;
	}

	// RmlUi initialisation.
	Rml::SetRenderInterface(&opengl_renderer);
	shell_renderer->SetViewport(window_width, window_height);

	ShellSystemInterface system_interface;
	Rml::SetSystemInterface(&system_interface);

	Rml::Initialise();

	// Create the main RmlUi context and set it on the shell's input layer.
	context = Rml::CreateContext("main", Rml::Vector2i(window_width, window_height));
	if (context == nullptr)
	{
		Rml::Shutdown();
		Shell::Shutdown();
		return -1;
	}

	Rml::Debugger::Initialise(context);
	Input::SetContext(context);
	Shell::SetContext(context);

	Shell::LoadFonts("assets/");

	{
		const Rml::StringList directories = GetTestInputDirectories();

		TestSuiteList test_suites;

		for (const Rml::String& directory : directories)
		{
			const Rml::StringList files = Shell::ListFiles(directory, "rml");

			if (files.empty())
			{
				Rml::Log::Message(Rml::Log::LT_WARNING, "Could not find any *.rml files in directory '%s'. Ignoring.", directory.c_str());
			}
			else
			{
				test_suites.emplace_back(directory, std::move(files));
			}
		}

		RMLUI_ASSERTMSG(!test_suites.empty(), "RML test files directory not found or empty.");

		TestViewer viewer(context);

		TestNavigator navigator(shell_renderer, context, &viewer, std::move(test_suites));
		g_navigator = &navigator;

		Shell::EventLoop(GameLoop);

		g_navigator = nullptr;
	}

	Rml::Shutdown();

	Shell::CloseWindow();
	Shell::Shutdown();

	return 0;
}
