#include "SandboxWindow.h"
#include "SandboxStorage.h"
#include "SandboxSystemInterface.h"
#include "XmlNodeHandlerSandboxMeta.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/ElementUtilities.h>
#include <RmlUi/Core/Elements/ElementFormControl.h>
#include <RmlUi/Core/Elements/ElementFormControlInput.h>
#include <RmlUi/Core/Factory.h>
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Core/StreamMemory.h>
#include <RmlUi/Core/StringUtilities.h>
#include <RmlUi/Core/StyleSheetContainer.h>
#include <RmlUi_Backend.h>
#include <algorithm>
#include <optional>

static const Rml::String g_options_path = "sandbox_options.ini";
static const Rml::String g_source_rml_path = "sandbox_source.rml";
static const Rml::String g_source_rcss_path = "sandbox_source.rcss";

// After making a change, wait for this amount of time before saving options and sources.
static constexpr double g_options_save_delay = 3.0;

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

static Rml::String GetValueOrPlaceholder(Rml::Element* element)
{
	Rml::String value = element->GetAttribute("value", Rml::String());
	if (!value.empty())
		return value;

	return element->GetAttribute("placeholder", Rml::String());
}

static void SetPlaceholder(Rml::Element* element, const Rml::String& value)
{
	element->SetAttribute("placeholder", value);
}

static bool SetPlaceholderAndCompare(Rml::Element* element, const Rml::String& value)
{
	const Rml::String value_before = GetValueOrPlaceholder(element);
	element->SetAttribute("placeholder", value);
	return GetValueOrPlaceholder(element) != value_before;
}

static bool IsCheckbox(Rml::Element* element)
{
	return rmlui_dynamic_cast<Rml::ElementFormControlInput*>(element) && element->GetAttribute<Rml::String>("type", "") == "checkbox";
}

static void SetChecked(Rml::Element* element, bool checked)
{
	if (checked)
		element->SetAttribute("checked", true);
	else
		element->RemoveAttribute("checked");
}

static void ClearCaches()
{
	Rml::Factory::ClearStyleSheetCache();
	Rml::Factory::ClearTemplateCache();
	Rml::ReleaseTextures();
}

SandboxWindow::SandboxWindow(SandboxSystemInterface* system_interface, SandboxFileInterface* file_interface) :
	system_interface(system_interface), file_interface(file_interface)
{}

bool SandboxWindow::Initialize(Rml::Context* context)
{
	Shutdown();

	meta_handler = Rml::MakeShared<XMLNodeHandlerSandboxMeta>();
	Rml::XMLParser::RegisterNodeHandler("meta", meta_handler);

	document = context->LoadDocument("sandbox/data/sandbox.rml");
	if (!document)
		return false;

	// We use 'rem' like a custom 'dp' for  the built-in styles, since 'dp' is set globally for the target document. We
	// fix this to the initial dp-ratio on document load, we won't get adaptive 'dp' like we would a normal document.
	document->SetProperty(Rml::PropertyId::FontSize, Rml::Property{document->GetContext()->GetDensityIndependentPixelRatio(), Rml::Unit::PX});

	auto GetFormControl = [this](const char* id) { return rmlui_dynamic_cast<Rml::ElementFormControl*>(document->GetElementById(id)); };

	el_rml_source = GetFormControl("sandbox_rml_source");
	el_rcss_source = GetFormControl("sandbox_rcss_source");
	el_file_path = GetFormControl("load_file_path");
	el_working_directory = GetFormControl("working_directory");
	el_themes = GetFormControl("active_themes");
	el_dp_ratio = GetFormControl("dp_ratio");
	el_inject_rcss = GetFormControl("inject_rcss");
	el_override_font_family = GetFormControl("override_font_family");
	el_target = document->GetElementById("sandbox_target");
	el_status = document->GetElementById("status");
	el_menu = document->GetElementById("menu");
	el_sources = document->GetElementById("sources");
	el_messages = document->GetElementById("messages");

	if (!el_rml_source || !el_rcss_source || !el_file_path || !el_working_directory || !el_themes || !el_dp_ratio || !el_inject_rcss ||
		!el_override_font_family || !el_target || !el_status || !el_menu || !el_sources || !el_messages)
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "Sandbox document is missing one or more of its required elements.");
		return false;
	}

	{
		Rml::String rml_rcss;
		if (!file_interface->LoadFile("assets/rml.rcss", rml_rcss))
			Rml::Log::Message(Rml::Log::LT_WARNING, "Could not load the basic RML style sheet, sandboxed RML fragments will be unstyled.");

		fragment_style_sheet = MakeStyleSheet(rml_rcss + g_sandbox_fragment_rcss, "sandbox://fragment_rcss");
	}

	el_rml_source->SetValue(g_default_rml_source);
	el_rcss_source->SetValue(g_default_rcss_source);

	LoadFromStorage();

	ApplyWorkingDirectory();
	ApplyActiveThemes();
	ApplyDpRatio();
	ApplyRcssSourceVisibility();

	for (Rml::Element* element : GetFormControls())
		element->AddEventListener(Rml::EventId::Change, this);

	document->AddEventListener(Rml::EventId::Click, this);
	document->AddEventListener(Rml::EventId::Keydown, this);

	ReloadSandboxDocument();

	document->Show(Rml::ModalFlag::None, Rml::FocusFlag::Document);

	return true;
}

