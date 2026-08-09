#pragma once

#include "SandboxLogger.h"
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/Elements/ElementFormControl.h>
#include <RmlUi/Core/EventListener.h>

class SandboxWindow : public Rml::EventListener {
public:
	SandboxWindow(SandboxLogger* logger);

	bool Initialize(Rml::Context* context);
	void Shutdown();

	void Update();

	void ProcessEvent(Rml::Event& event) override;

	Rml::ElementDocument* GetDocument();

private:
	void SetSandboxStylesheet(const Rml::String& string);
	void SetSandboxBody(const Rml::String& string);

	SandboxLogger* logger;

	Rml::ElementDocument* document = nullptr;
	Rml::ElementDocument* iframe = nullptr;
	Rml::SharedPtr<Rml::StyleSheetContainer> rml_basic_style_sheet;

	Rml::ElementFormControl* el_rml_source = nullptr;
	Rml::ElementFormControl* el_rcss_source = nullptr;
	Rml::Element* el_target = nullptr;
};
