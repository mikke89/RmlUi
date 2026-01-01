#pragma once

#include "../../Include/RmlUi/Core/EventInstancer.h"
#include "../../Include/RmlUi/Core/Types.h"

namespace Rml {

/**
    Default instancer for instancing events.
 */

class EventInstancerDefault : public EventInstancer {
public:
	EventInstancerDefault();
	virtual ~EventInstancerDefault();

	/// Instance and event object
	/// @param[in] target Target element of this event.
	/// @param[in] id ID of this event.
	/// @param[in] type Name of this event type.
	/// @param[in] parameters Additional parameters for this event.
	/// @param[in] interruptible If the event propagation can be stopped.
	EventPtr InstanceEvent(Element* target, EventId id, const String& type, const Dictionary& parameters, bool interruptible) override;

	/// Releases an event instanced by this instancer.
	/// @param[in] event The event to release.
	void ReleaseEvent(Event* event) override;

	/// Releases this event instancer.
	void Release() override;
};

} // namespace Rml
