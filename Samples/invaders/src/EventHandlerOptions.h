#ifndef RMLUI_INVADERS_EVENTHANDLEROPTIONS_H
#define RMLUI_INVADERS_EVENTHANDLEROPTIONS_H

#include "EventHandler.h"

/**
    @author Peter Curry
 */

class EventHandlerOptions : public EventHandler {
public:
	EventHandlerOptions();
	virtual ~EventHandlerOptions();

	void ProcessEvent(Rml::Event& event, const Rml::String& value) override;
};

#endif
