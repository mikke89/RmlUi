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

#ifndef RMLUI_CORE_EVENTLISTENERINSTANCER_H
#define RMLUI_CORE_EVENTLISTENERINSTANCER_H

#include "Element.h"
#include "Header.h"
#include "Traits.h"
#include "Types.h"

namespace Rml {

class EventListener;

/**
    Abstract instancer interface for instancing event listeners. This is required to be overridden for scripting
    systems.

    @author Lloyd Weehuizen
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
#endif
