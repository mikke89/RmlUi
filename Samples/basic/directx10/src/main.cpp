#include <Rocket/Core.h>
#include <Rocket/Debugger.h>
#include <Input.h>
#include <Shell.h>
#include "RenderInterfaceDirectX10.h"

// Because we're a windows app
#include <windows.h>

// For _T unicode/mbcs macro
#include <tchar.h>

static Rocket::Core::Context* context = NULL;

ShellRenderInterfaceExtensions *shell_renderer;

void GameLoop()
{
	context->Update();

	shell_renderer->PrepareRenderBuffer();
	context->Render();
	shell_renderer->PresentRenderBuffer();
}

int APIENTRY WinMain(HINSTANCE ROCKET_UNUSED_PARAMETER(instance_handle), HINSTANCE ROCKET_UNUSED_PARAMETER(previous_instance_handle), char* ROCKET_UNUSED_PARAMETER(command_line), int ROCKET_UNUSED_PARAMETER(command_show))
{
	ROCKET_UNUSED(instance_handle);
	ROCKET_UNUSED(previous_instance_handle);
	ROCKET_UNUSED(command_line);
	ROCKET_UNUSED(command_show);

	int window_width = 1024;
	int window_height = 768;

	RenderInterfaceDirectX10 directx_renderer;
	shell_renderer = &directx_renderer;

	// Generic OS initialisation, creates a window and does not attach OpenGL.
	if (!Shell::Initialise("../Samples/basic/directx/") ||
		!Shell::OpenWindow("DirectX 10 Sample", shell_renderer, window_width, window_height, true))
	{
		Shell::Shutdown();
		return -1;
	}

	// Install our DirectX render interface into Rocket.
	Rocket::Core::SetRenderInterface(&directx_renderer);

	ShellSystemInterface system_interface;
	Rocket::Core::SetSystemInterface(&system_interface);

	Rocket::Core::Initialise();

	// Create the main Rocket context and set it on the shell's input layer.
	context = Rocket::Core::CreateContext("main", Rocket::Core::Vector2i(window_width, window_height));
	if (context == NULL)
	{
		Rocket::Core::Shutdown();
		Shell::Shutdown();
		return -1;
	}

	Rocket::Debugger::Initialise(context);
	Input::SetContext(context);
	shell_renderer->SetContext(context);

	Shell::LoadFonts("../../assets/");

	// Load and show the tutorial document.
	Rocket::Core::ElementDocument* document = context->LoadDocument("../../assets/demo.rml");
	if (document != NULL)
	{
		document->Show();
		document->RemoveReference();
	}

	Shell::EventLoop(GameLoop);

	// Shutdown Rocket.
	context->RemoveReference();
	Rocket::Core::Shutdown();

	Shell::CloseWindow();
	Shell::Shutdown();

	return 0;
}
