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

#include "../../../Include/RmlUi/Core/Elements/ElementFormControlSelect.h"
#include "../../../Include/RmlUi/Core/ElementText.h"
#include "../../../Include/RmlUi/Core/ElementUtilities.h"
#include "../../../Include/RmlUi/Core/Event.h"
#include "WidgetDropDown.h"

namespace Rml {

ElementFormControlSelect::ElementFormControlSelect(const String& tag) : ElementFormControl(tag), widget(nullptr)
{
	widget = new WidgetDropDown(this);
}

ElementFormControlSelect::~ElementFormControlSelect()
{
	delete widget;
}

String ElementFormControlSelect::GetValue() const
{
	return GetAttribute("value", String());
}

void ElementFormControlSelect::SetValue(const String& value)
{
	MoveChildren();

	SetAttribute("value", value);
}

void ElementFormControlSelect::SetSelection(int selection)
{
	MoveChildren();

	widget->SetSelection(widget->GetOption(selection));
}

int ElementFormControlSelect::GetSelection() const
{
	return widget->GetSelection();
}

Element* ElementFormControlSelect::GetOption(int index)
{
	MoveChildren();

	return widget->GetOption(index);
}

int ElementFormControlSelect::GetNumOptions()
{
	MoveChildren();

	return widget->GetNumOptions();
}

int ElementFormControlSelect::Add(const String& rml, const String& value, int before, bool selectable)
{
	MoveChildren();

	return widget->AddOption(rml, value, before, false, selectable);
}

int ElementFormControlSelect::Add(ElementPtr element, int before)
{
	MoveChildren();

	return widget->AddOption(std::move(element), before);
}

void ElementFormControlSelect::Remove(int index)
{
	MoveChildren();

	widget->RemoveOption(index);
}

void ElementFormControlSelect::RemoveAll()
{
	MoveChildren();

	widget->ClearOptions();
}

void ElementFormControlSelect::OnUpdate()
{
	ElementFormControl::OnUpdate();

	MoveChildren();

	widget->OnUpdate();
}

void ElementFormControlSelect::OnRender()
{
	ElementFormControl::OnRender();

	widget->OnRender();
}

void ElementFormControlSelect::OnLayout()
{
	widget->OnLayout();
}

void ElementFormControlSelect::OnChildAdd(Element* child)
{
	if (widget)
		widget->OnChildAdd(child);
}

void ElementFormControlSelect::OnChildRemove(Element* child)
{
	if (widget)
		widget->OnChildRemove(child);
}

void ElementFormControlSelect::MoveChildren()
{
	// Move any child elements into the widget (except for the three functional elements).
	while (Element* raw_child = GetFirstChild())
	{
		ElementPtr child = RemoveChild(raw_child);
		widget->AddOption(std::move(child), -1);
	}
}

bool ElementFormControlSelect::GetIntrinsicDimensions(Vector2f& intrinsic_dimensions, float& /*ratio*/)
{
	intrinsic_dimensions.x = 128;
	intrinsic_dimensions.y = 16;
	return true;
}

void ElementFormControlSelect::OnAttributeChange(const ElementAttributes& changed_attributes)
{
	RMLUI_ASSERT(widget);

	ElementFormControl::OnAttributeChange(changed_attributes);

	auto it = changed_attributes.find("value");
	if (it != changed_attributes.end())
		widget->OnValueChange(it->second.Get<String>());
}

} // namespace Rml
