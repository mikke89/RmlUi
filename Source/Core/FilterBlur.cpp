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

#include "FilterBlur.h"
#include "../../Include/RmlUi/Core/CompiledFilterShader.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/PropertyDefinition.h"
#include "../../Include/RmlUi/Core/PropertyDictionary.h"
#include "../../Include/RmlUi/Core/RenderManager.h"

namespace Rml {

bool FilterBlur::Initialise(NumericValue in_sigma)
{
	sigma_value = in_sigma;
	return Any(in_sigma.unit & Unit::LENGTH);
}

CompiledFilter FilterBlur::CompileFilter(Element* element) const
{
	const float radius = element->ResolveLength(sigma_value);
	return element->GetRenderManager()->CompileFilter("blur", Dictionary{{"sigma", Variant(radius)}});
}

void FilterBlur::ExtendInkOverflow(Element* element, Rectanglef& scissor_region) const
{
	const float sigma = element->ResolveLength(sigma_value);
	const float blur_extent = 3.0f * Math::Max(sigma, 1.f);
	scissor_region = scissor_region.Extend(blur_extent);
}

FilterBlurInstancer::FilterBlurInstancer()
{
	ids.sigma = RegisterProperty("sigma", "0px").AddParser("length").GetId();
	RegisterShorthand("filter", "sigma", ShorthandType::FallThrough);
}

SharedPtr<Filter> FilterBlurInstancer::InstanceFilter(const String& /*name*/, const PropertyDictionary& properties)
{
	const Property* p_radius = properties.GetProperty(ids.sigma);
	if (!p_radius)
		return nullptr;

	auto decorator = MakeShared<FilterBlur>();
	if (decorator->Initialise(p_radius->GetNumericValue()))
		return decorator;

	return nullptr;
}

} // namespace Rml
