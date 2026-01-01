#pragma once

#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/EventListener.h>
#include <RmlUi/Core/EventListenerInstancer.h>

class DemoWindow;

class DemoEventListener : public Rml::EventListener {
public:
	DemoEventListener(const Rml::String& value, Rml::Element* element, DemoWindow* demo_window);

	void ProcessEvent(Rml::Event& event) override;

	void OnDetach(Rml::Element* element) override;

private:
	Rml::String value;
	Rml::Element* element;
	DemoWindow* demo_window;
};

class DemoEventListenerInstancer : public Rml::EventListenerInstancer {
public:
	DemoEventListenerInstancer(DemoWindow* demo_window);

	Rml::EventListener* InstanceEventListener(const Rml::String& value, Rml::Element* element) override;

private:
	DemoWindow* demo_window;
};
