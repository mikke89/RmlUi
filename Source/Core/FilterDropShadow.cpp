#include "FilterDropShadow.h"
#include "../../Include/RmlUi/Core/CompiledFilterShader.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/PropertyDefinition.h"
#include "../../Include/RmlUi/Core/PropertyDictionary.h"
#include "../../Include/RmlUi/Core/RenderManager.h"
#ifdef RMLUI_MATH_EXPRESSIONS
	#include "CalculationResolver.h"
#endif

namespace Rml {

bool FilterDropShadow::Initialise(Colourb in_color, NumericValue in_offset_x, NumericValue in_offset_y, NumericValue in_sigma)
{
	color = in_color;
	value_offset_x = in_offset_x;
	value_offset_y = in_offset_y;
	value_sigma = in_sigma;
	return Any(in_offset_x.unit & Unit::LENGTH) && Any(in_offset_y.unit & Unit::LENGTH) && Any(in_sigma.unit & Unit::LENGTH);
}

#ifdef RMLUI_MATH_EXPRESSIONS
bool FilterDropShadow::Initialise(Colourb in_color, NumericValue in_offset_x, NumericValue in_offset_y, NumericValue in_sigma,
	CalculationPtr in_offset_x_calculation, CalculationPtr in_offset_y_calculation, CalculationPtr in_sigma_calculation)
{
	color = in_color;
	value_offset_x = in_offset_x;
	value_offset_y = in_offset_y;
	value_sigma = in_sigma;
	calculation_offset_x = std::move(in_offset_x_calculation);
	calculation_offset_y = std::move(in_offset_y_calculation);
	calculation_sigma = std::move(in_sigma_calculation);
	return true;
}
#endif

CompiledFilter FilterDropShadow::CompileFilter(Element* element) const
{
	float sigma = element->ResolveLength(value_sigma);
	float offset_x = element->ResolveLength(value_offset_x);
	float offset_y = element->ResolveLength(value_offset_y);
#ifdef RMLUI_MATH_EXPRESSIONS
	if (calculation_sigma && !ResolveElementCalculation(*calculation_sigma, *element, 0.f, sigma))
		sigma = 0.f;
	if (calculation_offset_x && !ResolveElementCalculation(*calculation_offset_x, *element, 0.f, offset_x))
		offset_x = 0.f;
	if (calculation_offset_y && !ResolveElementCalculation(*calculation_offset_y, *element, 0.f, offset_y))
		offset_y = 0.f;
#endif
	const Vector2f offset = {
		offset_x,
		offset_y,
	};

	CompiledFilter filter = element->GetRenderManager()->CompileFilter("drop-shadow",
		Dictionary{{"color", Variant(color)}, {"offset", Variant(offset)}, {"sigma", Variant(sigma)}});

	return filter;
}

void FilterDropShadow::ExtendInkOverflow(Element* element, Rectanglef& scissor_region) const
{
	// Expand the ink overflow area to cover both the native element *and* its offset shadow w/blur.
	float sigma = element->ResolveLength(value_sigma);
	float offset_x = element->ResolveLength(value_offset_x);
	float offset_y = element->ResolveLength(value_offset_y);
#ifdef RMLUI_MATH_EXPRESSIONS
	if (calculation_sigma && !ResolveElementCalculation(*calculation_sigma, *element, 0.f, sigma))
		sigma = 0.f;
	if (calculation_offset_x && !ResolveElementCalculation(*calculation_offset_x, *element, 0.f, offset_x))
		offset_x = 0.f;
	if (calculation_offset_y && !ResolveElementCalculation(*calculation_offset_y, *element, 0.f, offset_y))
		offset_y = 0.f;
#endif
	const Vector2f offset = {
		offset_x,
		offset_y,
	};

	const float blur_extent = 3.f * sigma;
	scissor_region =
		scissor_region.Extend(Math::Max(-offset, Vector2f(0.f)) + Vector2f(blur_extent), Math::Max(offset, Vector2f(0.f)) + Vector2f(blur_extent));
}

FilterDropShadowInstancer::FilterDropShadowInstancer()
{
	ids.color = RegisterProperty("color", "transparent").AddParser("color").GetId();
	ids.offset_x = RegisterProperty("offset-x", "0px").AddParser("length").GetId();
	ids.offset_y = RegisterProperty("offset-y", "0px").AddParser("length").GetId();
	ids.sigma = RegisterProperty("sigma", "0px").AddParser("length").GetId();
	RegisterShorthand("filter", "color, offset-x, offset-y, sigma", ShorthandType::FallThrough);
}

SharedPtr<Filter> FilterDropShadowInstancer::InstanceFilter(const String& /*name*/, const PropertyDictionary& properties)
{
	const Property* p_color = properties.GetProperty(ids.color);
	const Property* p_offset_x = properties.GetProperty(ids.offset_x);
	const Property* p_offset_y = properties.GetProperty(ids.offset_y);
	const Property* p_sigma = properties.GetProperty(ids.sigma);
	if (!p_color || !p_offset_x || !p_offset_y || !p_sigma)
		return nullptr;

	auto decorator = MakeShared<FilterDropShadow>();
	NumericValue offset_x(0.f, Unit::PX), offset_y(0.f, Unit::PX), sigma(0.f, Unit::PX);
#ifdef RMLUI_MATH_EXPRESSIONS
	CalculationPtr offset_x_calculation, offset_y_calculation, sigma_calculation;
	auto extract = [](const Property* property, NumericValue& numeric, CalculationPtr& calculation) {
		if (property->unit == Unit::CALCULATION)
		{
			calculation = property->value.Get<CalculationPtr>();
			return bool(calculation);
		}
		numeric = property->GetNumericValue();
		return true;
	};
	if (!extract(p_offset_x, offset_x, offset_x_calculation) || !extract(p_offset_y, offset_y, offset_y_calculation) ||
		!extract(p_sigma, sigma, sigma_calculation))
		return nullptr;
	if (decorator->Initialise(p_color->Get<Colourb>(), offset_x, offset_y, sigma, std::move(offset_x_calculation), std::move(offset_y_calculation),
			std::move(sigma_calculation)))
		return decorator;
#else
	offset_x = p_offset_x->GetNumericValue();
	offset_y = p_offset_y->GetNumericValue();
	sigma = p_sigma->GetNumericValue();
	if (decorator->Initialise(p_color->Get<Colourb>(), offset_x, offset_y, sigma))
		return decorator;
#endif

	return nullptr;
}

} // namespace Rml
