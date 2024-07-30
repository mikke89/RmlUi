/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019-2023 The RmlUi Team, and contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "EventDispatcher.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/Event.h"
#include "../../Include/RmlUi/Core/EventListener.h"
#include "../../Include/RmlUi/Core/Factory.h"
#include "EventSpecification.h"
#include <algorithm>
#include <limits>

namespace Rml {

bool operator==(EventListenerEntry a, EventListenerEntry b)
{
	return a.id == b.id && a.in_capture_phase == b.in_capture_phase && a.listener == b.listener;
}
bool operator!=(EventListenerEntry a, EventListenerEntry b)
{
	return !(a == b);
}

struct CompareId {
	bool operator()(EventListenerEntry a, EventListenerEntry b) const { return a.id < b.id; }
};
struct CompareIdPhase {
	bool operator()(EventListenerEntry a, EventListenerEntry b) const
	{
		return std::tie(a.id, a.in_capture_phase) < std::tie(b.id, b.in_capture_phase);
	}
};

EventDispatcher::EventDispatcher(Element* _element) : element(_element) {}

EventDispatcher::~EventDispatcher()
{
	// Detach from all event dispatchers
	for (const auto& event : listeners)
		event.listener->OnDetach(element);
}

void EventDispatcher::AttachEvent(const EventId id, EventListener* listener, const bool in_capture_phase)
{
	const EventListenerEntry entry(id, listener, in_capture_phase);

	// The entries are sorted by (id,phase). Find the bounds of this sort, then find the entry.
	const auto range = std::equal_range(listeners.cbegin(), listeners.cend(), entry, CompareIdPhase());
	const auto matching_entry_it = std::find(range.first, range.second, entry);
	if (matching_entry_it == range.second)
	{
		listeners.emplace(range.second, entry);
		listener->OnAttach(element);
	}
}

void EventDispatcher::DetachEvent(const EventId id, EventListener* listener, const bool in_capture_phase)
{
	const auto listenerIt = std::find(listeners.cbegin(), listeners.cend(), EventListenerEntry(id, listener, in_capture_phase));
	if (listenerIt != listeners.cend())
	{
		listeners.erase(listenerIt);
		listener->OnDetach(element);
	}
}

void EventDispatcher::DetachAllEvents()
{
	for (const auto& event : listeners)
		event.listener->OnDetach(element);

	listeners.clear();

	for (int i = 0; i < element->GetNumChildren(true); ++i)
		element->GetChild(i)->GetEventDispatcher()->DetachAllEvents();
}

/*
    CollectedListener

    When dispatching an event we collect all possible event listeners to execute.
    They are stored in observer pointers, so that we can safely check if they have been destroyed since the previous listener execution.
*/
struct CollectedListener {
	CollectedListener(Element* _element, EventListener* _listener, int dom_distance_from_target, bool in_capture_phase) :
		element(_element->GetObserverPtr()), listener(_listener->GetObserverPtr())
	{
		sort = dom_distance_from_target * (in_capture_phase ? -1 : 1);
	}

	// The sort value is determined by the distance of the element to the target element in the DOM.
	// Capture phase is given negative values.
	int sort = 0;

	ObserverPtr<Element> element;
	ObserverPtr<EventListener> listener;

	// Default actions are returned by EventPhase::None.
	EventPhase GetPhase() const { return sort < 0 ? EventPhase::Capture : (sort == 0 ? EventPhase::Target : EventPhase::Bubble); }

