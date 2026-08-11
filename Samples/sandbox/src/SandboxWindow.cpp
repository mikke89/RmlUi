#include "SandboxWindow.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Elements/ElementFormControl.h>
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Core/StreamMemory.h>
#include <RmlUi/Core/StringUtilities.h>
#include <RmlUi/Core/StyleSheetContainer.h>
#include <RmlUi_Backend.h>

// Applied underneath the style sheet of any sandboxed document. Documents are normally laid out by the context, while
// here they are placed inside an element of the sandbox document, thus they need a block display to be formatted.
static const Rml::String g_sandbox_document_rcss = R"(
body { display: block; }
)";

static const Rml::String g_sandbox_fragment_rcss = R"(
body { display: block; top: 0; left: 0; right: 0; bottom: 0; overflow: hidden auto; background: #fff; }
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

static const Rml::String g_default_rml_source = "<p>Write your RML here, or load a document from file.</p>\n\n"
												"<!-- <img src=\"/assets/high_scores_alien_1.tga\"/> -->";

static const Rml::String g_default_rcss_source = "/* Write your RCSS here, it is applied on top of the document's own style sheet. */\n\n"
												 "/* body { color: #fea; background: #224; }\nimg { image-color: red; } */";

static const int g_max_log_entries = 200;

static Rml::SharedPtr<Rml::StyleSheetContainer> MakeStyleSheet(const Rml::String& content, const Rml::String& source_url)
{
	Rml::StreamMemory stream((const Rml::byte*)content.data(), content.size());
	stream.SetSourceURL(source_url);

	auto style_sheet = Rml::MakeShared<Rml::StyleSheetContainer>();
	style_sheet->LoadStyleSheetContainer(&stream);
	return style_sheet;
}

// Returns true if the source declares a complete document, as opposed to only the contents of a document body.
static bool IsCompleteDocument(const Rml::String& rml_source)
{
	const Rml::String source_begin = Rml::StringUtilities::ToLower(Rml::StringUtilities::StripWhitespace(rml_source).substr(0, 4));
	return source_begin == "<rml";
}

static Rml::String GetDirectoryOf(const Rml::String& path)
{
	const size_t i = path.find_last_of("/\\");
	return (i == Rml::String::npos ? Rml::String() : path.substr(0, i + 1));
}

static const char* GetLogClass(Rml::Log::Type type)
{
	switch (type)
	{
	case Rml::Log::LT_ERROR: return "error";
	case Rml::Log::LT_ASSERT: return "assert";
	case Rml::Log::LT_WARNING: return "warning";
	case Rml::Log::LT_INFO: return "info";
	case Rml::Log::LT_DEBUG: return "debug";
	default: break;
	}
	return "always";
}

SandboxWindow::SandboxWindow(SandboxLogger* logger, SandboxFileInterface* file_interface) : logger(logger), file_interface(file_interface) {}

bool SandboxWindow::Initialize(Rml::Context* context)
{
	using namespace Rml;

	Shutdown();

	document = context->LoadDocument(R"(C:\Projects\RmlUi\Samples\sandbox\data\sandbox.rml)");
	if (!document)
		return false;

	auto GetFormControl = [this](const char* id) { return rmlui_dynamic_cast<ElementFormControl*>(document->GetElementById(id)); };

	el_rml_source = GetFormControl("sandbox_rml_source");
	el_rcss_source = GetFormControl("sandbox_rcss_source");
	el_file_path = GetFormControl("load_file_path");
	el_working_directory = GetFormControl("working_directory");
	el_themes = GetFormControl("active_themes");
	el_dp_ratio = GetFormControl("dp_ratio");
	el_target = document->GetElementById("sandbox_target");
	el_status = document->GetElementById("load_file_status");
	el_log = document->GetElementById("log");

	if (!el_rml_source || !el_rcss_source || !el_file_path || !el_working_directory || !el_themes || !el_dp_ratio || !el_target || !el_status ||
		!el_log)
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

	for (ElementFormControl* element : {el_rml_source, el_rcss_source, el_file_path, el_working_directory, el_themes, el_dp_ratio})
		element->AddEventListener(EventId::Change, this);

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
	active_themes.clear();

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

	SubmitLogMessages();
}

void SandboxWindow::ProcessEvent(Rml::Event& event)
{
	using namespace Rml;

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
		else if (target->GetId() == "clear_log")
			el_log->SetInnerRML("");
	}
	break;
	case EventId::Change:
	{
		const String value = event.GetParameter<String>("value", "");
		const bool submitted = event.GetParameter<bool>("linebreak", false);

		if (element == el_rml_source)
			ReloadSandboxDocument();
		else if (element == el_rcss_source)
			SetSandboxStylesheet(value);
		else if (element == el_themes)
			SetActiveThemes(value);
		else if (element == el_dp_ratio)
			SetDensityIndependentPixelRatio(value);
		else if ((element == el_file_path || element == el_working_directory) && submitted)
			LoadDocumentFromFile();
	}
	break;
	default: break;
	}
}

Rml::ElementDocument* SandboxWindow::GetDocument()
{
	return document;
}

