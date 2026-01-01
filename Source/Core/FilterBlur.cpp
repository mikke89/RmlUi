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
