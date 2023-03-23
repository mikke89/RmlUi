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

#include "../../../Include/RmlUi/Core/Elements/ElementFormControlTextArea.h"
#include "../../../Include/RmlUi/Core/ElementText.h"
#include "../../../Include/RmlUi/Core/ElementUtilities.h"
#include "../../../Include/RmlUi/Core/Math.h"
#include "../../../Include/RmlUi/Core/PropertyIdSet.h"
#include "../../../Include/RmlUi/Core/StyleSheetSpecification.h"
#include "WidgetTextInputMultiLine.h"

namespace Rml {

// Constructs a new ElementFormControlTextArea.
ElementFormControlTextArea::ElementFormControlTextArea(const String& tag) : ElementFormControl(tag)
{
	widget = MakeUnique<WidgetTextInputMultiLine>(this);

	SetProperty(PropertyId::OverflowX, Property(Style::Overflow::Auto));
	SetProperty(PropertyId::OverflowY, Property(Style::Overflow::Auto));
	SetProperty(PropertyId::WhiteSpace, Property(Style::WhiteSpace::Prewrap));
}

ElementFormControlTextArea::~ElementFormControlTextArea() {}

// Returns a string representation of the current value of the form control.
String ElementFormControlTextArea::GetValue() const
{
	return GetAttribute< String >("value", "");
}

// Sets the current value of the form control.
void ElementFormControlTextArea::SetValue(const String& value)
{
	SetAttribute("value", value);
}

// Sets the number of characters visible across the text area. Note that this will only be precise when using a
// fixed-width font.
void ElementFormControlTextArea::SetNumColumns(int num_columns)
{
	SetAttribute< int >("cols", Math::Max(1, num_columns));
}

// Returns the approximate number of characters visible at once.
int ElementFormControlTextArea::GetNumColumns() const
{
	return GetAttribute< int >("cols", 20);
}

// Sets the number of visible lines of text in the text area.
void ElementFormControlTextArea::SetNumRows(int num_rows)
{
	SetAttribute< int >("rows", Math::Max(1, num_rows));
}

// Returns the number of visible lines of text in the text area.
int ElementFormControlTextArea::GetNumRows() const
{
	return GetAttribute< int >("rows", 2);
}

// Sets the maximum length (in characters) of this text field.
void ElementFormControlTextArea::SetMaxLength(int max_length)
{
	SetAttribute< int >("maxlength", max_length);
}

// Returns the maximum length (in characters) of this text field.
int ElementFormControlTextArea::GetMaxLength() const
{
	return GetAttribute< int >("maxlength", -1);
}

// Enables or disables word-wrapping in the text area.
void ElementFormControlTextArea::SetWordWrap(bool word_wrap)
{
	if (word_wrap != GetWordWrap())
	{
		if (word_wrap)
			RemoveAttribute("wrap");
		else
			SetAttribute("wrap", "nowrap");
	}
}

// Returns the state of word-wrapping in the text area.
bool ElementFormControlTextArea::GetWordWrap()
{
	String attribute = GetAttribute< String >("wrap", "");
	return attribute != "nowrap";
}

void ElementFormControlTextArea::Select()
{
	widget->Select();
}

void ElementFormControlTextArea::SetSelectionRange(int selection_start, int selection_end)
{
	widget->SetSelectionRange(selection_start, selection_end);
}

void ElementFormControlTextArea::GetSelection(int* selection_start, int* selection_end, String* selected_text) const
{
	widget->GetSelection(selection_start, selection_end, selected_text);
}

// Returns the control's inherent size, based on the length of the input field and the current font size.
bool ElementFormControlTextArea::GetIntrinsicDimensions(Vector2f& dimensions, float& /*ratio*/)
{
	dimensions.x = (float) (GetNumColumns() * ElementUtilities::GetStringWidth(this, "m"));
	dimensions.y = (float)GetNumRows() * GetLineHeight();

	return true;
}

// Updates the control's widget.
void ElementFormControlTextArea::OnUpdate()
{
	widget->OnUpdate();
}

// Renders the control's widget.
void ElementFormControlTextArea::OnRender()
{
	widget->OnRender();
}

// Formats the element.
void ElementFormControlTextArea::OnResize()
{
	widget->OnResize();
}

// Formats the element.
void ElementFormControlTextArea::OnLayout()
{
	widget->OnLayout();
}

// Called when attributes on the element are changed.
void ElementFormControlTextArea::OnAttributeChange(const ElementAttributes& changed_attributes)
{
	ElementFormControl::OnAttributeChange(changed_attributes);

	if (changed_attributes.find("wrap") != changed_attributes.end())
	{
		if (GetWordWrap())
			SetProperty(PropertyId::WhiteSpace, Property(Style::WhiteSpace::Prewrap));
		else
			SetProperty(PropertyId::WhiteSpace, Property(Style::WhiteSpace::Pre));
	}

	if (changed_attributes.find("rows") != changed_attributes.end() || changed_attributes.find("cols") != changed_attributes.end())
		DirtyLayout();

	auto it = changed_attributes.find("maxlength");
	if (it != changed_attributes.end())
		widget->SetMaxLength(it->second.Get(-1));

	it = changed_attributes.find("value");
	if (it != changed_attributes.end())
		widget->SetValue(it->second.Get<String>());
}

// Called when properties on the control are changed.
void ElementFormControlTextArea::OnPropertyChange(const PropertyIdSet& changed_properties)
{
	ElementFormControl::OnPropertyChange(changed_properties);

	// Some inherited properties require text formatting update, mainly font and line-height properties.
	const PropertyIdSet changed_inherited_layout_properties = changed_properties &
		(StyleSheetSpecification::GetRegisteredInheritedProperties() & StyleSheetSpecification::GetRegisteredPropertiesForcingLayout());

	if (!changed_inherited_layout_properties.Empty())
		widget->ForceFormattingOnNextLayout();

	if (changed_properties.Contains(PropertyId::Color) || changed_properties.Contains(PropertyId::BackgroundColor))
		widget->UpdateSelectionColours();

	if (changed_properties.Contains(PropertyId::CaretColor))
		widget->GenerateCursor();
}

// Returns the text content of the element.
void ElementFormControlTextArea::GetInnerRML(String& content) const
{
	content = GetValue();
}

} // namespace Rml
