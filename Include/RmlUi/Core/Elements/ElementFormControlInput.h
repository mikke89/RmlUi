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

#ifndef RMLUI_CORE_ELEMENTS_ELEMENTFORMCONTROLINPUT_H
#define RMLUI_CORE_ELEMENTS_ELEMENTFORMCONTROLINPUT_H

#include "../Header.h"
#include "ElementFormControl.h"

namespace Rml {

class InputType;

/**
    A form control for the generic input element. All functionality is handled through an input type interface.

    @author Peter Curry
 */

class RMLUICORE_API ElementFormControlInput : public ElementFormControl {
public:
	RMLUI_RTTI_DefineWithParent(ElementFormControlInput, ElementFormControl)

	/// Constructs a new ElementFormControlInput. This should not be called directly; use the
	/// Factory instead.
	/// @param[in] tag The tag the element was declared as in RML.
	ElementFormControlInput(const String& tag);
	virtual ~ElementFormControlInput();

	/// Returns a string representation of the current value of the form control.
	/// @return The value of the form control.
	String GetValue() const override;
	/// Sets the current value of the form control.
	/// @param value[in] The new value of the form control.
	void SetValue(const String& value) override;
	/// Returns if this value's type should be submitted with the form.
	/// @return True if the form control is to be submitted, false otherwise.
	bool IsSubmitted() override;

	/// Selects all text.
	/// @note Only applies to text and password input types.
	void Select();
	/// Selects the text in the given character range.
	/// @param[in] selection_start The first character to be selected.
	/// @param[in] selection_end The first character *after* the selection.
	/// @note Only applies to text and password input types.
	void SetSelectionRange(int selection_start, int selection_end);
	/// Retrieves the selection range and text.
	/// @param[out] selection_start The first character selected.
	/// @param[out] selection_end The first character *after* the selection.
	/// @param[out] selected_text The selected text.
	/// @note Only applies to text and password input types.
	void GetSelection(int* selection_start, int* selection_end, String* selected_text) const;

	/// Sets visual feedback used for the IME composition in the range.
	/// @param[in] range_start The first character to be selected.
	/// @param[in] range_end The first character *after* the selection.
	/// @note Only applies to text and password input types.
	void SetCompositionRange(int range_start, int range_end);

protected:
	/// Updates the element's underlying type.
	void OnUpdate() override;
	/// Renders the element's underlying type.
	void OnRender() override;
	/// Calls the element's underlying type.
	void OnResize() override;
	/// Calls the element's underlying type.
	void OnLayout() override;

	/// Checks for necessary functional changes in the control as a result of changed attributes.
	/// @param[in] changed_attributes The list of changed attributes.
	void OnAttributeChange(const ElementAttributes& changed_attributes) override;
	/// Called when properties on the control are changed.
	/// @param[in] changed_properties The properties changed on the element.
	void OnPropertyChange(const PropertyIdSet& changed_properties) override;

	/// If we are the added element, this will pass the call onto our type handler.
	/// @param[in] child The new member of the hierarchy.
	void OnChildAdd(Element* child) override;
	/// If we are the removed element, this will pass the call onto our type handler.
	/// @param[in] child The member of the hierarchy that was just removed.
	void OnChildRemove(Element* child) override;

	/// Checks for necessary functional changes in the control as a result of the event.
	/// @param[in] event The event to process.
	void ProcessDefaultAction(Event& event) override;

	/// Sizes the dimensions to the element's inherent size.
	/// @return True.
	bool GetIntrinsicDimensions(Vector2f& dimensions, float& ratio) override;

private:
	UniquePtr<InputType> type;
	String type_name;
};

} // namespace Rml
#endif
