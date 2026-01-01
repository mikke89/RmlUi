#pragma once

#include "Header.h"
#include "Traits.h"
#include "Types.h"

namespace Rml {

class Element;
class Event;

/**
    Abstract instancer interface for instancing events. This is required to be overridden for scripting systems.
 */

class RMLUICORE_API EventInstancer : public Releasable {
public:
	virtual ~EventInstancer();

	/// Instance an event object.
	/// @param[in] target Target element of this event.
	/// @param[in] id ID of this event.
	/// @param[in] type Name of this event type.
	/// @param[in] parameters Additional parameters for this event.
	/// @param[in] interruptible If the event propagation can be stopped.
	virtual EventPtr InstanceEvent(Element* target, EventId id, const String& type, const Dictionary& parameters, bool interruptible) = 0;

	/// Releases an event instanced by this instancer.
	/// @param[in] event The event to release.
	virtual void ReleaseEvent(Event* event) = 0;
};

} // namespace Rml
