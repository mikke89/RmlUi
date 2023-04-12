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

#include "CaptureScreen.h"
#include "TestConfig.h"
#include "TestNavigator.h"
#include "TestSuite.h"
#include "TestViewer.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Debugger.h>
#include <PlatformExtensions.h>
#include <RmlUi_Backend.h>
#include <Shell.h>
#include <stdio.h>

#if defined RMLUI_PLATFORM_WIN32
	#include <RmlUi_Include_Windows.h>
int APIENTRY WinMain(HINSTANCE /*instance_handle*/, HINSTANCE /*previous_instance_handle*/, char* win_command_line, int /*command_show*/)
#else
int main(int argc, char** argv)
#endif
{
	const char* command_line = nullptr;

#ifdef RMLUI_PLATFORM_WIN32
	command_line = win_command_line;
#else
	if (argc > 1)
		command_line = argv[1];
#endif
	int load_suite_index = -1;
	int load_case_index = -1;

	// Parse command line as <case_index> *or* <suite_index>:<case_index>.
	if (command_line)
	{
		int first_argument = -1;
		int second_argument = -1;
		const int num_arguments = sscanf(command_line, "%d:%d", &first_argument, &second_argument);

		switch (num_arguments)
		{
		case 1: load_case_index = first_argument - 1; break;
		case 2:
			load_suite_index = first_argument - 1;
			load_case_index = second_argument - 1;
			break;
		}
	}

	int window_width = 1500;
	int window_height = 800;

	// Initializes the shell which provides common functionality used by the included samples.
	if (!Shell::Initialize())
		return -1;

	// Constructs the system and render interfaces, creates a window, and attaches the renderer.
	if (!Backend::Initialize("Visual tests", window_width, window_height, true))
	{
		Shell::Shutdown();
		return -1;
	}

	// Install the custom interfaces constructed by the backend before initializing RmlUi.
	Rml::SetSystemInterface(Backend::GetSystemInterface());
	Rml::SetRenderInterface(Backend::GetRenderInterface());

	// RmlUi initialisation.
	Rml::Initialise();

	// Create the main RmlUi context.
	Rml::Context* context = Rml::CreateContext("main", Rml::Vector2i(window_width, window_height));
	if (!context)
	{
		Rml::Shutdown();
		Shell::Shutdown();
		return -1;
	}

	Rml::Debugger::Initialise(context);
	Shell::LoadFonts();

	{
		const Rml::StringList directories = GetTestInputDirectories();

		TestSuiteList test_suites;

		for (const Rml::String& directory : directories)
		{
			const Rml::StringList files = PlatformExtensions::ListFiles(directory, "rml");

			if (files.empty())
				Rml::Log::Message(Rml::Log::LT_WARNING, "Could not find any *.rml files in directory '%s'. Ignoring.", directory.c_str());
			else
				test_suites.emplace_back(directory, std::move(files));
		}

		RMLUI_ASSERTMSG(!test_suites.empty(), "RML test files directory not found or empty.");

		TestViewer viewer(context);

		TestNavigator navigator(context->GetRenderInterface(), context, &viewer, std::move(test_suites), load_suite_index, load_case_index);

		bool running = true;
		while (running)
		{
			running = Backend::ProcessEvents(context, &Shell::ProcessKeyDownShortcuts);

			context->Update();

			Backend::BeginFrame();
			context->Render();
			navigator.Render();
			Backend::PresentFrame();

			navigator.Update();
		}
	}

	Rml::Shutdown();
	Shell::Shutdown();
	Backend::Shutdown();

	return 0;
}
