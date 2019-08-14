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

#include "../../Include/RmlUi/Controls/ElementFormControlSelect.h"
#include "../../Include/RmlUi/Core/ElementText.h"
#include "../../Include/RmlUi/Core/Event.h"
#include "../../Include/RmlUi/Core/ElementUtilities.h"
#include "WidgetDropDown.h"

namespace Rml {
namespace Controls {

// Constructs a new ElementFormControlSelect.
ElementFormControlSelect::ElementFormControlSelect(const Rml::Core::String& tag) : ElementFormControl(tag)
{
	widget = new WidgetDropDown(this);
}

ElementFormControlSelect::~ElementFormControlSelect()
{
	delete widget;
}

// Returns a string representation of the current value of the form control.
Rml::Core::String ElementFormControlSelect::GetValue() const
{
	RMLUI_ASSERT(widget != nullptr);
	return widget->GetValue();
}

// Sets the current value of the form control.
void ElementFormControlSelect::SetValue(const Rml::Core::String& value)
{
	OnUpdate();

	RMLUI_ASSERT(widget != nullptr);
	widget->SetValue(value);
}

// Sets the index of the selection. If the new index lies outside of the bounds, it will be clamped.
void ElementFormControlSelect::SetSelection(int selection)
{
	OnUpdate();

	RMLUI_ASSERT(widget != nullptr);
	widget->SetSelection(selection);
}

// Returns the index of the currently selected item.
int ElementFormControlSelect::GetSelection() const
{
	RMLUI_ASSERT(widget != nullptr);
	return widget->GetSelection();
}

// Returns one of the select control's option elements.
SelectOption* ElementFormControlSelect::GetOption(int index)
{
	OnUpdate();

	RMLUI_ASSERT(widget != nullptr);
	return widget->GetOption(index);
}

// Returns the number of options in the select control.
int ElementFormControlSelect::GetNumOptions()
{
	OnUpdate();

	RMLUI_ASSERT(widget != nullptr);
	return widget->GetNumOptions();
}

// Adds a new option to the select control.
int ElementFormControlSelect::Add(const Rml::Core::String& rml, const Rml::Core::String& value, int before, bool selectable)
{
	OnUpdate();

	RMLUI_ASSERT(widget != nullptr);
	return widget->AddOption(rml, value, before, false, selectable);
}

// Removes an option from the select control.
void ElementFormControlSelect::Remove(int index)
{
	OnUpdate();

	RMLUI_ASSERT(widget != nullptr);
	widget->RemoveOption(index);
}

// Removes all options from the select control.
void ElementFormControlSelect::RemoveAll()
{
	OnUpdate();

	RMLUI_ASSERT(widget != nullptr);
	widget->ClearOptions();
}

// Moves all children to be under control of the widget.
void ElementFormControlSelect::OnUpdate()
{
	ElementFormControl::OnUpdate();

	// Move any child elements into the widget (except for the three functional elements).
	while (Core::Element * raw_child = GetFirstChild())
	{
		Core::ElementPtr child = RemoveChild(raw_child);

		bool select = child->GetAttribute("selected");
		bool selectable = !child->GetAttribute("disabled");
		Core::String option_value = child->GetAttribute("value", Core::String());

		child->RemoveAttribute("selected");
		child->RemoveAttribute("disabled");
		child->RemoveAttribute("value");

		widget->AddOption(std::move(child), option_value, -1, select, selectable);
	}
}

// Updates the layout of the widget's elements.
void ElementFormControlSelect::OnRender()
{
	ElementFormControl::OnRender();

	widget->OnRender();
}

// Forces an internal layout.
void ElementFormControlSelect::OnLayout()
{
	widget->OnLayout();
}

// Returns true to mark this element as replaced.
bool ElementFormControlSelect::GetIntrinsicDimensions(Rml::Core::Vector2f& intrinsic_dimensions)
{
	intrinsic_dimensions.x = 128;
	intrinsic_dimensions.y = 16;
	return true;
}

}
}
