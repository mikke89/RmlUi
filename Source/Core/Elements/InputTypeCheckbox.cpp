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

#include "InputTypeCheckbox.h"
#include "../../../Include/RmlUi/Core/Elements/ElementFormControlInput.h"

namespace Rml {

InputTypeCheckbox::InputTypeCheckbox(ElementFormControlInput* element) : InputType(element) {}

InputTypeCheckbox::~InputTypeCheckbox() {}

String InputTypeCheckbox::GetValue() const
{
	auto value = InputType::GetValue();
	return value.empty() ? "on" : value;
}

bool InputTypeCheckbox::IsSubmitted()
{
	return element->HasAttribute("checked");
}

bool InputTypeCheckbox::OnAttributeChange(const ElementAttributes& changed_attributes)
{
	if (changed_attributes.count("checked"))
	{
		const bool checked = element->HasAttribute("checked");
		element->SetPseudoClass("checked", checked);
		element->DispatchEvent(EventId::Change, {{"data-binding-override-value", Variant(checked)}, {"value", Variant(checked ? GetValue() : "")}});
	}

	return true;
}

void InputTypeCheckbox::ProcessDefaultAction(Event& event)
{
	if (event == EventId::Click && !element->IsDisabled())
	{
		if (element->HasAttribute("checked"))
			element->RemoveAttribute("checked");
		else
			element->SetAttribute("checked", "");
	}
}

bool InputTypeCheckbox::GetIntrinsicDimensions(Vector2f& dimensions, float& ratio)
{
	dimensions.x = 16;
	dimensions.y = 16;
	ratio = 1;

	return true;
}

} // namespace Rml
