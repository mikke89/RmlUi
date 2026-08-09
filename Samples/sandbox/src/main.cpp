#include "DocumentSource.h"
#include "SandboxLogger.h"
#include "SandboxSystemInterface.h"
#include "SandboxWindow.h"
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
	if (!Backend::Initialize("RmlUi Sandbox", width, height, true))
	{
		Shell::Shutdown();
		return -1;
	}

	SandboxLogger logger;

	// Install the custom interfaces constructed by the backend before initializing RmlUi.
	SandboxSystemInterface sandbox_system_interface{Backend::GetSystemInterface(), &logger};
	Rml::SetSystemInterface(&sandbox_system_interface);
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

	SandboxWindow sandbox_window{&logger};
	if (!sandbox_window.Initialize("Sandbox", context))
	{
		Rml::Shutdown();
		Backend::Shutdown();
		Shell::Shutdown();
		return -1;
	}

	{
		// Rml::ElementDocument* menu = context->LoadDocument(R"(C:/Projects/Gridlock/run/gui/menu_options.rml)");
		// menu->Show();

		context->LoadDocumentFromMemory(external_document_source)->Show();
	}

	bool running = true;
	while (running)
	{
		running = Backend::ProcessEvents(context, &Shell::ProcessKeyDownShortcuts, true);

		sandbox_window.Update();
		context->Update();

		Backend::BeginFrame();
		context->Render();
		Backend::PresentFrame();
	}

	sandbox_window.Shutdown();

	Rml::Shutdown();

	Backend::Shutdown();
	Shell::Shutdown();

	return 0;
}