void SandboxWindow::Shutdown()
{
	if (document)
		SaveToStorage();

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

	if (storage_dirty)
	{
		if (system_interface->GetElapsedTime() >= next_storage_save_time)
			SaveToStorage();
	}

	RefreshMessages();
}

void SandboxWindow::ProcessEvent(Rml::Event& event)
{
	Rml::Element* element = event.GetCurrentElement();

	switch (event.GetId())
	{
	case Rml::EventId::Keydown:
	{
		const Rml::Input::KeyIdentifier key_identifier = (Rml::Input::KeyIdentifier)event.GetParameter<int>("key_identifier", 0);
		const bool shift = event.GetParameter<int>("shift_key", 0) > 0;
		const bool ctrl = event.GetParameter<int>("ctrl_key", 0) > 0;
		const bool alt = event.GetParameter<int>("alt_key", 0) > 0;

		if (key_identifier == Rml::Input::KI_ESCAPE)
			Backend::RequestExit();
		else if (key_identifier == Rml::Input::KI_R && shift && ctrl && !alt)
			Initialize(document->GetContext());
	}
	break;
	case Rml::EventId::Click:
	{
		Rml::Element* target = event.GetTargetElement();
		if (target->GetId() == "load_file")
		{
			LoadDocumentFromFile();
		}
		else if (target->GetId() == "load_from_clipboard")
		{
			LoadDocumentFromClipboard();
		}
		else if (target->GetId() == "save_file")
		{
			for (const Rml::String& source : file_interface->GetLoadedFiles())
			{
				Rml::Log::Message(Rml::Log::LT_INFO, "Loaded file: %s", source.c_str());
			}
		}
	}
	break;
	case Rml::EventId::Change:
	{
		const bool submitted = event.GetParameter<bool>("linebreak", false);
		DirtyStorage();

		if (element == el_rml_source)
		{
			ReloadSandboxDocument();
		}
		else if (element == el_rcss_source)
		{
			ClearMessages();
			UpdateSandboxStylesheet();
		}
		else if (element == el_themes)
		{
			ClearMessages();
			ApplyActiveThemes();
			UpdateSandboxStylesheet();
		}
		else if (element == el_dp_ratio)
		{
			ApplyDpRatio();
		}
		else if (element == el_inject_rcss)
		{
			ClearMessages();
			ApplyRcssSourceVisibility();
			UpdateSandboxStylesheet();
		}
		else if (element == el_override_font_family)
		{
			ClearMessages();
			UpdateSandboxStylesheet();
		}
		else if (element == el_file_path && submitted)
		{
			LoadDocumentFromFile();
		}
		else if (element == el_working_directory)
		{
			ApplyWorkingDirectory();
			ReloadSandboxDocument();
		}
	}
	break;
	default: break;
	}
}

