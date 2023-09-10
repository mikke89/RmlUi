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

#include "FilterBasic.h"
#include "../../Include/RmlUi/Core/CompiledFilterShader.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/PropertyDefinition.h"
#include "../../Include/RmlUi/Core/PropertyDictionary.h"
#include "../../Include/RmlUi/Core/RenderManager.h"

namespace Rml {

bool FilterBasic::Initialise(const String& in_name, float in_value)
{
	name = in_name;
	value = in_value;
	return true;
}

CompiledFilter FilterBasic::CompileFilter(Element* element) const
{
	return element->GetRenderManager()->CompileFilter(name, Dictionary{{"value", Variant(value)}});
}

FilterBasicInstancer::FilterBasicInstancer(ValueType value_type, const char* default_value)
{
	switch (value_type)
	{
	case ValueType::NumberPercent: ids.value = RegisterProperty("value", default_value).AddParser("number_percent").GetId(); break;
	case ValueType::Angle: ids.value = RegisterProperty("value", default_value).AddParser("angle").GetId(); break;
	}

	RegisterShorthand("filter", "value", ShorthandType::FallThrough);
}

SharedPtr<Filter> FilterBasicInstancer::InstanceFilter(const String& name, const PropertyDictionary& properties)
{
	const Property* p_value = properties.GetProperty(ids.value);
	if (!p_value)
		return nullptr;

	float value = p_value->Get<float>();
	if (p_value->unit == Unit::PERCENT)
		value *= 0.01f;
	else if (p_value->unit == Unit::DEG)
		value = Rml::Math::DegreesToRadians(value);

	auto filter = MakeShared<FilterBasic>();
	if (filter->Initialise(name, value))
		return filter;

	return nullptr;
}

} // namespace Rml
