#ifndef RMLUI_INVADERS_EVENTHANDLERHIGHSCORE_H
#define RMLUI_INVADERS_EVENTHANDLERHIGHSCORE_H

#include "EventHandler.h"

/**
 */

class EventHandlerHighScore : public EventHandler {
public:
	EventHandlerHighScore();
	virtual ~EventHandlerHighScore();

	void ProcessEvent(Rml::Event& event, const Rml::String& value) override;
};

#endif
