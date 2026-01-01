#pragma once

#include "Element.h"
#include "Header.h"
#include "Traits.h"
#include "Types.h"

namespace Rml {

class EventListener;

/**
    Abstract instancer interface for instancing event listeners. This is required to be overridden for scripting
    systems.
 */

class RMLUICORE_API EventListenerInstancer {
public:
	virtual ~EventListenerInstancer();

	/// Instance an event listener object.
	/// @param value Value of the inline event.
	/// @param element Element that triggers this call to the instancer.
	/// @return An event listener which will be attached to the element.
	/// @lifetime The returned event listener must be kept alive until the call to `EventListener::OnDetach` on the
	///           returned listener, and then cleaned up by the user. The detach function is called when the listener
	///           is detached manually, or automatically when the element is destroyed.
	virtual EventListener* InstanceEventListener(const String& value, Element* element) = 0;
};

} // namespace Rml
