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

#include "InputTypeRadio.h"
#include "../../../Include/RmlUi/Core/ElementUtilities.h"
#include "../../../Include/RmlUi/Core/Elements/ElementForm.h"
#include "../../../Include/RmlUi/Core/Elements/ElementFormControlInput.h"

namespace Rml {

InputTypeRadio::InputTypeRadio(ElementFormControlInput* element) : InputType(element)
{
	if (element->HasAttribute("checked"))
		PopRadioSet();
}

InputTypeRadio::~InputTypeRadio() {}

String InputTypeRadio::GetValue() const
{
	auto value = InputType::GetValue();
	return value.empty() ? "on" : value;
}

bool InputTypeRadio::IsSubmitted()
{
	return element->HasAttribute("checked");
}

bool InputTypeRadio::OnAttributeChange(const ElementAttributes& changed_attributes)
{
	if (changed_attributes.count("checked"))
	{
		bool checked = element->HasAttribute("checked");
		element->SetPseudoClass("checked", checked);

		if (checked)
			PopRadioSet();

		const auto perceived_value = Variant(checked ? GetValue() : "");
		element->DispatchEvent(EventId::Change,
			{{"data-binding-override-value", checked ? Variant(perceived_value) : Variant()}, {"value", perceived_value}});
	}

	return true;
}

void InputTypeRadio::OnChildAdd()
{
	if (element->HasAttribute("checked"))
		PopRadioSet();
}

void InputTypeRadio::ProcessDefaultAction(Event& event)
{
	if (event == EventId::Click && !element->IsDisabled())
		element->SetAttribute("checked", "");
}

bool InputTypeRadio::GetIntrinsicDimensions(Vector2f& dimensions, float& ratio)
{
	dimensions.x = 16;
	dimensions.y = 16;
	ratio = 1;

	return true;
}

void InputTypeRadio::PopRadioSet()
{
	// Uncheck all other radio buttons with our name in the form.
	ElementForm* form = nullptr;
	Element* parent = element->GetParentNode();
	while (parent != nullptr && (form = rmlui_dynamic_cast<ElementForm*>(parent)) == nullptr)
		parent = parent->GetParentNode();

	if (form != nullptr)
	{
		ElementList form_controls;
		ElementUtilities::GetElementsByTagName(form_controls, form, "input");

		for (size_t i = 0; i < form_controls.size(); ++i)
		{
			ElementFormControlInput* radio_control = rmlui_dynamic_cast<ElementFormControlInput*>(form_controls[i]);
			if (radio_control != nullptr && element != radio_control && radio_control->GetAttribute<String>("type", "text") == "radio" &&
				radio_control->GetName() == element->GetName())
			{
				radio_control->RemoveAttribute("checked");
			}
		}
	}
}

} // namespace Rml
