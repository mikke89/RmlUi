#include "SandboxWindow.h"
#include "RmlUi/Core/StreamMemory.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Elements/ElementFormControl.h>
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Core/StyleSheetContainer.h>
#include <RmlUi_Backend.h>

static const Rml::String g_sandbox_default_rcss = R"(
body { top: 0; left: 0; right: 0; bottom: 0; overflow: hidden auto; }
scrollbarvertical { width: 15px; }
scrollbarvertical slidertrack { background: #eee; }
scrollbarvertical slidertrack:active { background: #ddd; }
scrollbarvertical sliderbar { width: 15px; min-height: 30px; background: #aaa; }
scrollbarvertical sliderbar:hover { background: #888; }
scrollbarvertical sliderbar:active { background: #666; }
scrollbarhorizontal { height: 15px; }
scrollbarhorizontal slidertrack { background: #eee; }
scrollbarhorizontal slidertrack:active { background: #ddd; }
scrollbarhorizontal sliderbar { height: 15px; min-width: 30px; background: #aaa; }
scrollbarhorizontal sliderbar:hover { background: #888; }
scrollbarhorizontal sliderbar:active { background: #666; }
)";

SandboxWindow::SandboxWindow(SandboxLogger* logger) : logger(logger) {}

bool SandboxWindow::Initialize(const Rml::String& title, Rml::Context* context)
{
	using namespace Rml;

	document = context->LoadDocument("sandbox/data/sandbox.rml");
	if (!document)
		return false;

	el_rml_source = rmlui_dynamic_cast<Rml::ElementFormControl*>(document->GetElementById("sandbox_rml_source"));
	if (!el_rml_source)
		return false;

	// Add sandbox default text.
	{
		auto value = el_rml_source->GetValue();
		value += "<p>Write your RML here</p>\n\n<!-- <img src=\"assets/high_scores_alien_1.tga\"/> -->";
		el_rml_source->SetValue(value);
		el_rml_source->AddEventListener(Rml::EventId::Change, this);
	}

	el_target = document->GetElementById("sandbox_target");
	if (!el_target)
		return false;

	// Prepare sandbox document.
	{
		iframe = context->CreateDocument();
		auto iframe_ptr = iframe->GetParentNode()->RemoveChild(iframe);
		el_target->AppendChild(std::move(iframe_ptr));
		iframe->SetProperty(PropertyId::Position, Property(Style::Position::Absolute));
		iframe->SetProperty(PropertyId::Display, Property(Style::Display::Block));
		iframe->SetInnerRML("<p>Rendered output goes here.</p>");

		// Load basic RML style sheet
		Rml::String style_sheet_content;
		{
			// Load file into string
			auto file_interface = Rml::GetFileInterface();
			Rml::FileHandle handle = file_interface->Open("assets/rml.rcss");

			size_t length = file_interface->Length(handle);
			style_sheet_content.resize(length);
			file_interface->Read((void*)style_sheet_content.data(), length, handle);
			file_interface->Close(handle);

			style_sheet_content += g_sandbox_default_rcss;
		}

		Rml::StreamMemory stream((Rml::byte*)style_sheet_content.data(), style_sheet_content.size());
		stream.SetSourceURL("sandbox://default_rcss");

		rml_basic_style_sheet = MakeShared<Rml::StyleSheetContainer>();
		rml_basic_style_sheet->LoadStyleSheetContainer(&stream);
	}

	el_rcss_source = rmlui_dynamic_cast<Rml::ElementFormControl*>(document->GetElementById("sandbox_rcss_source"));
	if (!el_rcss_source)
		return false;

	// Add sandbox style sheet text.
	{
		Rml::String value = "/* Write your RCSS here */\n\n/* body { color: #fea; background: #224; }\nimg { image-color: red; } */";
		el_rcss_source->SetValue(value);
		SetSandboxStylesheet(value);
		el_rcss_source->AddEventListener(Rml::EventId::Change, this);
	}

	document->AddEventListener(Rml::EventId::Keydown, this);
	document->AddEventListener(Rml::EventId::Keyup, this);
	document->AddEventListener(Rml::EventId::Animationend, this);

	document->Show();

	return true;
}

void SandboxWindow::Shutdown()
{
	if (document)
	{
		document->Close();
		document = nullptr;
	}
}

void SandboxWindow::Update()
{
	iframe->UpdateDocument();
}

void SandboxWindow::ProcessEvent(Rml::Event& event)
{
	using namespace Rml;

	switch (event.GetId())
	{
	case EventId::Keydown:
	{
		Rml::Input::KeyIdentifier key_identifier = (Rml::Input::KeyIdentifier)event.GetParameter<int>("key_identifier", 0);

		if (key_identifier == Rml::Input::KI_ESCAPE)
			Backend::RequestExit();
	}
	case EventId::Change:
	{
		if (event.GetCurrentElement() == el_rcss_source)
		{
			auto value = event.GetParameter<Rml::String>("value", "");
			SetSandboxStylesheet(value);
		}
		else if (event.GetCurrentElement() == el_rml_source)
		{
			auto value = event.GetParameter<Rml::String>("value", "");
			SetSandboxBody(value);
		}
	}
	default: break;
	}
}

Rml::ElementDocument* SandboxWindow::GetDocument()
{
	return document;
}

void SandboxWindow::SetSandboxStylesheet(const Rml::String& string)
{
	if (iframe && rml_basic_style_sheet)
	{
		auto style = Rml::MakeShared<Rml::StyleSheetContainer>();
		Rml::StreamMemory stream((const Rml::byte*)string.data(), string.size());
		stream.SetSourceURL("sandbox://rcss");

		style->LoadStyleSheetContainer(&stream);
		style = rml_basic_style_sheet->CombineStyleSheetContainer(*style);
		iframe->SetStyleSheetContainer(style);
	}
}

void SandboxWindow::SetSandboxBody(const Rml::String& string)
{
	if (iframe)
		iframe->SetInnerRML(string);
}
