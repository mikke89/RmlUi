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

#include "../../Include/RmlUi/Core/ComputedValues.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "ComputeProperty.h"

namespace Rml {

const AnimationList* Style::ComputedValues::animation() const
{
	if (auto p = element->GetLocalProperty(PropertyId::Animation))
	{
		if (p->unit == Property::ANIMATION)
			return &(p->value.GetReference<AnimationList>());
	}
	return nullptr;
}

const TransitionList* Style::ComputedValues::transition() const
{
	if (auto p = element->GetLocalProperty(PropertyId::Transition))
	{
		if (p->unit == Property::TRANSITION)
			return &(p->value.GetReference<TransitionList>());
	}
	return nullptr;
}

String Style::ComputedValues::font_family() const
{
	if (auto p = element->GetProperty(PropertyId::FontFamily))
		return ComputeFontFamily(p->Get<String>());

	return String();
}

String Style::ComputedValues::cursor() const
{
	if (auto p = element->GetProperty(PropertyId::Cursor))
		return p->Get<String>();

	return String();
}

float ResolveValue(Style::LengthPercentageAuto length, float base_value)
{
	if (length.type == Style::LengthPercentageAuto::Length)
		return length.value;
	else if (length.type == Style::LengthPercentageAuto::Percentage)
		return length.value * 0.01f * base_value;
	return 0.0f;
}

float ResolveValue(Style::LengthPercentage length, float base_value)
{
	if (length.type == Style::LengthPercentage::Length)
		return length.value;
	else if (length.type == Style::LengthPercentage::Percentage)
		return length.value * 0.01f * base_value;
	return 0.0f;
}

} // namespace Rml
