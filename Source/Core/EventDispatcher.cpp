/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019 The RmlUi Team, and contributors
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

#include "precompiled.h"
#include "EventDispatcher.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/Event.h"
#include "../../Include/RmlUi/Core/EventListener.h"
#include "../../Include/RmlUi/Core/Factory.h"
#include "EventSpecification.h"

namespace Rml {
namespace Core {


bool operator==(EventListenerEntry a, EventListenerEntry b) { return a.id == b.id && a.in_capture_phase == b.in_capture_phase && a.listener == b.listener; }
bool operator!=(EventListenerEntry a, EventListenerEntry b) { return !(a == b); }

struct CompareId {
	bool operator()(EventListenerEntry a, EventListenerEntry b) const { return a.id < b.id; }
}; 
struct CompareIdPhase {
	bool operator()(EventListenerEntry a, EventListenerEntry b) const { return std::tie(a.id, a.in_capture_phase) < std::tie(b.id, b.in_capture_phase); }
};



EventDispatcher::EventDispatcher(Element* _element)
{
	element = _element;
}

EventDispatcher::~EventDispatcher()
{
	// Detach from all event dispatchers
	for (const auto& event : listeners)
		event.listener->OnDetach(element);
}

void EventDispatcher::AttachEvent(EventId id, EventListener* listener, bool in_capture_phase)
{
	EventListenerEntry entry(id, listener, in_capture_phase);

	// The entries are sorted by (id,phase). Find the bounds of this sort, then find the entry.
	auto range = std::equal_range(listeners.begin(), listeners.end(), entry, CompareIdPhase());
	auto it = std::find(range.first, range.second, entry);

	if(it == range.second)
	{
		// No existing entry found, add it to the end of the (id, phase) range
		listeners.emplace(it, entry);
		listener->OnAttach(element);
	}
}


void EventDispatcher::DetachEvent(EventId id, EventListener* listener, bool in_capture_phase)
{
	EventListenerEntry entry(id, listener, in_capture_phase);
	
	// The entries are sorted by (id,phase). Find the bounds of this sort, then find the entry.
	// We could also just do a linear search over all the entries, which might be faster for low number of entries.
	auto range = std::equal_range(listeners.begin(), listeners.end(), entry, CompareIdPhase());
	auto it = std::find(range.first, range.second, entry);

	if (it != range.second)
	{
		// We found our listener, remove it
		listeners.erase(it);
		listener->OnDetach(element);
	}
}

// Detaches all events from this dispatcher and all child dispatchers.
void EventDispatcher::DetachAllEvents()
{
	for (const auto& event : listeners)
		event.listener->OnDetach(element);

	listeners.clear();

	for (int i = 0; i < element->GetNumChildren(true); ++i)
		element->GetChild(i)->GetEventDispatcher()->DetachAllEvents();
}

bool EventDispatcher::DispatchEvent(Element* target_element, EventId id, const String& type, const Dictionary& parameters, bool interruptible, bool bubbles, DefaultActionPhase default_action_phase)
{
	EventPtr event = Factory::InstanceEvent(target_element, id, type, parameters, interruptible);
	if (!event)
		return false;

	// Build the element traversal from the tree
	typedef std::vector<Element*> Elements;
	Elements elements;

	Element* walk_element = target_element->GetParentNode();
	while (walk_element) 
	{
		elements.push_back(walk_element);
		walk_element = walk_element->GetParentNode();
	}

	event->SetPhase(EventPhase::Capture);
	// Capture phase - root to target (only triggers event listeners that are registered with capture phase)
	// Note: We walk elements in REVERSE as they're placed in the list from the elements parent to the root
	for (int i = (int)elements.size() - 1; i >= 0 && event->IsPropagating(); i--) 
	{
		EventDispatcher* dispatcher = elements[i]->GetEventDispatcher();
		event->SetCurrentElement(elements[i]);
		dispatcher->TriggerEvents(*event, default_action_phase);
	}

	// Target phase - direct at the target
	if (event->IsPropagating()) 
	{
		event->SetPhase(EventPhase::Target);
		event->SetCurrentElement(target_element);
		target_element->GetEventDispatcher()->TriggerEvents(*event, default_action_phase);
	}

	// Bubble phase - target to root (normal event bindings)
	if (bubbles && event->IsPropagating())
	{
		event->SetPhase(EventPhase::Bubble);
		for (size_t i = 0; i < elements.size() && event->IsPropagating(); i++) 
		{
			EventDispatcher* dispatcher = elements[i]->GetEventDispatcher();
			event->SetCurrentElement(elements[i]);
			dispatcher->TriggerEvents(*event, default_action_phase);
		}
	}

	bool propagating = event->IsPropagating();

	return propagating;
}

String EventDispatcher::ToString() const
{
	String result;

	if (listeners.empty())
		return result;

	auto add_to_result = [&result](EventId id, int count) {
		const EventSpecification& specification = EventSpecificationInterface::Get(id);
		result += CreateString(specification.type.size() + 32, "%s (%d), ", specification.type.c_str(), count);
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

void EventDispatcher::TriggerEvents(Event& event, DefaultActionPhase default_action_phase)
{
	const EventPhase phase = event.GetPhase();

	// Find the range of entries with matching id and phase, given that listeners are sorted by (id,phase).
	// In the case of target phase we will match any listener phase.
	Listeners::iterator begin, end;
	if (phase == EventPhase::Capture)
		std::tie(begin, end) = std::equal_range(listeners.begin(), listeners.end(), EventListenerEntry(event.GetId(), nullptr, true), CompareIdPhase());
	else if (phase == EventPhase::Target)
		std::tie(begin, end) = std::equal_range(listeners.begin(), listeners.end(), EventListenerEntry(event.GetId(), nullptr, false), CompareId());
	else if (phase == EventPhase::Bubble)
		std::tie(begin, end) = std::equal_range(listeners.begin(), listeners.end(), EventListenerEntry(event.GetId(), nullptr, false), CompareIdPhase());

	// Copy the range in case the original list of listeners get modified during ProcessEvent.
	const Listeners listeners_range(begin, end);

	for(const EventListenerEntry& entry : listeners_range)
	{
		entry.listener->ProcessEvent(event);
		
		if (!event.IsImmediatePropagating())
			break;
	}

	const bool do_default_action = ((unsigned int)phase & (unsigned int)default_action_phase);

	// Do the default action unless we have been cancelled.
	if (do_default_action && event.IsPropagating())
	{
		element->ProcessDefaultAction(event);
	}
}


void EventDispatcher::AddEvents(std::vector<ListenerDesc>& add_listeners, std::vector<ObserverPtr<Element>>& default_action_elements, const EventId event_id, const EventPhase phase, DefaultActionPhase default_action_phase)
{
	// Find the range of entries with matching id and phase, given that listeners are sorted by (id,phase).
	// In the case of target phase we will match any listener phase.
	Listeners::iterator begin, end;
	if (phase == EventPhase::Capture)
		std::tie(begin, end) = std::equal_range(listeners.begin(), listeners.end(), EventListenerEntry(event_id, nullptr, true), CompareIdPhase());
	else if (phase == EventPhase::Target)
		std::tie(begin, end) = std::equal_range(listeners.begin(), listeners.end(), EventListenerEntry(event_id, nullptr, false), CompareId());
	else if (phase == EventPhase::Bubble)
		std::tie(begin, end) = std::equal_range(listeners.begin(), listeners.end(), EventListenerEntry(event_id, nullptr, false), CompareIdPhase());

	// Copy the range in case the original list of listeners get modified during ProcessEvent.
	const Listeners listeners_range(begin, end);

	for (auto it = begin; it != end; ++it)
	{
		add_listeners.push_back({ element->GetObserverPtr(), it->listener->GetObserverPtr(), phase });
	}

	const bool do_default_action = ((unsigned int)phase & (unsigned int)default_action_phase);

	// Do the default action unless we have been cancelled.
	if (do_default_action)
	{
		default_action_elements.push_back(element->GetObserverPtr());
	}
}

bool EventDispatcher::TrueDispatchEvent(Element* target_element, EventId id, const String& type, const Dictionary& parameters, bool interruptible, bool bubbles, DefaultActionPhase default_action_phase)
{
	std::vector<ListenerDesc> listeners;
	std::vector<ObserverPtr<Element>> default_action_elements;



	// Build the element traversal from the tree
	typedef std::vector<Element*> Elements;
	Elements elements;

	Element* walk_element = target_element->GetParentNode();
	while (walk_element)
	{
		elements.push_back(walk_element);
		walk_element = walk_element->GetParentNode();
	}

	// Capture phase - root to target (only triggers event listeners that are registered with capture phase)
	// Note: We walk elements in REVERSE as they're placed in the list from the elements parent to the root
	for (int i = (int)elements.size() - 1; i >= 0; i--)
	{
		EventDispatcher* dispatcher = elements[i]->GetEventDispatcher();
		dispatcher->AddEvents(listeners, default_action_elements, id, EventPhase::Capture, default_action_phase);
	}

	target_element->GetEventDispatcher()->AddEvents(listeners, default_action_elements, id, EventPhase::Target, default_action_phase);

	// Bubble phase - target to root (normal event bindings)
	if (bubbles)
	{
		for (Element* el : elements)
		{
			EventDispatcher* dispatcher = el->GetEventDispatcher();
			dispatcher->AddEvents(listeners, default_action_elements, id, EventPhase::Bubble, default_action_phase);
		}
	}

	if (listeners.empty() && default_action_elements.empty())
		return true;

	// Instance event
	EventPtr event = Factory::InstanceEvent(target_element, id, type, parameters, interruptible);
	if (!event)
		return false;

	Element* previous_element = nullptr;

	// Process event in each listener
	for (auto& listener_desc : listeners)
	{
		auto element = listener_desc.element.get();
		auto listener = listener_desc.listener.get();

		if (previous_element != element)
		{
			event->SetCurrentElement(element);
			if (!event->IsPropagating())
				break;
			previous_element = element;
		}

		if (element && listener)
		{
			event->SetPhase(listener_desc.phase);
			listener->ProcessEvent(*event);
		}

		if (!event->IsImmediatePropagating())
			break;
	}

	// Process default actions
	for (auto& element_observer : default_action_elements)
	{
		if (!event->IsPropagating())
			break;

		auto element = element_observer.get();

		if (element)
		{
			event->SetPhase(element == target_element ? EventPhase::Target : EventPhase::Bubble);
			event->SetCurrentElement(element);
			element->ProcessDefaultAction(*event);
		}
	}

	bool propagating = event->IsPropagating();

	return propagating;
}

}
}
