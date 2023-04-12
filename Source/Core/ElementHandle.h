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

#ifndef RMLUI_CORE_ELEMENTHANDLE_H
#define RMLUI_CORE_ELEMENTHANDLE_H

#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/Header.h"

namespace Rml {

/**
    A derivation of an element for use as a mouse handle. A handle is designed to be instanced and attached as a non-
    DOM element to a window-style element, and listened to for movement events which can be responsed to to move or
    resize the element as appropriate.

    @author Peter Curry
 */

class RMLUICORE_API ElementHandle : public Element {
public:
	RMLUI_RTTI_DefineWithParent(ElementHandle, Element)

	ElementHandle(const String& tag);
	virtual ~ElementHandle();

protected:
	void OnAttributeChange(const ElementAttributes& changed_attributes) override;
	void ProcessDefaultAction(Event& event) override;

	Vector2f drag_start;
	Vector2f move_original_position;
	Vector2f size_original_size;

	Element* move_target;
	Element* size_target;

	bool initialised;
};

} // namespace Rml
#endif
