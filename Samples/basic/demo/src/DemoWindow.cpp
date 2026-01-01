#include "DemoWindow.h"
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

bool DemoWindow::Initialize(const Rml::String& title, Rml::Context* context)
{
	using namespace Rml;

	document = context->LoadDocument("basic/demo/data/demo.rml");
	if (!document)
		return false;

	document->GetElementById("title")->SetInnerRML(title);

	// Add sandbox default text.
	if (auto source = rmlui_dynamic_cast<Rml::ElementFormControl*>(document->GetElementById("sandbox_rml_source")))
	{
		auto value = source->GetValue();
		value += "<p>Write your RML here</p>\n\n<!-- <img src=\"assets/high_scores_alien_1.tga\"/> -->";
		source->SetValue(value);
	}

	// Prepare sandbox document.
	if (auto target = document->GetElementById("sandbox_target"))
	{
		iframe = context->CreateDocument();
		auto iframe_ptr = iframe->GetParentNode()->RemoveChild(iframe);
		target->AppendChild(std::move(iframe_ptr));
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

	// Add sandbox style sheet text.
	if (auto source = rmlui_dynamic_cast<Rml::ElementFormControl*>(document->GetElementById("sandbox_rcss_source")))
	{
		Rml::String value = "/* Write your RCSS here */\n\n/* body { color: #fea; background: #224; }\nimg { image-color: red; } */";
		source->SetValue(value);
		SetSandboxStylesheet(value);
	}

	gauge = document->GetElementById("gauge");
	progress_horizontal = document->GetElementById("progress_horizontal");

	document->Show();

	return true;
}

void DemoWindow::Shutdown()
{
	if (document)
	{
		document->Close();
		document = nullptr;
	}
}

void DemoWindow::Update()
{
	if (iframe)
		iframe->UpdateDocument();

	if (submitting && gauge && progress_horizontal)
	{
		using namespace Rml;
		constexpr float progressbars_time = 2.f;
		const float progress = Math::Min(float(GetSystemInterface()->GetElapsedTime() - submitting_start_time) / progressbars_time, 2.f);

		float value_gauge = 1.0f;
		float value_horizontal = 0.0f;
		if (progress < 1.0f)
			value_gauge = 0.5f - 0.5f * Math::Cos(Math::RMLUI_PI * progress);
		else
			value_horizontal = 0.5f - 0.5f * Math::Cos(Math::RMLUI_PI * (progress - 1.0f));

		progress_horizontal->SetAttribute("value", value_horizontal);

		const float value_begin = 0.09f;
		const float value_end = 1.f - value_begin;
		float value_mapped = value_begin + value_gauge * (value_end - value_begin);
		gauge->SetAttribute("value", value_mapped);

		auto value_gauge_str = CreateString("%d %%", Math::RoundToInteger(value_gauge * 100.f));
		auto value_horizontal_str = CreateString("%d %%", Math::RoundToInteger(value_horizontal * 100.f));

		if (auto el_value = document->GetElementById("gauge_value"))
			el_value->SetInnerRML(value_gauge_str);
		if (auto el_value = document->GetElementById("progress_value"))
			el_value->SetInnerRML(value_horizontal_str);

		String label = "Placing tubes";
		size_t num_dots = (size_t(progress * 10.f) % 4);
		if (progress > 1.0f)
			label += "... Placed! Assembling message";
		if (progress < 2.0f)
			label += String(num_dots, '.');
		else
			label += "... Done!";

		if (auto el_label = document->GetElementById("progress_label"))
			el_label->SetInnerRML(label);

		if (progress >= 2.0f)
		{
			submitting = false;
			if (auto el_output = document->GetElementById("form_output"))
				el_output->SetInnerRML(submit_message);
		}

		document->GetContext()->RequestNextUpdate(.0);
	}
}

void DemoWindow::ProcessEvent(Rml::Event& event)
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
	break;

	default: break;
	}
}

Rml::ElementDocument* DemoWindow::GetDocument()
{
	return document;
}

void DemoWindow::SubmitForm(Rml::String in_submit_message)
{
	submitting = true;
	submitting_start_time = Rml::GetSystemInterface()->GetElapsedTime();
	submit_message = in_submit_message;
	if (auto el_output = document->GetElementById("form_output"))
		el_output->SetInnerRML("");
	if (auto el_progress = document->GetElementById("submit_progress"))
		el_progress->SetProperty("display", "block");
}

void DemoWindow::SetSandboxStylesheet(const Rml::String& string)
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

void DemoWindow::SetSandboxBody(const Rml::String& string)
{
	if (iframe)
		iframe->SetInnerRML(string);
}

TweeningParameters DemoWindow::GetTweeningParameters() const
{
	return tweening_parameters;
}

void DemoWindow::SetTweeningParameters(TweeningParameters in_tweening_parameters)
{
	tweening_parameters = in_tweening_parameters;
}
