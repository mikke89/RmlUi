#include "SandboxWindow.h"
#include "SandboxSystemInterface.h"
#include "XmlNodeHandlerSandboxMeta.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Elements/ElementFormControl.h>
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Core/StreamMemory.h>
#include <RmlUi/Core/StringUtilities.h>
#include <RmlUi/Core/StyleSheetContainer.h>
#include <RmlUi_Backend.h>

static const Rml::String g_sandbox_document_rcss = R"(
body { display: block; }
)";

static const Rml::String g_sandbox_fragment_rcss = R"(
body { display: block; top: 0; left: 0; right: 0; bottom: 0; overflow: hidden auto; background: #fff; }
scrollbarvertical { width: 15dp; }
scrollbarvertical slidertrack { background: #eee; }
scrollbarvertical slidertrack:active { background: #ddd; }
scrollbarvertical sliderbar { width: 15dp; min-height: 30dp; background: #aaa; }
scrollbarvertical sliderbar:hover { background: #888; }
scrollbarvertical sliderbar:active { background: #666; }
scrollbarhorizontal { height: 15dp; }
scrollbarhorizontal slidertrack { background: #eee; }
scrollbarhorizontal slidertrack:active { background: #ddd; }
scrollbarhorizontal sliderbar { height: 15dp; min-width: 30dp; background: #aaa; }
scrollbarhorizontal sliderbar:hover { background: #888; }
scrollbarhorizontal sliderbar:active { background: #666; }
)";

static const Rml::String g_default_rml_source = R"(<!-- Write your RML here, or load a document from file. -->

<p>All your base are belong to us.</p>
<img src="/assets/high_scores_alien_1.tga" />
)";

static const Rml::String g_default_rcss_source = R"(/* Write your RCSS here */

body {
  font-size: 16dp;
  color: #fea;
  background: #224;
  padding: 1em;
}
img {
  image-color: red;
}
)";

static Rml::SharedPtr<Rml::StyleSheetContainer> MakeStyleSheet(const Rml::String& content, const Rml::String& source_url)
{
	Rml::StreamMemory stream((const Rml::byte*)content.data(), content.size());
	stream.SetSourceURL(source_url);

	auto style_sheet = Rml::MakeShared<Rml::StyleSheetContainer>();
	style_sheet->LoadStyleSheetContainer(&stream);
	return style_sheet;
}

static bool IsCompleteDocument(const Rml::String& rml_source)
{
	return Rml::StringUtilities::StripWhitespace(rml_source).substr(0, 4) == "<rml";
}

static Rml::String GetDirectoryOf(const Rml::String& path)
{
	const size_t i = path.find_last_of("/\\");
	return (i == Rml::String::npos ? Rml::String() : path.substr(0, i + 1));
}

static const char* GetLogClass(Rml::Log::Type type)
{
	static const char* message_type_str[Rml::Log::Type::LT_MAX] = {"Always", "Error", "Assert", "Warning", "Info", "Debug"};
	return message_type_str[type];
}

SandboxWindow::SandboxWindow(SandboxSystemInterface* system_interface, SandboxFileInterface* file_interface) :
	system_interface(system_interface), file_interface(file_interface)
{}

bool SandboxWindow::Initialize(Rml::Context* context)
{
	using namespace Rml;

	Shutdown();

	meta_handler = MakeShared<XMLNodeHandlerSandboxMeta>();
	Rml::XMLParser::RegisterNodeHandler("meta", meta_handler);

	document = context->LoadDocument("sandbox/data/sandbox.rml");
	if (!document)
		return false;

	auto GetFormControl = [this](const char* id) { return rmlui_dynamic_cast<ElementFormControl*>(document->GetElementById(id)); };

	el_rml_source = GetFormControl("sandbox_rml_source");
	el_rcss_source = GetFormControl("sandbox_rcss_source");
	el_file_path = GetFormControl("load_file_path");
	el_working_directory = GetFormControl("working_directory");
	el_themes = GetFormControl("active_themes");
	el_dp_ratio = GetFormControl("dp_ratio");
	el_inject_rcss = GetFormControl("inject_rcss");
	el_override_font_family = GetFormControl("override_font_family");
	el_target = document->GetElementById("sandbox_target");
	el_status = document->GetElementById("load_file_status");
	el_sources = document->GetElementById("sources");
	el_messages = document->GetElementById("messages");

	if (!el_rml_source || !el_rcss_source || !el_file_path || !el_working_directory || !el_themes || !el_dp_ratio || !el_inject_rcss ||
		!el_override_font_family || !el_target || !el_status || !el_sources || !el_messages)
	{
		Log::Message(Log::LT_ERROR, "Sandbox document is missing one or more of its required elements.");
		return false;
	}

	{
		String rml_rcss;
		if (!GetFileInterface()->LoadFile("assets/rml.rcss", rml_rcss))
			Log::Message(Log::LT_WARNING, "Could not load the basic RML style sheet, sandboxed RML fragments will be unstyled.");

		fragment_style_sheet = MakeStyleSheet(rml_rcss + g_sandbox_fragment_rcss, "sandbox://fragment_rcss");
	}

	el_rml_source->SetValue(g_default_rml_source);
	el_rcss_source->SetValue(g_default_rcss_source);

	for (ElementFormControl* element :
		{el_rml_source, el_rcss_source, el_file_path, el_working_directory, el_themes, el_dp_ratio, el_inject_rcss, el_override_font_family})
	{
		element->AddEventListener(EventId::Change, this);
	}

	document->AddEventListener(EventId::Click, this);
	document->AddEventListener(EventId::Keydown, this);

	ReloadSandboxDocument();

	document->Show();

	return true;
}

void SandboxWindow::Shutdown()
{
	base_style_sheet.reset();
	fragment_style_sheet.reset();
	sandbox_document = nullptr;

	if (document)
	{
		document->Close();
		document = nullptr;
	}
}

void SandboxWindow::Update()
{
	if (sandbox_document)
		sandbox_document->UpdateDocument();
}

void SandboxWindow::ProcessEvent(Rml::Event& event)
{
	using namespace Rml;
	Context* context = document->GetContext();

	Element* element = event.GetCurrentElement();

	switch (event.GetId())
	{
	case EventId::Keydown:
	{
		const Input::KeyIdentifier key_identifier = (Input::KeyIdentifier)event.GetParameter<int>("key_identifier", 0);
		const bool shift = event.GetParameter<int>("shift_key", 0) > 0;
		const bool ctrl = event.GetParameter<int>("ctrl_key", 0) > 0;
		const bool alt = event.GetParameter<int>("alt_key", 0) > 0;

		if (key_identifier == Input::KI_ESCAPE)
			Backend::RequestExit();
		else if (key_identifier == Input::KI_R && shift && ctrl && !alt)
			Initialize(document->GetContext());
	}
	break;
	case EventId::Click:
	{
		Element* target = event.GetTargetElement();
		if (target->GetId() == "load_file")
			LoadDocumentFromFile();
		else if (target->GetId() == "load_from_clipboard")
			LoadDocumentFromClipboard();
	}
	break;
	case EventId::Change:
	{
		const String value = event.GetParameter<String>("value", "");
		const bool submitted = event.GetParameter<bool>("linebreak", false);

		if (element == el_rml_source)
		{
			ReloadSandboxDocument();
		}
		else if (element == el_rcss_source)
		{
			UpdateSandboxStylesheet();
		}
		else if (element == el_themes)
		{
			const String effective_value = (value.empty() ? element->GetAttribute("placeholder", String()) : value);
			Rml::StringList set_themes;
			Rml::StringUtilities::ExpandString(set_themes, effective_value);

			const Rml::StringList active_themes = context->GetActiveThemes();
			for (const Rml::String& theme : active_themes)
			{
				if (std::find(set_themes.begin(), set_themes.end(), theme) == set_themes.end())
					context->ActivateTheme(theme, false);
			}
			for (const Rml::String& theme : set_themes)
				context->ActivateTheme(theme, true);

			UpdateSandboxStylesheet();
		}
		else if (element == el_dp_ratio)
		{
			const String effective_value = (value.empty() ? element->GetAttribute("placeholder", String()) : value);
			const float dp_ratio = FromString(effective_value, 1.f);
			context->SetDensityIndependentPixelRatio(Rml::Math::Clamp(dp_ratio, 0.5f, 5.f));
		}
		else if (element == el_inject_rcss)
		{
			el_sources->SetClass("hide_rcss_source", !event.GetParameter<bool>("checked", false));
			UpdateSandboxStylesheet();
		}
		else if (element == el_override_font_family)
		{
			UpdateSandboxStylesheet();
		}
		else if ((element == el_file_path || element == el_working_directory) && submitted)
		{
			LoadDocumentFromFile();
		}
	}
	break;
	default: break;
	}
}

void SandboxWindow::ReloadSandboxDocument()
{
	Rml::Context* context = document->GetContext();
	system_interface->PopMessages();

	if (sandbox_document)
	{
		el_target->RemoveChild(sandbox_document);
		sandbox_document = nullptr;
	}
	base_style_sheet.reset();

	const Rml::String rml_source = el_rml_source->GetValue();
	const bool is_complete_document = IsCompleteDocument(rml_source);

	Rml::ElementDocument* new_document = nullptr;
	if (is_complete_document)
	{
		meta_handler->ClearMetaList();
		new_document = context->LoadDocumentFromMemory(rml_source, sandbox_source_url.empty() ? Rml::String("sandbox://rml") : sandbox_source_url);

		if (!new_document)
		{
			SetStatus("Failed to instance the sandboxed document.", true);
			return;
		}

		for (const auto& [name, content] : meta_handler->GetMetaList())
		{
			if (name == "density-independent-pixel-ratio")
			{
				el_dp_ratio->SetAttribute("placeholder", content);
				if (el_dp_ratio->GetValue().empty())
					el_dp_ratio->DispatchEvent(Rml::EventId::Change, Rml::Dictionary{{"value", Rml::Variant{Rml::String()}}});
			}
			else if (name == "active-themes")
			{
				el_themes->SetAttribute("placeholder", content);
				if (el_themes->GetValue().empty())
					el_themes->DispatchEvent(Rml::EventId::Change, Rml::Dictionary{{"value", Rml::Variant{Rml::String()}}});
			}
		}

		auto defaults = MakeStyleSheet(g_sandbox_document_rcss, "sandbox://document_rcss");
		if (const Rml::StyleSheetContainer* document_style_sheet = new_document->GetStyleSheetContainer())
			base_style_sheet = defaults->CombineStyleSheetContainer(*document_style_sheet);
		else
			base_style_sheet = std::move(defaults);
	}
	else
	{
		new_document = context->CreateDocument();
		base_style_sheet = fragment_style_sheet;
		new_document->SetInnerRML(rml_source);
	}

	el_target->AppendChild(new_document->GetParentNode()->RemoveChild(new_document));
	sandbox_document = new_document;

	sandbox_document->Show(Rml::ModalFlag::None, Rml::FocusFlag::None);

	UpdateSandboxStylesheet();
}

void SandboxWindow::UpdateSandboxStylesheet()
{
	if (!sandbox_document || !base_style_sheet)
		return;

	sandbox_document->SetStyleSheetContainer(nullptr);

	auto new_style = base_style_sheet;

	if (el_inject_rcss->HasAttribute("checked"))
	{
		auto user_style_sheet = MakeStyleSheet(el_rcss_source->GetValue(), "sandbox://rcss");
		new_style = new_style->CombineStyleSheetContainer(*user_style_sheet);
	}

	if (el_override_font_family->HasAttribute("checked"))
	{
		const Rml::String override_font_family_rcss =
			"*:not(#rmlui-high-precedence1):not(#rmlui-high-precedence2):not(#rmlui-high-precedence3) { font-family: rmlui-debugger-font; }";
		auto user_style_sheet = MakeStyleSheet(override_font_family_rcss, "sandbox://override_font_family");
		new_style = new_style->CombineStyleSheetContainer(*user_style_sheet);
	}

	sandbox_document->SetStyleSheetContainer(new_style);

	RefreshMessages();
}

void SandboxWindow::LoadDocumentFromFile()
{
	const Rml::String path = Rml::StringUtilities::Replace(Rml::StringUtilities::StripWhitespace(el_file_path->GetValue()), '\\', '\\');
	if (path.empty())
	{
		SetStatus("Enter the path of a document to load.", true);
		return;
	}

	Rml::String working_directory = Rml::StringUtilities::StripWhitespace(el_working_directory->GetValue());
	if (working_directory.empty())
	{
		working_directory = GetDirectoryOf(path);
		el_working_directory->SetValue(working_directory);
	}
	file_interface->SetWorkingDirectory(working_directory);

	Rml::String rml_source;
	if (!Rml::GetFileInterface()->LoadFile(path, rml_source))
	{
		SetStatus("Could not open '" + path + "'.", true);
		return;
	}

	sandbox_source_url = path;
	el_rml_source->SetValue(rml_source);

	SetStatus("Successfully loaded file.", false);
}

void SandboxWindow::LoadDocumentFromClipboard()
{
	Rml::String rml_source;
	Rml::GetSystemInterface()->GetClipboardText(rml_source);

	if (rml_source.empty())
	{
		SetStatus("No clipboard text available.", true);
		return;
	}

	const Rml::String working_directory = Rml::StringUtilities::StripWhitespace(el_working_directory->GetValue());
	file_interface->SetWorkingDirectory(working_directory);

	sandbox_source_url = working_directory + "\\rml_pasted_document.rml";
	el_rml_source->SetValue(rml_source);

	SetStatus("Successfully loaded from clipboard.", false);
}

void SandboxWindow::RefreshMessages()
{
	Rml::Vector<SandboxSystemInterface::Message> messages = system_interface->PopMessages();

	el_messages->SetInnerRML("");

	for (const SandboxSystemInterface::Message& message : messages)
	{
		Rml::ElementPtr entry = document->CreateElement("div");
		entry->SetClass(GetLogClass(message.type), true);
		entry->SetInnerRML(Rml::StringUtilities::EncodeRml(message.text));
		el_messages->AppendChild(std::move(entry));
	}

	document->UpdateDocument();
	el_messages->SetScrollTop(el_messages->GetScrollHeight());
}

void SandboxWindow::SetStatus(const Rml::String& message, bool error)
{
	el_status->SetInnerRML(Rml::StringUtilities::EncodeRml(message));
	el_status->SetClass("error", error);
	el_status->SetClass("visible", true);

	// Update document to trigger transition on next visibility change.
	document->UpdateDocument();
	el_status->SetClass("visible", false);
}
