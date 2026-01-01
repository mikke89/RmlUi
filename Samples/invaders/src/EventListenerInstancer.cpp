#include "EventListenerInstancer.h"
#include "EventListener.h"

EventListenerInstancer::EventListenerInstancer() {}

EventListenerInstancer::~EventListenerInstancer() {}

Rml::EventListener* EventListenerInstancer::InstanceEventListener(const Rml::String& value, Rml::Element* /*element*/)
{
	// Instances a new event handler for Invaders.
	return new EventListener(value);
}
