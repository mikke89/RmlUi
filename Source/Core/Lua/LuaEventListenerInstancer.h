#ifndef ROCKETCORELUALUAEVENTLISTENERINSTANCER_H
#define ROCKETCORELUALUAEVENTLISTENERINSTANCER_H
#include <Rocket/Core/EventListenerInstancer.h>

namespace Rocket {
namespace Core {
namespace Lua {

class LuaEventListenerInstancer : public EventListenerInstancer
{
public:
    /// Instance an event listener object.
	/// @param value Value of the event.
	/// @param element Element that triggers the events.
	virtual EventListener* InstanceEventListener(const String& value, Element* element);

	/// Releases this event listener instancer.
	virtual void Release();
};

}
}
}
#endif