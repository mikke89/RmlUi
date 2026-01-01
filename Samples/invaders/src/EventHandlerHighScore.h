#pragma once

#include "EventHandler.h"

class EventHandlerHighScore : public EventHandler {
public:
	EventHandlerHighScore();
	virtual ~EventHandlerHighScore();

	void ProcessEvent(Rml::Event& event, const Rml::String& value) override;
};
