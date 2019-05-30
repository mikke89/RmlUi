/*
 * This source file is part of libRocket, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://www.librocket.com
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
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
#include "../../Include/Rocket/Core/Event.h"
#include "../../Include/Rocket/Core/EventInstancer.h"

namespace Rocket {
namespace Core {

Event::Event()
{
	phase = EventPhase::None;
	interruped = false;
	interruptible = false;
	current_element = NULL;
	target_element = NULL;
}

Event::Event(Element* _target_element, const String& _type, const Dictionary& _parameters, bool _interruptible) : type(_type), parameters(_parameters), target_element(_target_element), parameters_backup(_parameters), interruptible(_interruptible)
{
	phase = EventPhase::None;
	interruped = false;
	current_element = NULL;
}

Event::~Event()
{
}

void Event::SetCurrentElement(Element* element)
{
	ProjectMouse(element);
	current_element = element;
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
	return !interruped;
}

void Event::StopPropagation()
{
	// Set interruped to true if we can be interruped
	if (interruptible) 
	{
		interruped = true;
	}
}

const Dictionary* Event::GetParameters() const
{
	return &parameters;
}

void Event::OnReferenceDeactivate()
{
	instancer->ReleaseEvent(this);
}

void Event::ProjectMouse(Element* element)
{
	if (element)
	{
		Variant *old_mouse_x = GetIf(parameters_backup, "mouse_x");
		Variant *old_mouse_y = GetIf(parameters_backup, "mouse_y");
		if (!old_mouse_x || !old_mouse_y)
		{
			// This is not a mouse event.
			return;
		}

		Variant *mouse_x = GetIf(parameters, "mouse_x");
		Variant *mouse_y = GetIf(parameters, "mouse_y");
		if (!mouse_x || !mouse_y)
		{
			// This should not happen.
			return;
		}

		Vector2f old_mouse(
			old_mouse_x->Get< float >(),
			old_mouse_y->Get< float >()
		);
		Vector2f mouse = element->Project(old_mouse);

		mouse_x->Reset(mouse.x);
		mouse_y->Reset(mouse.y);
	}
	else
	{
		parameters = parameters_backup;
	}
}

}
}
