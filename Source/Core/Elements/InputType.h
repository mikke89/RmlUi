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

#ifndef RMLUI_CORE_ELEMENTS_INPUTTYPE_H
#define RMLUI_CORE_ELEMENTS_INPUTTYPE_H

#include "../../../Include/RmlUi/Core/Event.h"
#include "../../../Include/RmlUi/Core/Types.h"

namespace Rml {

class ElementFormControlInput;

/**
    An interface for a input type handler used by ElementFormControlInput. A concrete InputType object handles the
    functionality of an input element.

    @author Peter Curry
 */

class InputType {
public:
	InputType(ElementFormControlInput* element);
	virtual ~InputType();

	/// Returns a string representation of the current value of the form control.
	/// @return The value of the form control.
	virtual String GetValue() const;
	/// Returns if this value should be submitted with the form.
	/// @return True if the form control is to be submitted, false otherwise.
	virtual bool IsSubmitted();

	/// Called every update from the host element.
	virtual void OnUpdate();

	/// Called every render from the host element.
	virtual void OnRender();

	/// Called every time the host element's size changes.
	virtual void OnResize();

	/// Called every time the host element is layed out.
	virtual void OnLayout();

	/// Checks for necessary functional changes in the control as a result of changed attributes.
	/// @param[in] changed_attributes The list of changed attributes.
	/// @return True if no layout is required, false if the layout needs to be dirtied.
	virtual bool OnAttributeChange(const ElementAttributes& changed_attributes);
	/// Called when properties on the control are changed.
	/// @param[in] changed_properties The properties changed on the element.
	virtual void OnPropertyChange(const PropertyIdSet& changed_properties);

	/// Called when the element is added into a hierarchy.
	virtual void OnChildAdd();
	/// Called when the element is removed from a hierarchy.
	virtual void OnChildRemove();

	/// Checks for necessary functional changes in the control as a result of the event.
	/// @param[in] event The event to process.
	virtual void ProcessDefaultAction(Event& event) = 0;

	/// Sizes the dimensions to the element's inherent size.
	virtual bool GetIntrinsicDimensions(Vector2f& dimensions, float& ratio) = 0;

	/// Selects all text.
	virtual void Select();
	/// Selects the text in the given character range.
	virtual void SetSelectionRange(int selection_start, int selection_end);
	/// Retrieves the selection range and text.
	virtual void GetSelection(int* selection_start, int* selection_end, String* selected_text) const;

	/// Sets visual feedback for the IME composition in the given character range.
	virtual void SetCompositionRange(int range_start, int range_end);

protected:
	ElementFormControlInput* element;
};

} // namespace Rml
#endif
