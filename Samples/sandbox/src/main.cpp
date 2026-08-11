#include "SandboxFileInterface.h"
#include "SandboxLogger.h"
#include "SandboxSystemInterface.h"
#include "SandboxWindow.h"
#include <RmlUi/Core/Core.h>
#include <RmlUi/Debugger.h>
#include <PlatformExtensions.h>
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

	const Rml::String samples_root = PlatformExtensions::FindSamplesRoot();
	if (samples_root.empty())
		return -1;

	// Constructs the system and render interfaces, creates a window, and attaches the renderer.
	if (!Backend::Initialize("RmlUi Sandbox", width, height, true))
		return -1;

	SandboxLogger logger;

	// Install the custom interfaces before initializing RmlUi.
	SandboxFileInterface sandbox_file_interface{samples_root};
	SandboxSystemInterface sandbox_system_interface{Backend::GetSystemInterface(), &logger};
	Rml::SetFileInterface(&sandbox_file_interface);
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
		return -1;
	}

	Rml::Debugger::Initialise(context);
	context->GetRootElement()->GetElementById("rmlui-debug-log-beacon")->SetProperty("display", "none");

	Shell::LoadFonts();

	SandboxWindow sandbox_window{&logger, &sandbox_file_interface};
	if (!sandbox_window.Initialize(context))
	{
		Rml::Shutdown();
		Backend::Shutdown();
		return -1;
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

	return 0;
}
