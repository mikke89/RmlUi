#include "precompiled.h"
#include "LuaEventListenerInstancer.h"
#include "LuaEventListener.h"

namespace Rocket {
namespace Core {
namespace Lua {

/// Instance an event listener object.
/// @param value Value of the event.
/// @param element Element that triggers the events.
EventListener* LuaEventListenerInstancer::InstanceEventListener(const String& value, Element* element)
{
    return new LuaEventListener(value,element);
}

/// Releases this event listener instancer.
void LuaEventListenerInstancer::Release()
{
    delete this;
}

}
}
}