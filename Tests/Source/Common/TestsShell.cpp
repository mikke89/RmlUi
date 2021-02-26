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

#include "TestsShell.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/EventListener.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Debugger.h>
#include <Shell.h>
#include <Input.h>
#include <ShellRenderInterfaceOpenGL.h>
#include "TestsInterface.h"

#include <doctest.h>

// Uncomment the following to render to the shell window instead of the dummy renderer.
//#define RMLUI_TESTS_USE_SHELL

// Uncomment the following line to enable rendering the context in a loop. Requires the shell backend. Useful for viewing the result while building RML.
//#define RMLUI_TESTS_ENABLE_RENDER_LOOP


namespace {
	const Rml::Vector2i window_size(1500, 800);

	bool shell_initialized = false;
	int num_documents_begin = 0;
	Rml::Context* shell_context = nullptr;

	TestsSystemInterface tests_system_interface;

#ifdef RMLUI_TESTS_USE_SHELL
	ShellRenderInterfaceOpenGL shell_render_interface;

	class TestsShellEventListener : public Rml::EventListener {
	public:
		void ProcessEvent(Rml::Event& event) override
		{
			if (event.GetId() == Rml::EventId::Keydown)
			{
				Rml::Input::KeyIdentifier key_identifier = (Rml::Input::KeyIdentifier)event.GetParameter< int >("key_identifier", 0);

				// Will escape the current render loop
				if (key_identifier == Rml::Input::KI_ESCAPE || key_identifier == Rml::Input::KI_RETURN || key_identifier == Rml::Input::KI_NUMPADENTER)
					Shell::RequestExit();
			}
		}
	} shell_event_listener;
#else
	// The tests renderer only collects statistics, does not render anything.
	TestsRenderInterface shell_render_interface;
#endif
}

static void InitializeShell()
{
	// Initialize shell and create context.
	if (!shell_initialized)
	{
		shell_initialized = true;

		Rml::SetSystemInterface(&tests_system_interface);
		Rml::SetRenderInterface(&shell_render_interface);

		REQUIRE(Rml::Initialise());
		shell_context = Rml::CreateContext("main", window_size);

		REQUIRE(Shell::Initialise());
		Shell::LoadFonts("assets/");

#ifdef RMLUI_TESTS_USE_SHELL
		// Also, create the window.

		Rml::Debugger::Initialise(shell_context);
		num_documents_begin = shell_context->GetNumDocuments();

		REQUIRE(Shell::OpenWindow("RmlUi Tests", &shell_render_interface, window_size.x, window_size.y, true));

		shell_render_interface.SetViewport(window_size.x, window_size.y);

		::Input::SetContext(shell_context);
		Shell::SetContext(shell_context);

		shell_context->GetRootElement()->AddEventListener(Rml::EventId::Keydown, &shell_event_listener, true);
#endif
	}
}


Rml::Context* TestsShell::GetContext()
{
	InitializeShell();
	return shell_context;
}

void TestsShell::PrepareRenderBuffer()
{
#ifdef RMLUI_TESTS_USE_SHELL
	shell_render_interface.PrepareRenderBuffer();
#endif
}

void TestsShell::PresentRenderBuffer()
{
#ifdef RMLUI_TESTS_USE_SHELL
	shell_render_interface.PresentRenderBuffer();
#endif
}

void TestsShell::RenderLoop()
{
#if defined(RMLUI_TESTS_USE_SHELL) && defined(RMLUI_TESTS_ENABLE_RENDER_LOOP)
	REQUIRE(shell_context);
	
	Shell::EventLoop([]() {
		shell_context->Update();
		PrepareRenderBuffer();
		shell_context->Render();
		PresentRenderBuffer();
	});
#endif
}

void TestsShell::ShutdownShell()
{
	if (shell_initialized)
	{
		RMLUI_ASSERTMSG(shell_context->GetNumDocuments() == num_documents_begin, "Make sure all previously opened documents have been closed.");
		(void)num_documents_begin;

		tests_system_interface.SetNumExpectedWarnings(0);

		Rml::Shutdown();

#ifdef RMLUI_TESTS_USE_SHELL
		Shell::CloseWindow();
		Shell::Shutdown();
		Shell::SetContext(nullptr);
#endif

		shell_context = nullptr;
		shell_initialized = false;
	}
}

void TestsShell::SetNumExpectedWarnings(int num_warnings)
{
	tests_system_interface.SetNumExpectedWarnings(num_warnings);
}

Rml::String TestsShell::GetRenderStats()
{
	Rml::String result;

#if !defined(RMLUI_TESTS_USE_SHELL)

	shell_context->Update();
	shell_render_interface.ResetCounters();
	shell_context->Render();
	auto& counters = shell_render_interface.GetCounters();

	result = Rml::CreateString(256,
		"Context::Render() stats:\n"
		"  Render calls: %zu\n"
		"  Scissor enable: %zu\n"
		"  Scissor set: %zu\n"
		"  Texture load: %zu\n"
		"  Texture generate: %zu\n"
		"  Texture release: %zu\n"
		"  Transform set: %zu",
		counters.render_calls,
		counters.enable_scissor,
		counters.set_scissor,
		counters.load_texture,
		counters.generate_texture,
		counters.release_texture,
		counters.set_transform
	);

#endif

	return result;
}
