#ifndef RMLUI_INVADERS_EVENTHANDLERSTARTGAME_H
#define RMLUI_INVADERS_EVENTHANDLERSTARTGAME_H

#include "EventHandler.h"

/**
    @author Peter Curry
 */

class EventHandlerStartGame : public EventHandler {
public:
	EventHandlerStartGame();
	virtual ~EventHandlerStartGame();

	void ProcessEvent(Rml::Event& event, const Rml::String& value) override;
};

#endif
