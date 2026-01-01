#include "SystemFontWin32.h"
#include <RmlUi/Core.h>
#include <RmlUi/Debugger.h>
#include <PlatformExtensions.h>
#include <RmlUi_Backend.h>
#include <RmlUi_Include_Windows.h>
#include <Shell.h>

#if !defined RMLUI_PLATFORM_WIN32
	#error "This sample works only on Windows!"
#endif

static void LoadFonts()
{
	struct FontFace {
		Rml::String filename;
		bool fallback_face;
	};
	Rml::Vector<FontFace> font_faces = {
		{"assets/LatoLatin-Regular.ttf", false},
		{"assets/LatoLatin-Italic.ttf", false},
		{"assets/LatoLatin-Bold.ttf", false},
		{"assets/LatoLatin-BoldItalic.ttf", false},
	};

	for (const Rml::String& path : GetSelectedSystemFonts())
		font_faces.push_back({path, true});

	for (const FontFace& face : font_faces)
		Rml::LoadFontFace(face.filename, face.fallback_face);
}

int APIENTRY WinMain(HINSTANCE /*instance_handle*/, HINSTANCE /*previous_instance_handle*/, char* /*command_line*/, int /*command_show*/)
{
	const int window_width = 1024;
	const int window_height = 768;

	// Initializes the shell which provides common functionality used by the included samples.
	if (!Shell::Initialize())
		return -1;

	// Constructs the system and render interfaces, creates a window, and attaches the renderer.
	if (!Backend::Initialize("IME Sample", window_width, window_height, true))
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
		Backend::Shutdown();
		Shell::Shutdown();
		return -1;
	}

	Rml::Debugger::Initialise(context);

	// Load required fonts with support for most character sets.
	LoadFonts();

	// Load and show the demo document.
	Rml::ElementDocument* document = context->LoadDocument("basic/ime/data/ime.rml");
	if (document)
		document->Show();

	bool running = true;
	while (running)
	{
		running = Backend::ProcessEvents(context, &Shell::ProcessKeyDownShortcuts, true);

		context->Update();

		Backend::BeginFrame();
		context->Render();
		Backend::PresentFrame();
	}

	Rml::Shutdown();

	Backend::Shutdown();
	Shell::Shutdown();

	return 0;
}