	bool operator<(const CollectedListener& other) const { return sort < other.sort; }
};

bool EventDispatcher::DispatchEvent(Element* target_element, const EventId id, const String& type, const Dictionary& parameters,
	const bool interruptible, const bool bubbles, const DefaultActionPhase default_action_phase)
{
	RMLUI_ASSERTMSG(!((int)default_action_phase & (int)EventPhase::Capture),
		"We assume here that the default action phases cannot include capture phase.");

	Vector<CollectedListener> listeners;
	Vector<ObserverPtr<Element>> default_action_elements;

	const EventPhase phases_to_execute = EventPhase((int)EventPhase::Capture | (int)EventPhase::Target | (bubbles ? (int)EventPhase::Bubble : 0));

	// Walk the DOM tree from target to root, collecting all possible listeners and elements with default actions in the process.
	int dom_distance_from_target = 0;
	Element* walk_element = target_element;
	while (walk_element)
	{
		EventDispatcher* dispatcher = walk_element->GetEventDispatcher();
		dispatcher->CollectListeners(dom_distance_from_target, id, phases_to_execute, listeners);

		if (dom_distance_from_target == 0)
		{
			if ((int)default_action_phase & (int)EventPhase::Target)
				default_action_elements.push_back(walk_element->GetObserverPtr());
		}
		else if ((int)default_action_phase & (int)EventPhase::Bubble)
		{
			default_action_elements.push_back(walk_element->GetObserverPtr());
		}

		walk_element = walk_element->GetParentNode();
		dom_distance_from_target += 1;
	}

	if (listeners.empty() && default_action_elements.empty())
		return true;

	// Use stable_sort so that the order of the listeners in a given element is maintained.
	std::stable_sort(listeners.begin(), listeners.end());

	// Instance event
	EventPtr event = Factory::InstanceEvent(target_element, id, type, parameters, interruptible);
	if (!event)
		return false;

	auto previous_sort_value = std::numeric_limits<int>::max();

	// Process the event in each listener.
	for (const auto& listener_desc : listeners)
	{
		Element* element = listener_desc.element.get();
		EventListener* listener = listener_desc.listener.get();

		if (listener_desc.sort != previous_sort_value)
		{
			// New sort values represent a new level in the DOM, thus, set the new element and possibly new phase.
			if (!event->IsPropagating())
				break;
			event->SetCurrentElement(element);
			event->SetPhase(listener_desc.GetPhase());
			previous_sort_value = listener_desc.sort;
		}

		// We only submit the event if both the current element and listener are still alive.
		if (element && listener)
		{
			listener->ProcessEvent(*event);
		}

		if (!event->IsImmediatePropagating())
			break;
	}

	// Process the default actions.
	for (auto& element_ptr : default_action_elements)
	{
		if (!event->IsPropagating())
			break;

		if (Element* element = element_ptr.get())
		{
			event->SetCurrentElement(element);
			event->SetPhase(element == target_element ? EventPhase::Target : EventPhase::Bubble);
			element->ProcessDefaultAction(*event);
		}
	}

	bool propagating = event->IsPropagating();

	return propagating;
}

void EventDispatcher::CollectListeners(int dom_distance_from_target, const EventId event_id, const EventPhase event_executes_in_phases,
	Vector<CollectedListener>& collect_listeners)
{
	// Find all the entries with a matching id, given that listeners are sorted by id first.
	Listeners::iterator begin, end;
	std::tie(begin, end) = std::equal_range(listeners.begin(), listeners.end(), EventListenerEntry(event_id, nullptr, false), CompareId());

	const bool in_target_phase = (dom_distance_from_target == 0);

	if (in_target_phase)
	{
		// Listeners always attach to target phase, but make sure the event can actually execute in target phase.
		if ((int)event_executes_in_phases & (int)EventPhase::Target)
		{
			for (auto it = begin; it != end; ++it)
				collect_listeners.emplace_back(element, it->listener, dom_distance_from_target, false);
		}
	}
	else
	{
		// Iterate through all the listeners and collect those matching the event execution phase.
		for (auto it = begin; it != end; ++it)
		{
			// Listeners will either attach to capture or bubble phase, make sure the event can execute in the same phase.
			const EventPhase listener_executes_in_phase = (it->in_capture_phase ? EventPhase::Capture : EventPhase::Bubble);
			if ((int)event_executes_in_phases & (int)listener_executes_in_phase)
				collect_listeners.emplace_back(element, it->listener, dom_distance_from_target, it->in_capture_phase);
		}
	}
}

String EventDispatcher::ToString() const
{
	String result;

	if (listeners.empty())
		return result;

	auto add_to_result = [&result](EventId id, int count) {
		const EventSpecification& specification = EventSpecificationInterface::Get(id);
		result += CreateString("%s (%d), ", specification.type.c_str(), count);
	};

	EventId previous_id = listeners[0].id;
	int count = 0;
	for (const auto& listener : listeners)
	{
		if (listener.id != previous_id)
		{
			add_to_result(previous_id, count);
			previous_id = listener.id;
			count = 0;
		}
		count++;
	}

	if (count > 0)
		add_to_result(previous_id, count);

	if (result.size() > 2)
		result.resize(result.size() - 2);

	return result;
}

} // namespace Rml
