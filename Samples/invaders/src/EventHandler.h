#ifndef RMLUI_INVADERS_EVENTHANDLER_H
#define RMLUI_INVADERS_EVENTHANDLER_H

#include <RmlUi/Core/Types.h>

/**
    @author Peter Curry
 */

class EventHandler {
public:
	virtual ~EventHandler();
	virtual void ProcessEvent(Rml::Event& event, const Rml::String& value) = 0;
};

#endif
