#pragma once

#include "../../Include/RmlUi/Core/Event.h"
#include "../../Include/RmlUi/Core/Types.h"

namespace Rml {

class Element;
class EventListener;
struct CollectedListener;

struct EventListenerEntry {
	EventListenerEntry(const EventId id, EventListener* listener, const bool in_capture_phase) :
		id(id), in_capture_phase(in_capture_phase), listener(listener)
	{}

	EventId id;
	bool in_capture_phase;
	EventListener* listener;
};

/**
    The Event Dispatcher manages a list of event listeners and triggers the events via EventHandlers
    whenever requested.
*/

class EventDispatcher {
public:
	/// Constructor
	/// @param element Element this dispatcher acts on
	EventDispatcher(Element* element);

	/// Destructor
	~EventDispatcher();

	/// Attaches a new listener to the specified event name.
	/// @param[in] type Type of the event to attach to.
	/// @param[in] event_listener The event listener to be notified when the event fires.
	/// @param[in] in_capture_phase Should the listener be notified in the capture phase.
	void AttachEvent(EventId id, EventListener* event_listener, bool in_capture_phase);

	/// Detaches a listener from the specified event name
	/// @param[in] type Type of the event to attach to
	/// @param[in] event_listener The event listener to be notified when the event fires
	/// @param[in] in_capture_phase Should the listener be notified in the capture phase
	void DetachEvent(EventId id, EventListener* listener, bool in_capture_phase);

	/// Detaches all events from this dispatcher and all child dispatchers.
	void DetachAllEvents();

	/// Dispatches the specified event.
	/// @param[in] target_element The element to target
	/// @param[in] id The id of the event
	/// @param[in] type The type of the event
	/// @param[in] parameters The event parameters
	/// @param[in] interruptible Can the event propagation be stopped
	/// @param[in] bubbles True if the event should execute the bubble phase
	/// @param[in] default_action_phase The phases to execute default actions in
	/// @return True if the event was not consumed (ie, was prevented from propagating by an element), false if it was.
	static bool DispatchEvent(Element* target_element, EventId id, const String& type, const Dictionary& parameters, bool interruptible, bool bubbles,
		DefaultActionPhase default_action_phase);

	/// Returns event types with number of listeners for debugging.
	/// @return Summary of attached listeners.
	String ToString() const;

private:
	Element* element;

	// Listeners are sorted first by (id, phase) and then by the order in which the listener was inserted.
	// All listeners added are unique.
	typedef Vector<EventListenerEntry> Listeners;
	Listeners listeners;

	// Collect all the listeners from this dispatcher that are allowed to execute given the input arguments.
	void CollectListeners(int dom_distance_from_target, EventId event_id, EventPhase phases_to_execute, Vector<CollectedListener>& collect_listeners);
};

} // namespace Rml
