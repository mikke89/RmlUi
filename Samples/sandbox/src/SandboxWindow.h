#pragma once

#include "SandboxFileInterface.h"
#include "SandboxSystemInterface.h"
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/Elements/ElementFormControl.h>
#include <RmlUi/Core/EventListener.h>

class XMLNodeHandlerSandboxMeta;

class SandboxWindow : public Rml::EventListener {
public:
	SandboxWindow(SandboxSystemInterface* logger, SandboxFileInterface* file_interface);

	bool Initialize(Rml::Context* context);
	void Shutdown();

	void Update();

	void ProcessEvent(Rml::Event& event) override;

private:
	void ReloadSandboxDocument();
	void UpdateSandboxStylesheet();

	void LoadDocumentFromFile();
	void LoadDocumentFromClipboard();
	void RefreshMessages();

	void SetStatus(const Rml::String& message, bool error);

	Rml::ElementList GetFormControls();

	void ApplyWorkingDirectory();
	void ApplyActiveThemes();
	void ApplyDpRatio();
	void ApplyRcssSourceVisibility();
	void ClearMessages();

	void DirtyStorage();
	void LoadFromStorage();
	void SaveToStorage();

	SandboxSystemInterface* system_interface;
	SandboxFileInterface* file_interface;

	Rml::ElementDocument* document = nullptr;
	Rml::ElementDocument* sandbox_document = nullptr;

	Rml::SharedPtr<Rml::StyleSheetContainer> base_style_sheet;
	Rml::SharedPtr<Rml::StyleSheetContainer> fragment_style_sheet;

	Rml::String sandbox_source_url;

	Rml::ElementFormControl* el_rml_source = nullptr;
	Rml::ElementFormControl* el_rcss_source = nullptr;
	Rml::ElementFormControl* el_file_path = nullptr;
	Rml::ElementFormControl* el_working_directory = nullptr;
	Rml::ElementFormControl* el_themes = nullptr;
	Rml::ElementFormControl* el_dp_ratio = nullptr;
	Rml::ElementFormControl* el_inject_rcss = nullptr;
	Rml::ElementFormControl* el_override_font_family = nullptr;
	Rml::Element* el_target = nullptr;
	Rml::Element* el_menu = nullptr;
	Rml::Element* el_sources = nullptr;
	Rml::Element* el_status = nullptr;
	Rml::Element* el_messages = nullptr;

	bool storage_dirty = false;
	double next_storage_save_time = 0;

	Rml::SharedPtr<XMLNodeHandlerSandboxMeta> meta_handler;
};
