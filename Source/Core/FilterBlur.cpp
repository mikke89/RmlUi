#include "FilterBlur.h"
#include "../../Include/RmlUi/Core/CompiledFilterShader.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/PropertyDefinition.h"
#include "../../Include/RmlUi/Core/PropertyDictionary.h"
#include "../../Include/RmlUi/Core/RenderManager.h"
#ifdef RMLUI_MATH_EXPRESSIONS
	#include "CalculationResolver.h"
#endif

namespace Rml {

bool FilterBlur::Initialise(NumericValue in_sigma)
{
	sigma_value = in_sigma;
	return Any(in_sigma.unit & Unit::LENGTH);
}

#ifdef RMLUI_MATH_EXPRESSIONS
bool FilterBlur::Initialise(CalculationPtr in_sigma)
{
	sigma_calculation = std::move(in_sigma);
	sigma_value = NumericValue(0.f, Unit::PX);
	return bool(sigma_calculation);
}
#endif

CompiledFilter FilterBlur::CompileFilter(Element* element) const
{
	float radius = element->ResolveLength(sigma_value);
#ifdef RMLUI_MATH_EXPRESSIONS
	if (sigma_calculation && !ResolveElementCalculation(*sigma_calculation, *element, 0.f, radius))
		radius = 0.f;
#endif
	return element->GetRenderManager()->CompileFilter("blur", Dictionary{{"sigma", Variant(radius)}});
}

void FilterBlur::ExtendInkOverflow(Element* element, Rectanglef& scissor_region) const
{
	float sigma = element->ResolveLength(sigma_value);
#ifdef RMLUI_MATH_EXPRESSIONS
	if (sigma_calculation && !ResolveElementCalculation(*sigma_calculation, *element, 0.f, sigma))
		sigma = 0.f;
#endif
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
#ifdef RMLUI_MATH_EXPRESSIONS
	if (p_radius->unit == Unit::CALCULATION)
	{
		if (decorator->Initialise(p_radius->value.Get<CalculationPtr>()))
			return decorator;
	}
	else
#endif
		if (decorator->Initialise(p_radius->GetNumericValue()))
		return decorator;

	return nullptr;
}

} // namespace Rml
