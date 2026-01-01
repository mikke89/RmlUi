#pragma once

#include <RmlUi/Core/EventListenerInstancer.h>

class EventListenerInstancer : public Rml::EventListenerInstancer {
public:
	EventListenerInstancer();
	virtual ~EventListenerInstancer();

	/// Instances a new event listener for Invaders.
	Rml::EventListener* InstanceEventListener(const Rml::String& value, Rml::Element* element) override;
};