void SandboxWindow::ReloadSandboxDocument()
{
	ClearCaches();

	Rml::Context* context = document->GetContext();
	ClearMessages();
	file_interface->ClearLoadedFiles();

	if (sandbox_document)
	{
		el_target->RemoveChild(sandbox_document);
		sandbox_document = nullptr;
	}
	base_style_sheet.reset();

	const Rml::String rml_source = el_rml_source->GetValue();
	const bool is_complete_document = IsCompleteDocument(rml_source);

	Rml::ElementPtr new_element = nullptr;
	if (is_complete_document)
	{
		meta_handler->ClearMetaList();

		const Rml::String source_url = sandbox_source_url.empty() ? Rml::String("sandbox://rml") : sandbox_source_url;
		Rml::StreamMemory stream((const Rml::byte*)rml_source.data(), rml_source.size());
		stream.SetSourceURL(Rml::StringUtilities::Replace(source_url, ':', '|'));

		// Instance the document directly to avoid the document being formatted before we apply the proper styles.
		new_element = Rml::Factory::InstanceDocumentStream(context, &stream, context->GetDocumentsBaseTag(), nullptr);

		if (!new_element)
		{
			SetStatus("Failed to instance the sandboxed document.", true);
			return;
		}

		Rml::String new_dp_ratio;
		Rml::String new_active_themes;

		for (const auto& [name, content] : meta_handler->GetMetaList())
		{
			if (name == "density-independent-pixel-ratio")
				new_dp_ratio = content;
			else if (name == "active-themes")
				new_active_themes = content;
		}

		if (SetPlaceholderAndCompare(el_dp_ratio, new_dp_ratio))
			el_dp_ratio->DispatchEvent(Rml::EventId::Change, Rml::Dictionary{{"value", Rml::Variant{Rml::String()}}});
		if (SetPlaceholderAndCompare(el_themes, new_active_themes))
			el_themes->DispatchEvent(Rml::EventId::Change, Rml::Dictionary{{"value", Rml::Variant{Rml::String()}}});

		auto defaults = MakeStyleSheet(g_sandbox_document_rcss, "sandbox://document_rcss");
		if (const Rml::StyleSheetContainer* document_style_sheet =
				rmlui_static_cast<Rml::ElementDocument*>(new_element.get())->GetStyleSheetContainer())
			base_style_sheet = defaults->CombineStyleSheetContainer(*document_style_sheet);
		else
			base_style_sheet = std::move(defaults);
	}
	else
	{
		Rml::ElementDocument* document = context->CreateDocument();
		new_element = document->GetParentNode()->RemoveChild(document);
		base_style_sheet = fragment_style_sheet;
		new_element->SetInnerRML(rml_source);
	}

	sandbox_document = rmlui_static_cast<Rml::ElementDocument*>(new_element.get());
	el_target->AppendChild(std::move(new_element));

	UpdateSandboxStylesheet();

	sandbox_document->Show(Rml::ModalFlag::None, Rml::FocusFlag::None);
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
			"*:not(#rmlui-high-precedence1 #rmlui-high-precedence2 #rmlui-high-precedence3) { font-family: LatoLatin; }";
		auto user_style_sheet = MakeStyleSheet(override_font_family_rcss, "sandbox://override_font_family");
		new_style = new_style->CombineStyleSheetContainer(*user_style_sheet);
	}

	sandbox_document->SetStyleSheetContainer(new_style);
}

void SandboxWindow::LoadDocumentFromFile()
{
	ClearCaches();

	const Rml::String path = el_file_path->GetValue();
	if (path.empty())
	{
		SetStatus("Enter the path of a document to load.", true);
		return;
	}

	Rml::String working_directory_placeholder = GetDirectoryOf(path);
	SetPlaceholder(el_working_directory, working_directory_placeholder);

	file_interface->SetWorkingDirectory(GetValueOrPlaceholder(el_working_directory));

	Rml::String rml_source;
	if (!file_interface->LoadFile(path, rml_source))
	{
		SetStatus("Could not open '" + path + "'.", true);
		return;
	}

	SetChecked(el_inject_rcss, !IsCompleteDocument(rml_source));
	ApplyRcssSourceVisibility();

	sandbox_source_url = path;
	el_rml_source->SetValue(rml_source);
	DirtyStorage();

	SetStatus("Successfully loaded file.", false);
}

void SandboxWindow::LoadDocumentFromClipboard()
{
	ClearCaches();

	Rml::String rml_source;
	Rml::GetSystemInterface()->GetClipboardText(rml_source);

	if (rml_source.empty())
	{
		SetStatus("No clipboard text available.", true);
		return;
	}

	SetPlaceholder(el_working_directory, "");
	const Rml::String working_directory = GetValueOrPlaceholder(el_working_directory);
	file_interface->SetWorkingDirectory(working_directory);

	SetChecked(el_inject_rcss, !IsCompleteDocument(rml_source));
	ApplyRcssSourceVisibility();

	sandbox_source_url = working_directory + "./rml_pasted_document.rml";
	el_rml_source->SetValue(rml_source);
	DirtyStorage();

	SetStatus("Successfully loaded from clipboard.", false);
}

