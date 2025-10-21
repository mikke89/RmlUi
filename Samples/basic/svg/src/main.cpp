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

#include <RmlUi/Core.h>
#include <RmlUi/Debugger.h>
#include <RmlUi_Backend.h>
#include <Shell.h>

class ButtonClickEventListener final : public Rml::EventListener {
public:
	void ProcessEvent(Rml::Event& event) override;
};

static ButtonClickEventListener click_listener;

void ButtonClickEventListener::ProcessEvent(Rml::Event& event)
{
	if (event == Rml::EventId::Click)
	{
		Rml::Element* current_element = event.GetCurrentElement();
		Rml::ElementDocument* document = current_element->GetOwnerDocument();
		if (document)
		{
			Rml::ElementList elements;
			document->GetElementsByTagName(elements, "svg");
			for (size_t i = 0; i < elements.size(); i++)
			{
				if (elements[i]->GetAttribute<std::string>("_toggle", "").empty())
				{
					elements[i]->SetInnerRML("<circle cx=\"25\" cy=\"25\" r=\"20\" stroke=\"yellow\" stroke-width=\"3\" fill=\"green\" />");
					elements[i]->SetAttribute("_toggle", "1");
				}
				else
				{
					elements[i]->SetInnerRML("<circle cx=\"25\" cy=\"25\" r=\"20\" stroke=\"black\" stroke-width=\"3\" fill=\"red\" />");
					elements[i]->SetAttribute("_toggle", "");
				}
			}
		}
	}
}

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
	if (!Backend::Initialize("SVG sample", window_width, window_height, true))
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
	if (context == nullptr)
	{
		Rml::Shutdown();
		Backend::Shutdown();
		Shell::Shutdown();
		return -1;
	}

	Rml::Debugger::Initialise(context);
	Shell::LoadFonts();

	// Load and show the documents.
	std::vector<std::pair<std::string, std::string>> rml_docs = {{"basic/svg/data/svg_element.rml", "SVG Element"},
		{"basic/svg/data/svg_decorator.rml", "SVG Decorator"}, {"basic/svg/data/svg_inline.rml", "SVG Inline"}};
	for (const auto& rml_doc : rml_docs)
	{
		if (Rml::ElementDocument* document = context->LoadDocument(rml_doc.first))
		{
			document->Show();
			document->GetElementById("title")->SetInnerRML(rml_doc.second);
			Rml::Element* e = document->GetElementById("svg_inline_button");
			if (e != nullptr)
			{
				e->AddEventListener("click", &click_listener, false);
			}
		}
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
