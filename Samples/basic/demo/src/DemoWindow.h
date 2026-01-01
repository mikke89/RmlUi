#pragma once

#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/EventListener.h>

struct TweeningParameters {
	Rml::Tween::Type type = Rml::Tween::Linear;
	Rml::Tween::Direction direction = Rml::Tween::Out;
	float duration = 0.5f;
};

class DemoWindow : public Rml::EventListener {
public:
	bool Initialize(const Rml::String& title, Rml::Context* context);
	void Shutdown();

	void Update();

	void ProcessEvent(Rml::Event& event) override;

	Rml::ElementDocument* GetDocument();

	void SubmitForm(Rml::String in_submit_message);
	void SetSandboxStylesheet(const Rml::String& string);
	void SetSandboxBody(const Rml::String& string);

	TweeningParameters GetTweeningParameters() const;
	void SetTweeningParameters(TweeningParameters tweening_parameters);

private:
	Rml::ElementDocument* document = nullptr;
	Rml::ElementDocument* iframe = nullptr;
	Rml::Element* gauge = nullptr;
	Rml::Element* progress_horizontal = nullptr;
	Rml::SharedPtr<Rml::StyleSheetContainer> rml_basic_style_sheet;

	bool submitting = false;
	double submitting_start_time = 0;
	Rml::String submit_message;

	TweeningParameters tweening_parameters;
};
