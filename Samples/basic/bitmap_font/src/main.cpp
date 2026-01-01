#include "FontEngineInterfaceBitmap.h"
#include <RmlUi/Core.h>
#include <RmlUi/Debugger.h>
#include <RmlUi_Backend.h>
#include <Shell.h>

/*

    This demo shows how to create a custom bitmap font engine implementation.

    It should work even when RmlUi is compiled without the default font engine (see CMake flag 'NO_FONT_INTERFACE_DEFAULT').
    See the interface in 'FontEngineInterfaceBitmap.h' and the implementation in 'FontEngineBitmap.h'.

*/

#if defined RMLUI_PLATFORM_WIN32
	#include <RmlUi_Include_Windows.h>
int APIENTRY WinMain(HINSTANCE /*instance_handle*/, HINSTANCE /*previous_instance_handle*/, char* /*command_line*/, int /*command_show*/)
#else
int main(int /*argc*/, char** /*argv*/)
#endif
{
	int window_width = 1024;
	int window_height = 768;

	// Initializes the shell which provides common functionality used by the included samples.
	if (!Shell::Initialize())
		return -1;

	// Constructs the system and render interfaces, creates a window, and attaches the renderer.
	if (!Backend::Initialize("Bitmap Font Sample", window_width, window_height, true))
	{
		Shell::Shutdown();
		return -1;
	}

	// Install the custom interfaces constructed by the backend before initializing RmlUi.
	Rml::SetSystemInterface(Backend::GetSystemInterface());
	Rml::SetRenderInterface(Backend::GetRenderInterface());

	// Construct and load the font interface.
	auto font_interface = Rml::MakeUnique<FontEngineInterfaceBitmap>();
	Rml::SetFontEngineInterface(font_interface.get());

	// RmlUi initialisation.
	Rml::Initialise();

	// Create the main RmlUi context.
	Rml::Context* context = Rml::CreateContext("main", Rml::Vector2i(window_width, window_height));
	if (!context)
	{
		Rml::Shutdown();
		Backend::Shutdown();
		Shell::Shutdown();
		return -1;
	}

	Rml::Debugger::Initialise(context);

	// Load bitmap font
	if (!Rml::LoadFontFace("basic/bitmap_font/data/Comfortaa_Regular_22.fnt"))
	{
		Rml::Shutdown();
		Backend::Shutdown();
		Shell::Shutdown();
		return -1;
	}

	// Load and show the demo document.
	if (Rml::ElementDocument* document = context->LoadDocument("basic/bitmap_font/data/bitmap_font.rml"))
	{
		if (auto el = document->GetElementById("title"))
			el->SetInnerRML("Bitmap font");

		document->Show();
	}

	bool running = true;
	while (running)
	{
		running = Backend::ProcessEvents(context, &Shell::ProcessKeyDownShortcuts, true);

		context->Update();

		Backend::BeginFrame();
		context->Render();
		Backend::PresentFrame();
	}

	// Shutdown RmlUi.
	Rml::Shutdown();

	Backend::Shutdown();
	Shell::Shutdown();

	return 0;
}
