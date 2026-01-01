#pragma once

#include "EventHandler.h"

class EventHandlerOptions : public EventHandler {
public:
	EventHandlerOptions();
	virtual ~EventHandlerOptions();

	void ProcessEvent(Rml::Event& event, const Rml::String& value) override;
};
