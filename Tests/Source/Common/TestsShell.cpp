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

#include "TestsShell.h"
#include "TestsInterface.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/EventListener.h>
#include <RmlUi/Debugger.h>
#include <RmlUi_Backend.h>
#include <Shell.h>
#include <cstdlib>
#include <doctest.h>

namespace {
const Rml::Vector2i window_size(1500, 800);

// Set the following environment variable to render to the shell window with the current backend, instead of the to
// dummy renderer. Useful for viewing the result while building RML.
const bool use_backend_shell = [] {
	if (const char* env_variable = std::getenv("RMLUI_TESTS_USE_SHELL"))
		return Rml::FromString<bool>(env_variable);
	return false;
}();

bool shell_initialized = false;
bool debugger_allowed = true;
int num_documents_begin = 0;
Rml::Context* shell_context = nullptr;

TestsSystemInterface tests_system_interface;

// The tests renderer only collects statistics, does not render anything.
Rml::UniquePtr<TestsRenderInterface> tests_render_interface;

class TestsShellEventListener : public Rml::EventListener {
public:
	void ProcessEvent(Rml::Event& event) override
	{
		if (event.GetId() == Rml::EventId::Keydown)
		{
			Rml::Input::KeyIdentifier key_identifier = (Rml::Input::KeyIdentifier)event.GetParameter<int>("key_identifier", 0);

			// Will escape the current render loop
			if (key_identifier == Rml::Input::KI_ESCAPE || key_identifier == Rml::Input::KI_RETURN || key_identifier == Rml::Input::KI_NUMPADENTER)
				Backend::RequestExit();
		}
	}

	void OnDetach(Rml::Element* /*element*/) override { delete this; }
};

} // namespace

static void InitializeShell(bool allow_debugger, Rml::RenderInterface* override_render_interface)
{
	if (shell_initialized)
		return;

	// Initialize shell and create context.
	shell_initialized = true;
	debugger_allowed = allow_debugger;
	REQUIRE(Shell::Initialize());

	if (use_backend_shell)
	{
		// Initialize the backend and launch a window.
		REQUIRE(Backend::Initialize("RmlUi Tests", window_size.x, window_size.y, true));

		// Use our custom tests system interface.
		Rml::SetSystemInterface(&tests_system_interface);
		// However, use the backend's render interface.
		Rml::SetRenderInterface(Backend::GetRenderInterface());

		REQUIRE(Rml::Initialise());
		shell_context = Rml::CreateContext("main", window_size);
		Shell::LoadFonts();

		if (allow_debugger)
		{
			Rml::Debugger::Initialise(shell_context);
			num_documents_begin = shell_context->GetNumDocuments();
		}

		shell_context->GetRootElement()->AddEventListener(Rml::EventId::Keydown, new TestsShellEventListener, true);
	}

	else
	{
		// Set our custom system and render interfaces.
		Rml::SetSystemInterface(&tests_system_interface);
		Rml::SetRenderInterface(override_render_interface ? override_render_interface : TestsShell::GetTestsRenderInterface());

		REQUIRE(Rml::Initialise());
		shell_context = Rml::CreateContext("main", window_size);
		Shell::LoadFonts();
	}
}

Rml::Context* TestsShell::GetContext(bool allow_debugger, Rml::RenderInterface* override_render_interface)
{
	InitializeShell(allow_debugger, override_render_interface);
	return shell_context;
}

void TestsShell::BeginFrame()
{
	if (use_backend_shell)
		Backend::BeginFrame();
}

void TestsShell::PresentFrame()
{
	if (use_backend_shell)
		Backend::PresentFrame();
}

void TestsShell::RenderLoop()
{
	REQUIRE(shell_context);

	if (use_backend_shell)
	{
		bool running = true;
		while (running)
		{
			running = Backend::ProcessEvents(shell_context, &Shell::ProcessKeyDownShortcuts);
			shell_context->Update();
			BeginFrame();
			shell_context->Render();
			PresentFrame();
		}
	}
	else
	{
		shell_context->Update();
		shell_context->Render();
	}
}

void TestsShell::ShutdownShell(bool reset_tests_render_interface)
{
	if (!shell_initialized)
		return;

	if (debugger_allowed)
	{
		RMLUI_ASSERTMSG(shell_context->GetNumDocuments() == num_documents_begin, "Make sure all previously opened documents have been closed.");
		(void)num_documents_begin;
	}

	Rml::Shutdown();

	tests_system_interface.SetNumExpectedWarnings(0);

	if (use_backend_shell)
		Backend::Shutdown();
	else if (reset_tests_render_interface)
		tests_render_interface.reset();

	Shell::Shutdown();

	shell_context = nullptr;
	shell_initialized = false;
}

void TestsShell::SetNumExpectedWarnings(int num_warnings)
{
	tests_system_interface.SetNumExpectedWarnings(num_warnings);
}

Rml::String TestsShell::GetRenderStats()
{
	Rml::String result;

	if (!use_backend_shell)
	{
		shell_context->Update();
		tests_render_interface->ResetCounters();
		shell_context->Render();
		auto& counters = tests_render_interface->GetCounters();

		result = Rml::CreateString("Context::Render() stats:\n"
								   "  Compile geometry: %zu\n"
								   "  Render geometry: %zu\n"
								   "  Release geometry: %zu\n"
								   "  Texture load: %zu\n"
								   "  Texture generate: %zu\n"
								   "  Texture release: %zu\n"
								   "  Scissor enable: %zu\n"
								   "  Scissor set: %zu\n"
								   "  Clip mask enable: %zu\n"
								   "  Clip mask render: %zu\n"
								   "  Transform set: %zu",
			counters.compile_geometry, counters.render_geometry, counters.release_geometry, counters.load_texture, counters.generate_texture,
			counters.release_texture, counters.enable_scissor, counters.set_scissor, counters.enable_clip_mask, counters.render_to_clip_mask,
			counters.set_transform);
	}

	return result;
}

TestsRenderInterface* TestsShell::GetTestsRenderInterface()
{
	if (use_backend_shell)
		return nullptr;

	if (!tests_render_interface)
		tests_render_interface = Rml::MakeUnique<TestsRenderInterface>();

	return tests_render_interface.get();
}

void TestsShell::ResetTestsRenderInterface()
{
	tests_render_interface.reset();
}

TestsSystemInterface* TestsShell::GetTestsSystemInterface()
{
	return &tests_system_interface;
}
