#include <RmlUi/Core.h>
#include <RmlUi/Debugger.h>
#include <Input.h>
#include <Shell.h>
#include "RenderInterfaceDirectX10.h"

// Because we're a windows app
#include <windows.h>

// For _T unicode/mbcs macro
#include <tchar.h>

static Rml::Core::Context* context = nullptr;

ShellRenderInterfaceExtensions *shell_renderer;

void GameLoop()
{
	context->Update();

	shell_renderer->PrepareRenderBuffer();
	context->Render();
	shell_renderer->PresentRenderBuffer();
}

int APIENTRY WinMain(HINSTANCE RMLUI_UNUSED_PARAMETER(instance_handle), HINSTANCE RMLUI_UNUSED_PARAMETER(previous_instance_handle), char* RMLUI_UNUSED_PARAMETER(command_line), int RMLUI_UNUSED_PARAMETER(command_show))
{
	RMLUI_UNUSED(instance_handle);
	RMLUI_UNUSED(previous_instance_handle);
	RMLUI_UNUSED(command_line);
	RMLUI_UNUSED(command_show);

	int window_width = 1024;
	int window_height = 768;

	RenderInterfaceDirectX10 directx_renderer;
	shell_renderer = &directx_renderer;

	// Generic OS initialisation, creates a window and does not attach OpenGL.
	if (!Shell::Initialise() ||
		!Shell::OpenWindow("DirectX 10 Sample", shell_renderer, window_width, window_height, true))
	{
		Shell::Shutdown();
		return -1;
	}

	// Install our DirectX render interface into RmlUi.
	Rml::Core::SetRenderInterface(&directx_renderer);

	ShellSystemInterface system_interface;
	Rml::Core::SetSystemInterface(&system_interface);

	Rml::Core::Initialise();

	// Create the main RmlUi context and set it on the shell's input layer.
	context = Rml::Core::CreateContext("main", Rml::Core::Vector2i(window_width, window_height));
	if (context == nullptr)
	{
		Rml::Core::Shutdown();
		Shell::Shutdown();
		return -1;
	}

	Rml::Debugger::Initialise(context);
	Input::SetContext(context);
	shell_renderer->SetContext(context);

	Shell::LoadFonts("assets/");

	// Load and show the tutorial document.
	Rml::Core::ElementDocument* document = context->LoadDocument("assets/demo.rml");
	if (document != nullptr)
	{
		document->Show();
		document->RemoveReference();
	}

	Shell::EventLoop(GameLoop);

	// Shutdown RmlUi.
	context->RemoveReference();
	Rml::Core::Shutdown();

	Shell::CloseWindow();
	Shell::Shutdown();

	return 0;
}
