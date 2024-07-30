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

#include "../../../Include/RmlUi/Core/Elements/ElementFormControlInput.h"
#include "../../../Include/RmlUi/Core/Event.h"
#include "InputTypeButton.h"
#include "InputTypeCheckbox.h"
#include "InputTypeRadio.h"
#include "InputTypeRange.h"
#include "InputTypeSubmit.h"
#include "InputTypeText.h"

namespace Rml {

ElementFormControlInput::ElementFormControlInput(const String& tag) : ElementFormControl(tag)
{
	// OnAttributeChange will be called right after this, possible with a non-default type. Thus,
	// creating the default InputTypeText here may result in it being destroyed in just a few moments.
	// Instead, we create the InputTypeText in OnAttributeChange in the case where the type attribute has not been set.
}

ElementFormControlInput::~ElementFormControlInput() {}

String ElementFormControlInput::GetValue() const
{
	RMLUI_ASSERT(type);
	return type->GetValue();
}

void ElementFormControlInput::SetValue(const String& value)
{
	SetAttribute("value", value);
}

bool ElementFormControlInput::IsSubmitted()
{
	RMLUI_ASSERT(type);
	return type->IsSubmitted();
}

void ElementFormControlInput::Select()
{
	RMLUI_ASSERT(type);
	type->Select();
}

void ElementFormControlInput::SetSelectionRange(int selection_start, int selection_end)
{
	RMLUI_ASSERT(type);
	type->SetSelectionRange(selection_start, selection_end);
}

void ElementFormControlInput::GetSelection(int* selection_start, int* selection_end, String* selected_text) const
{
	RMLUI_ASSERT(type);
	type->GetSelection(selection_start, selection_end, selected_text);
}

void ElementFormControlInput::SetCompositionRange(int range_start, int range_end)
{
	RMLUI_ASSERT(type);
	type->SetCompositionRange(range_start, range_end);
}

void ElementFormControlInput::OnUpdate()
{
	RMLUI_ASSERT(type);
	type->OnUpdate();
}

void ElementFormControlInput::OnRender()
{
	RMLUI_ASSERT(type);
	type->OnRender();
}

void ElementFormControlInput::OnResize()
{
	RMLUI_ASSERT(type);
	type->OnResize();
}

void ElementFormControlInput::OnLayout()
{
	RMLUI_ASSERT(type);
	type->OnLayout();
}

void ElementFormControlInput::OnAttributeChange(const ElementAttributes& changed_attributes)
{
	ElementFormControl::OnAttributeChange(changed_attributes);

	String new_type_name;

	auto it_type = changed_attributes.find("type");
	if (it_type != changed_attributes.end())
	{
		new_type_name = it_type->second.Get<String>("text");
	}

	if (!type || (!new_type_name.empty() && new_type_name != type_name))
	{
		// Reset the existing type before constructing a new one. This ensures the old type removes properties and event
		// listeners attached to this element, so it does not interfere with new ones being attached by the new type.
		type.reset();

		if (new_type_name == "password")
			type = MakeUnique<InputTypeText>(this, InputTypeText::OBSCURED);
		else if (new_type_name == "radio")
			type = MakeUnique<InputTypeRadio>(this);
		else if (new_type_name == "checkbox")
			type = MakeUnique<InputTypeCheckbox>(this);
		else if (new_type_name == "range")
			type = MakeUnique<InputTypeRange>(this);
		else if (new_type_name == "submit")
			type = MakeUnique<InputTypeSubmit>(this);
		else if (new_type_name == "button")
			type = MakeUnique<InputTypeButton>(this);
		else
		{
			new_type_name = "text";
			type = MakeUnique<InputTypeText>(this);
		}

		if (!type_name.empty())
			SetClass(type_name, false);
		SetClass(new_type_name, true);
		type_name = new_type_name;

		DirtyLayout();
	}

	RMLUI_ASSERT(type);

	if (!type->OnAttributeChange(changed_attributes))
		DirtyLayout();
}

void ElementFormControlInput::OnPropertyChange(const PropertyIdSet& changed_properties)
{
	ElementFormControl::OnPropertyChange(changed_properties);

	if (type)
		type->OnPropertyChange(changed_properties);
}

void ElementFormControlInput::OnChildAdd(Element* child)
{
	if (child == this && type)
		type->OnChildAdd();
}

void ElementFormControlInput::OnChildRemove(Element* child)
{
	if (child == this && type)
		type->OnChildRemove();
}

void ElementFormControlInput::ProcessDefaultAction(Event& event)
{
	ElementFormControl::ProcessDefaultAction(event);
	if (type)
		type->ProcessDefaultAction(event);
}

bool ElementFormControlInput::GetIntrinsicDimensions(Vector2f& dimensions, float& ratio)
{
	if (type)
		return type->GetIntrinsicDimensions(dimensions, ratio);
	return false;
}

} // namespace Rml
