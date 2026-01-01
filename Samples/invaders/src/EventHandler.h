#pragma once

#include <RmlUi/Core/Types.h>

class EventHandler {
public:
	virtual ~EventHandler();
	virtual void ProcessEvent(Rml::Event& event, const Rml::String& value) = 0;
};
