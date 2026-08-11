#pragma once

#include "SandboxFileInterface.h"
#include "SandboxLogger.h"
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/Elements/ElementFormControl.h>
#include <RmlUi/Core/EventListener.h>

class SandboxWindow : public Rml::EventListener {
public:
	SandboxWindow(SandboxLogger* logger, SandboxFileInterface* file_interface);

	bool Initialize(Rml::Context* context);
	void Shutdown();

	void Update();

	void ProcessEvent(Rml::Event& event) override;

	Rml::ElementDocument* GetDocument();

private:
	/// Replaces the sandboxed document with one built from the current RML source. The source is loaded as a complete
	/// document when it declares an 'rml' root tag, otherwise it is treated as the contents of a document body.
	void ReloadSandboxDocument();
	/// Applies the RCSS source on top of the sandboxed document's own style sheet.
	void SetSandboxStylesheet(const Rml::String& rcss);

	/// Reads the document at the given path into the RML source, and displays it.
	void LoadDocumentFromFile();
	void SetActiveThemes(const Rml::String& themes);
	void SetDensityIndependentPixelRatio(const Rml::String& value);
	void SubmitLogMessages();

	void SetStatus(const Rml::String& message, bool error);

	SandboxLogger* logger;
	SandboxFileInterface* file_interface;

	Rml::ElementDocument* document = nullptr;
	Rml::ElementDocument* sandbox_document = nullptr;

	/// The style sheet the sandboxed document brings along, before the RCSS source is applied on top of it.
	Rml::SharedPtr<Rml::StyleSheetContainer> base_style_sheet;
	/// Style sheet applied to documents built from an RML body fragment.
	Rml::SharedPtr<Rml::StyleSheetContainer> fragment_style_sheet;

	/// Source URL of the sandboxed document, used to resolve any relative paths declared within it.
	Rml::String sandbox_source_url;

	Rml::StringList active_themes;

	Rml::ElementFormControl* el_rml_source = nullptr;
	Rml::ElementFormControl* el_rcss_source = nullptr;
	Rml::ElementFormControl* el_file_path = nullptr;
	Rml::ElementFormControl* el_working_directory = nullptr;
	Rml::ElementFormControl* el_themes = nullptr;
	Rml::ElementFormControl* el_dp_ratio = nullptr;
	Rml::ElementFormControl* el_inject_rcss = nullptr;
	Rml::Element* el_target = nullptr;
	Rml::Element* el_sources = nullptr;
	Rml::Element* el_status = nullptr;
	Rml::Element* el_log = nullptr;
};
