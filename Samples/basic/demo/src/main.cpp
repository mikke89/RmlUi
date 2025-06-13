/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2018 Michael R. P. Ragazzon
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

#include "DemoEventListener.h"
#include "DemoWindow.h"
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Factory.h>
#include <RmlUi/Debugger.h>
#include <RmlUi_Backend.h>
#include <Shell.h>

#if defined RMLUI_PLATFORM_WIN32
	#include <RmlUi_Include_Windows.h>
int APIENTRY WinMain(HINSTANCE /*instance_handle*/, HINSTANCE /*previous_instance_handle*/, char* /*command_line*/, int /*command_show*/)
#else
int main(int /*argc*/, char** /*argv*/)
#endif
{
	const int width = 1600;
	const int height = 890;

	// Initializes the shell which provides common functionality used by the included samples.
	if (!Shell::Initialize())
		return -1;

	// Constructs the system and render interfaces, creates a window, and attaches the renderer.
	if (!Backend::Initialize("Demo Sample", width, height, true))
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
	Rml::Context* context = Rml::CreateContext("main", Rml::Vector2i(width, height));
	if (!context)
	{
		Rml::Shutdown();
		Backend::Shutdown();
		Shell::Shutdown();
		return -1;
	}

	Rml::Debugger::Initialise(context);

	Shell::LoadFonts();

	DemoWindow demo_window;

	DemoEventListenerInstancer event_listener_instancer{&demo_window};
	Rml::Factory::RegisterEventListenerInstancer(&event_listener_instancer);

	if (!demo_window.Initialize("Demo sample", context))
	{
		Rml::Shutdown();
		Backend::Shutdown();
		Shell::Shutdown();
		return -1;
	}

	demo_window.GetDocument()->AddEventListener(Rml::EventId::Keydown, &demo_window);
	demo_window.GetDocument()->AddEventListener(Rml::EventId::Keyup, &demo_window);
	demo_window.GetDocument()->AddEventListener(Rml::EventId::Animationend, &demo_window);

	bool running = true;
	while (running)
	{
		demo_window.Update();

		running = Backend::ProcessEvents(context, &Shell::ProcessKeyDownShortcuts, true);
		context->Update();

		Backend::BeginFrame();
		context->Render();
		Backend::PresentFrame();
	}

	demo_window.Shutdown();

	Rml::Shutdown();

	Backend::Shutdown();
	Shell::Shutdown();

	return 0;
}
