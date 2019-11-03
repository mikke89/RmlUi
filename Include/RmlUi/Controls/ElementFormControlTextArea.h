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

#ifndef RMLUICONTROLSELEMENTFORMCONTROLTEXTAREA_H
#define RMLUICONTROLSELEMENTFORMCONTROLTEXTAREA_H

#include "Header.h"
#include "ElementFormControl.h"

namespace Rml {
namespace Controls {

class WidgetTextInput;

/**
	Default RmlUi implemention of a text area.

	@author Peter Curry
 */

class RMLUICONTROLS_API ElementFormControlTextArea : public ElementFormControl
{
public:
	RMLUI_RTTI_DefineWithParent(ElementFormControlTextArea, ElementFormControl)

	/// Constructs a new ElementFormControlTextArea. This should not be called directly; use the
	/// Factory instead.
	/// @param[in] tag The tag the element was declared as in RML.
	ElementFormControlTextArea(const Rml::Core::String& tag);
	virtual ~ElementFormControlTextArea();

	/// Returns a string representation of the current value of the form control. This is the value of the control
	/// regardless of whether it has been selected / checked (as appropriate for the control).
	/// @return The value of the form control.
	Rml::Core::String GetValue() const override;
	/// Sets the current value of the form control.
	/// @param[in] value The new value of the form control.
	void SetValue(const Rml::Core::String& value) override;

	/// Sets the number of characters visible across the text area. Note that this will only be precise when using
	/// a fixed-width font.
	/// @param[in] size The number of visible characters.
	void SetNumColumns(int num_columns);
	/// Returns the approximate number of characters visible at once.
	/// @return The number of visible characters.
	int GetNumColumns() const;

	/// Sets the number of visible lines of text in the text area.
	/// @param[in] num_rows The new number of visible lines of text.
	void SetNumRows(int num_rows);
	/// Returns the number of visible lines of text in the text area.
	/// @return The number of visible lines of text.
	int GetNumRows() const;

	/// Sets the maximum length (in characters) of this text area.
	/// @param[in] max_length The new maximum length of the text area. A number lower than zero will mean infinite
	/// characters.
	void SetMaxLength(int max_length);
	/// Returns the maximum length (in characters) of this text area.
	/// @return The maximum number of characters allowed in this text area.
	int GetMaxLength() const;

	/// Enables or disables word-wrapping in the text area.
	/// @param[in] word_wrap True to enable word-wrapping, false to disable.
	void SetWordWrap(bool word_wrap);
	/// Returns the state of word-wrapping in the text area.
	/// @return True if the text area is word-wrapping, false otherwise.
	bool GetWordWrap();

	/// Returns the control's inherent size, based on the length of the input field and the current font size.
	/// @return True.
	bool GetIntrinsicDimensions(Rml::Core::Vector2f& dimensions) override;

protected:
	/// Updates the control's widget.
	void OnUpdate() override;
	/// Renders the control's widget.
	void OnRender() override;
	/// Resizes and positions the control's widget.
	void OnResize() override;
	/// Formats the element.
	void OnLayout() override;

	/// Called when attributes on the element are changed.
	void OnAttributeChange(const Core::ElementAttributes& changed_attributes) override;
	/// Called when properties on the control are changed.
	/// @param[in] changed_properties The properties changed on the element.
	void OnPropertyChange(const Core::PropertyIdSet& changed_properties) override;

	/// Returns the text content of the element.
	/// @param[out] content The content of the element.
	void GetInnerRML(Rml::Core::String& content) const override;

private:
	WidgetTextInput* widget;		
};

}
}

#endif
