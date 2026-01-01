#pragma once

#include "EventHandler.h"

class EventHandlerStartGame : public EventHandler {
public:
	EventHandlerStartGame();
	virtual ~EventHandlerStartGame();

	void ProcessEvent(Rml::Event& event, const Rml::String& value) override;
};
