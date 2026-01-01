#include "../../Include/RmlUi/Core/ComputedValues.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "ComputeProperty.h"

namespace Rml {

const AnimationList* Style::ComputedValues::animation() const
{
	if (auto p = element->GetLocalProperty(PropertyId::Animation))
	{
		if (p->unit == Unit::ANIMATION)
			return &(p->value.GetReference<AnimationList>());
	}
	return nullptr;
}

const TransitionList* Style::ComputedValues::transition() const
{
	if (auto p = element->GetLocalProperty(PropertyId::Transition))
	{
		if (p->unit == Unit::TRANSITION)
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

float Style::ComputedValues::letter_spacing() const
{
	if (inherited.has_letter_spacing)
	{
		if (auto p = element->GetProperty(PropertyId::LetterSpacing))
			return element->ResolveLength(p->GetNumericValue());
	}
	return 0.f;
}

float ResolveValueOr(Style::LengthPercentageAuto length, float base_value, float default_value)
{
	if (length.type == Style::LengthPercentageAuto::Length)
		return length.value;
	else if (length.type == Style::LengthPercentageAuto::Percentage && base_value >= 0.f)
		return length.value * 0.01f * base_value;
	return default_value;
}

float ResolveValueOr(Style::LengthPercentage length, float base_value, float default_value)
{
	if (length.type == Style::LengthPercentage::Length)
		return length.value;
	else if (length.type == Style::LengthPercentage::Percentage && base_value >= 0.f)
		return length.value * 0.01f * base_value;
	return default_value;
}

} // namespace Rml
