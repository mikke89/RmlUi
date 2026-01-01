#ifndef RMLUI_INVADERS_EVENTLISTENER_H
#define RMLUI_INVADERS_EVENTLISTENER_H

#include <RmlUi/Core/EventListener.h>

/**
    @author Peter Curry
 */

class EventListener : public Rml::EventListener {
public:
	EventListener(const Rml::String& value);
	virtual ~EventListener();

	/// Sends the event value through to Invader's event processing system.
	void ProcessEvent(Rml::Event& event) override;

	/// Destroys the event.
	void OnDetach(Rml::Element* element) override;

private:
	Rml::String value;
};

#endif
