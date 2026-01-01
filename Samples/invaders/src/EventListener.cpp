#include "EventListener.h"
#include "EventManager.h"

EventListener::EventListener(const Rml::String& value) : value(value) {}

EventListener::~EventListener() {}

void EventListener::ProcessEvent(Rml::Event& event)
{
	// Sends the event value through to Invader's event processing system.
	EventManager::ProcessEvent(event, value);
}

void EventListener::OnDetach(Rml::Element* /*element*/)
{
	// Destroys the event listener.
	delete this;
}