void SandboxWindow::ReloadSandboxDocument()
{
	using namespace Rml;

	Context* context = document->GetContext();
	if (!context)
		return;

	if (sandbox_document)
	{
		el_target->RemoveChild(sandbox_document);
		sandbox_document = nullptr;
	}
	base_style_sheet.reset();

	const String rml_source = el_rml_source->GetValue();
	const bool is_complete_document = IsCompleteDocument(rml_source);

	ElementDocument* new_document = nullptr;
	if (is_complete_document)
		new_document = context->LoadDocumentFromMemory(rml_source, sandbox_source_url.empty() ? String("sandbox://rml") : sandbox_source_url);
	else
		new_document = context->CreateDocument();

	if (!new_document)
	{
		SetStatus("Failed to instance the sandboxed document.", true);
		return;
	}

	// Move the document out of the context and into the sandbox target, so that it is displayed within our own document.
	el_target->AppendChild(new_document->GetParentNode()->RemoveChild(new_document));
	sandbox_document = new_document;

	if (is_complete_document)
	{
		// Place the document's own style sheet on top of the sandbox defaults, so that the document takes precedence.
		auto defaults = MakeStyleSheet(g_sandbox_document_rcss, "sandbox://document_rcss");
		if (const StyleSheetContainer* document_style_sheet = sandbox_document->GetStyleSheetContainer())
			base_style_sheet = defaults->CombineStyleSheetContainer(*document_style_sheet);
		else
			base_style_sheet = std::move(defaults);
	}
	else
	{
		base_style_sheet = fragment_style_sheet;
		sandbox_document->SetInnerRML(rml_source);
	}

	// Documents are hidden until shown, but we display it as a regular element rather than as a context document.
	sandbox_document->SetProperty(PropertyId::Visibility, Property(Style::Visibility::Visible));

	SetSandboxStylesheet(el_rcss_source->GetValue());
}

void SandboxWindow::SetSandboxStylesheet(const Rml::String& rcss)
{
	if (!sandbox_document || !base_style_sheet)
		return;

	auto user_style_sheet = MakeStyleSheet(rcss, "sandbox://rcss");
	sandbox_document->SetStyleSheetContainer(base_style_sheet->CombineStyleSheetContainer(*user_style_sheet));
}

void SandboxWindow::LoadDocumentFromFile()
{
	using namespace Rml;

	const String path = StringUtilities::Replace(StringUtilities::StripWhitespace(el_file_path->GetValue()), '\\', '\\');
	if (path.empty())
	{
		SetStatus("Enter the path of a document to load.", true);
		return;
	}

	String working_directory = StringUtilities::StripWhitespace(el_working_directory->GetValue());
	if (working_directory.empty())
	{
		working_directory = GetDirectoryOf(path);
		el_working_directory->SetValue(working_directory);
	}
	file_interface->SetWorkingDirectory(working_directory);

	String rml_source;
	if (!GetFileInterface()->LoadFile(path, rml_source))
	{
		SetStatus("Could not open '" + path + "'.", true);
		return;
	}

	sandbox_source_url = path;
	el_rml_source->SetValue(rml_source);
	ReloadSandboxDocument();

	if (!IsCompleteDocument(rml_source))
		SetStatus("Loaded as a body fragment, no 'rml' root tag found.", false);
	else
		SetStatus("Successfully loaded.", false);
}

void SandboxWindow::SetActiveThemes(const Rml::String& themes)
{
	Rml::Context* context = document->GetContext();
	if (!context)
		return;

	Rml::StringList new_themes;
	Rml::StringUtilities::ExpandString(new_themes, themes);

	for (const Rml::String& theme : active_themes)
		context->ActivateTheme(theme, false);

	active_themes.clear();

	for (const Rml::String& theme : new_themes)
	{
		if (theme.empty())
			continue;
		context->ActivateTheme(theme, true);
		active_themes.push_back(theme);
	}
}

void SandboxWindow::SetDensityIndependentPixelRatio(const Rml::String& value)
{
	Rml::Context* context = document->GetContext();

	float dp_ratio = 1.f;
	if (!value.empty() && !Rml::TypeConverter<Rml::String, float>::Convert(value, dp_ratio))
		return;

	context->SetDensityIndependentPixelRatio(Rml::Math::Clamp(dp_ratio, 0.5f, 5.f));
}

void SandboxWindow::SubmitLogMessages()
{
	Rml::Vector<SandboxLogger::Message> messages = logger->PopMessages();
	if (messages.empty())
		return;

	// Building the log entries may itself emit log messages, which would then feed back into the log indefinitely.
	logger->SetSuspended(true);

	for (const SandboxLogger::Message& message : messages)
	{
		Rml::ElementPtr entry = document->CreateElement("div");
		entry->SetClass(GetLogClass(message.type), true);
		entry->SetInnerRML(Rml::StringUtilities::EncodeRml(message.text));
		el_log->AppendChild(std::move(entry));
	}

	while (el_log->GetNumChildren() > g_max_log_entries)
		el_log->RemoveChild(el_log->GetFirstChild());

	document->UpdateDocument();
	el_log->SetScrollTop(el_log->GetScrollHeight());

	logger->SetSuspended(false);
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
