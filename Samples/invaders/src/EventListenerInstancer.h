#ifndef RMLUI_INVADERS_EVENTLISTENERINSTANCER_H
#define RMLUI_INVADERS_EVENTLISTENERINSTANCER_H

#include <RmlUi/Core/EventListenerInstancer.h>

/**
    @author Peter Curry
 */

class EventListenerInstancer : public Rml::EventListenerInstancer {
public:
	EventListenerInstancer();
	virtual ~EventListenerInstancer();

	/// Instances a new event listener for Invaders.
	Rml::EventListener* InstanceEventListener(const Rml::String& value, Rml::Element* element) override;
};

#endif
