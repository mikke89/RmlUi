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

#include "../../Include/RmlUi/Core/Event.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/EventInstancer.h"

namespace Rml {

Event::Event() {}

Event::Event(Element* _target_element, EventId id, const String& type, const Dictionary& _parameters, bool interruptible) :
	parameters(_parameters), target_element(_target_element), type(type), id(id), interruptible(interruptible)
{
	const Variant* mouse_x = GetIf(parameters, "mouse_x");
	const Variant* mouse_y = GetIf(parameters, "mouse_y");
	if (mouse_x && mouse_y)
	{
		has_mouse_position = true;
		mouse_x->GetInto(mouse_screen_position.x);
		mouse_y->GetInto(mouse_screen_position.y);
	}
}

Event::~Event() {}

void Event::SetCurrentElement(Element* element)
{
	current_element = element;
	if (has_mouse_position)
	{
		ProjectMouse(element);
	}
}

Element* Event::GetCurrentElement() const
{
	return current_element;
}

Element* Event::GetTargetElement() const
{
	return target_element;
}

const String& Event::GetType() const
{
	return type;
}

bool Event::operator==(const String& _type) const
{
	return type == _type;
}

bool Event::operator==(EventId _id) const
{
	return id == _id;
}

void Event::SetPhase(EventPhase _phase)
{
	phase = _phase;
}

EventPhase Event::GetPhase() const
{
	return phase;
}

bool Event::IsPropagating() const
{
	return !interrupted;
}

void Event::StopPropagation()
{
	if (interruptible)
	{
		interrupted = true;
	}
}

bool Event::IsImmediatePropagating() const
{
	return !interrupted_immediate;
}

bool Event::IsInterruptible() const
{
	return interruptible;
}

void Event::StopImmediatePropagation()
{
	if (interruptible)
	{
		interrupted_immediate = true;
		interrupted = true;
	}
}

const Dictionary& Event::GetParameters() const
{
	return parameters;
}

Vector2f Event::GetUnprojectedMouseScreenPos() const
{
	return mouse_screen_position;
}

void Event::Release()
{
	if (instancer)
		instancer->ReleaseEvent(this);
	else
		Log::Message(Log::LT_WARNING, "Leak detected: Event %s not instanced via RmlUi Factory. Unable to release.", type.c_str());
}

EventId Event::GetId() const
{
	return id;
}

void Event::ProjectMouse(Element* element)
{
	if (!element)
	{
		parameters["mouse_x"] = mouse_screen_position.x;
		parameters["mouse_y"] = mouse_screen_position.y;
		return;
	}

	// Only need to project mouse position if element has a transform state
	if (element->GetTransformState())
	{
		// Project mouse from parent (previous 'mouse_x/y' property) to child (element)
		Variant* mouse_x = GetIf(parameters, "mouse_x");
		Variant* mouse_y = GetIf(parameters, "mouse_y");
		if (!mouse_x || !mouse_y)
		{
			RMLUI_ERROR;
			return;
		}

		Vector2f projected_position = mouse_screen_position;

		// Not sure how best to handle the case where the projection fails.
		if (element->Project(projected_position))
		{
			*mouse_x = projected_position.x;
			*mouse_y = projected_position.y;
		}
		else
			StopPropagation();
	}
}

} // namespace Rml
