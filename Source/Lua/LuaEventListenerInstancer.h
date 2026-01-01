#pragma once
#include <RmlUi/Core/EventListenerInstancer.h>

namespace Rml {
namespace Lua {

class LuaEventListenerInstancer : public ::Rml::EventListenerInstancer {
public:
	/// Instance an event listener object.
	/// @param value Value of the event.
	/// @param element Element that triggers the events.
	EventListener* InstanceEventListener(const String& value, Element* element) override;
};

} // namespace Lua
} // namespace Rml
