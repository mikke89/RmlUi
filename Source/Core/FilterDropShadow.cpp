#include "FilterDropShadow.h"
#include "../../Include/RmlUi/Core/CompiledFilterShader.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/PropertyDefinition.h"
#include "../../Include/RmlUi/Core/PropertyDictionary.h"
#include "../../Include/RmlUi/Core/RenderManager.h"

namespace Rml {

bool FilterDropShadow::Initialise(Colourb in_color, NumericValue in_offset_x, NumericValue in_offset_y, NumericValue in_sigma)
{
	color = in_color;
	value_offset_x = in_offset_x;
	value_offset_y = in_offset_y;
	value_sigma = in_sigma;
	return Any(in_offset_x.unit & Unit::LENGTH) && Any(in_offset_y.unit & Unit::LENGTH) && Any(in_sigma.unit & Unit::LENGTH);
}

CompiledFilter FilterDropShadow::CompileFilter(Element* element) const
{
	const float sigma = element->ResolveLength(value_sigma);
	const Vector2f offset = {
		element->ResolveLength(value_offset_x),
		element->ResolveLength(value_offset_y),
	};

	CompiledFilter filter = element->GetRenderManager()->CompileFilter("drop-shadow",
		Dictionary{{"color", Variant(color)}, {"offset", Variant(offset)}, {"sigma", Variant(sigma)}});

	return filter;
}

void FilterDropShadow::ExtendInkOverflow(Element* element, Rectanglef& scissor_region) const
{
	// Expand the ink overflow area to cover both the native element *and* its offset shadow w/blur.
	const float sigma = element->ResolveLength(value_sigma);
	const Vector2f offset = {
		element->ResolveLength(value_offset_x),
		element->ResolveLength(value_offset_y),
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
	if (decorator->Initialise(p_color->Get<Colourb>(), p_offset_x->GetNumericValue(), p_offset_y->GetNumericValue(), p_sigma->GetNumericValue()))
		return decorator;

	return nullptr;
}

} // namespace Rml
