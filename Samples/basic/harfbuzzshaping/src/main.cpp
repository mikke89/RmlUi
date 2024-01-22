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

#include "FontEngineInterfaceHarfBuzz.h"
#include <RmlUi/Core.h>
#include <RmlUi/Debugger.h>
#include <RmlUi_Backend.h>
#include <Shell.h>

/*
    This demo shows how to create a custom text-shaping font engine implementation using HarfBuzz.
*/

// Toggle this variable to enable/disable text shaping.
constexpr bool EnableTextShaping = true;

class HarfBuzzEventListener : public Rml::EventListener {
public:
	HarfBuzzEventListener(const Rml::String& value, Rml::ElementDocument* document) :
		value(value), english_element(document->GetElementById("lorem-ipsum-en")), arabic_element(document->GetElementById("lorem-ipsum-ar")),
		hindi_element(document->GetElementById("lorem-ipsum-hi"))
	{}

	void ProcessEvent(Rml::Event& /*event*/) override
	{
		if (value == "set-english")
		{
			english_element->SetClass("hide-lorem-ipsum", false);
			arabic_element->SetClass("hide-lorem-ipsum", true);
			hindi_element->SetClass("hide-lorem-ipsum", true);
		}
		else if (value == "set-arabic")
		{
			english_element->SetClass("hide-lorem-ipsum", true);
			arabic_element->SetClass("hide-lorem-ipsum", false);
			hindi_element->SetClass("hide-lorem-ipsum", true);
		}
		else if (value == "set-hindi")
		{
			english_element->SetClass("hide-lorem-ipsum", true);
			arabic_element->SetClass("hide-lorem-ipsum", true);
			hindi_element->SetClass("hide-lorem-ipsum", false);
		}
	}

	void OnDetach(Rml::Element* /*element*/) override { delete this; }

private:
	Rml::String value;

	Rml::Element* english_element;
	Rml::Element* arabic_element;
	Rml::Element* hindi_element;
};

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
	if (!Backend::Initialize("HarfBuzz Text Shaping Sample", window_width, window_height, true))
	{
		Shell::Shutdown();
		return -1;
	}

	// Install the custom interfaces constructed by the backend before initializing RmlUi.
	Rml::SetSystemInterface(Backend::GetSystemInterface());
	Rml::SetRenderInterface(Backend::GetRenderInterface());

	// Construct and load the font interface.
	Rml::UniquePtr<FontEngineInterfaceHarfBuzz> font_interface = nullptr;
	if (EnableTextShaping)
	{
		font_interface = Rml::MakeUnique<FontEngineInterfaceHarfBuzz>();
		Rml::SetFontEngineInterface(font_interface.get());

		// Add language data to the font engine's internal language lookup table.
		font_interface->RegisterLanguage("en", "Latn", TextFlowDirection::LeftToRight);
		font_interface->RegisterLanguage("ar", "Arab", TextFlowDirection::RightToLeft);
		font_interface->RegisterLanguage("hi", "Deva", TextFlowDirection::LeftToRight);
	}

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

	// Load required fonts.
	Rml::String font_paths[3] = {
		"assets/LatoLatin-Regular.ttf",
		"basic/harfbuzzshaping/data/Cairo-Regular.ttf",
		"basic/harfbuzzshaping/data/Poppins-Regular.ttf",
	};
	for (const Rml::String& font_path : font_paths)
		if (!Rml::LoadFontFace(font_path))
		{
			Rml::Shutdown();
			Backend::Shutdown();
			Shell::Shutdown();
			return -1;
		}

	// Load and show the demo document.
	if (Rml::ElementDocument* document = context->LoadDocument("basic/harfbuzzshaping/data/harfbuzzshaping.rml"))
	{
		if (auto el = document->GetElementById("title"))
			el->SetInnerRML("HarfBuzz Text Shaping");

		document->Show();

		// Create event handlers.
		for (const Rml::String& button_id : {"set-english", "set-arabic", "set-hindi"})
			document->GetElementById(button_id)->AddEventListener(Rml::EventId::Click, new HarfBuzzEventListener(button_id, document));
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

	// Shut down debugger before font interface.
	Rml::Debugger::Shutdown();
	if (EnableTextShaping)
		font_interface.reset();

	// Shutdown RmlUi.
	Rml::Shutdown();

	Backend::Shutdown();
	Shell::Shutdown();

	return 0;
}