void SandboxWindow::RefreshMessages()
{
	Rml::Vector<SandboxSystemInterface::Message> messages = system_interface->PopMessages();

	for (const SandboxSystemInterface::Message& message : messages)
	{
		Rml::ElementPtr entry = document->CreateElement("div");
		entry->SetClass(GetLogClass(message.type), true);
		entry->SetInnerRML(Rml::StringUtilities::EncodeRml(message.text));
		el_messages->AppendChild(std::move(entry));
	}
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

Rml::ElementList SandboxWindow::GetFormControls()
{
	Rml::ElementList elements;
	for (Rml::Element* panel : {el_menu, el_sources})
		panel->QuerySelectorAll(elements, "input, textarea, select");

	elements.erase(std::remove_if(elements.begin(), elements.end(), [](Rml::Element* element) { return element->GetId().empty(); }), elements.end());

	return elements;
}

void SandboxWindow::ApplyWorkingDirectory()
{
	file_interface->SetWorkingDirectory(GetValueOrPlaceholder(el_working_directory));
}

void SandboxWindow::ApplyActiveThemes()
{
	Rml::Context* context = document->GetContext();

	Rml::StringList set_themes;
	Rml::StringUtilities::ExpandString(set_themes, GetValueOrPlaceholder(el_themes));

	const Rml::StringList active_themes = context->GetActiveThemes();
	for (const Rml::String& theme : active_themes)
	{
		if (std::find(set_themes.begin(), set_themes.end(), theme) == set_themes.end())
			context->ActivateTheme(theme, false);
	}
	for (const Rml::String& theme : set_themes)
		context->ActivateTheme(theme, true);
}

void SandboxWindow::ApplyDpRatio()
{
	const Rml::String value = GetValueOrPlaceholder(el_dp_ratio);
	float dp_ratio = 1.f;
	if (!value.empty() && Rml::TypeConverter<Rml::String, float>::Convert(value, dp_ratio))
		dp_ratio = Rml::Math::Clamp(dp_ratio, 0.5f, 4.f);

	document->GetContext()->SetDensityIndependentPixelRatio(Rml::Math::Clamp(dp_ratio, 0.5f, 4.f));
}

void SandboxWindow::ApplyRcssSourceVisibility()
{
	el_sources->SetClass("hide_rcss_source", !el_inject_rcss->HasAttribute("checked"));
}

void SandboxWindow::ClearMessages()
{
	system_interface->PopMessages();
	el_messages->SetInnerRML("");
}

void SandboxWindow::DirtyStorage()
{
	storage_dirty = true;
	next_storage_save_time = system_interface->GetElapsedTime() + g_options_save_delay;
}

void SandboxWindow::LoadFromStorage()
{
	const Rml::UnorderedMap<Rml::String, Rml::String> options = LoadOptions(g_options_path);

	auto GetIfOption = [&](const Rml::String& name) -> std::optional<Rml::String> {
		auto it = options.find(name);
		if (it != options.end())
			return it->second;
		return std::nullopt;
	};

	if (auto value = GetIfOption("source_url"))
		sandbox_source_url = *value;

	for (Rml::Element* element : GetFormControls())
	{
		const Rml::String& id = element->GetId();
		if (element == el_rml_source || element == el_rcss_source || id.empty())
			continue;

		if (IsCheckbox(element))
		{
			if (auto value = GetIfOption(id))
				SetChecked(element, Rml::FromString<bool>(*value));
		}
		else
		{
			if (auto value = GetIfOption(id))
				element->SetAttribute("value", *value);

			if (auto value = GetIfOption(id + ".placeholder"))
				element->SetAttribute("placeholder", *value);
		}
	}

	if (Rml::String source = LoadFile(g_source_rml_path); !source.empty())
		el_rml_source->SetValue(source);
	if (Rml::String source = LoadFile(g_source_rcss_path); !source.empty())
		el_rcss_source->SetValue(source);
}

void SandboxWindow::SaveToStorage()
{
	storage_dirty = false;

	Rml::UnorderedMap<Rml::String, Rml::String> options;
	options["source_url"] = sandbox_source_url;

	for (Rml::Element* element : GetFormControls())
	{
		const Rml::String& id = element->GetId();
		if (element == el_rml_source || element == el_rcss_source || id.empty())
			continue;

		if (IsCheckbox(element))
		{
			options[id] = Rml::ToString<bool>(element->HasAttribute("checked"));
		}
		else
		{
			options[id] = element->GetAttribute("value", Rml::String());
			if (element->HasAttribute("placeholder"))
				options[id + "placeholder"] = element->GetAttribute("placeholder", Rml::String());
		}
	}

	SaveOptions(g_options_path, options);

	SaveFile(g_source_rml_path, el_rml_source->GetValue());
	SaveFile(g_source_rcss_path, el_rcss_source->GetValue());
}
