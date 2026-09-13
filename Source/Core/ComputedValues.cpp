#include "../../Include/RmlUi/Core/ComputedValues.h"
#include "../../Include/RmlUi/Core/Context.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/ElementDocument.h"
#include "ComputeProperty.h"
#include "ElementStyle.h"
#ifdef RMLUI_MATH_EXPRESSIONS
	#include "CalculationResolver.h"
#endif

namespace Rml {

#ifdef RMLUI_MATH_EXPRESSIONS
namespace Style {

	struct ComputedCalculationStore {
		UnorderedMap<uint8_t, CalculationPtr> entries;
	};

	ComputedValues::ComputedValues(Element* element) : element(element) {}
	ComputedValues::~ComputedValues() = default;

	CalculationPtr ComputedValues::GetCalculation(ComputedCalculationSlot slot) const
	{
		if (!calculations)
			return nullptr;
		auto it = calculations->entries.find(static_cast<uint8_t>(slot));
		return it != calculations->entries.end() ? it->second : nullptr;
	}

	void ComputedValues::SetCalculation(ComputedCalculationSlot slot, CalculationPtr calculation)
	{
		const uint8_t key = static_cast<uint8_t>(slot);
		if (calculation)
		{
			if (!calculations)
				calculations = UniquePtr<ComputedCalculationStore>(new ComputedCalculationStore);
			calculations->entries[key] = std::move(calculation);
		}
		else if (calculations)
		{
			calculations->entries.erase(key);
			if (calculations->entries.empty())
				calculations.reset();
		}
	}

	void ComputedValues::CopyNonInheritedCalculations(const ComputedValues& other)
	{
		if (!other.calculations)
		{
			calculations.reset();
			return;
		}
		calculations = UniquePtr<ComputedCalculationStore>(new ComputedCalculationStore(*other.calculations));
	}

} // namespace Style
#endif

const AnimationList* Style::ComputedValues::animation() const
{
	if (auto p = GetLocalPropertyWithResolvedVariables(PropertyId::Animation))
	{
		if (p->unit == Unit::ANIMATION)
			return &(p->value.GetReference<AnimationList>());
	}
	return nullptr;
}

const TransitionList* Style::ComputedValues::transition() const
{
	if (auto p = GetLocalPropertyWithResolvedVariables(PropertyId::Transition))
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
		{
#ifdef RMLUI_MATH_EXPRESSIONS
			if (p->unit == Unit::CALCULATION)
			{
				float dp_ratio = 1.f;
				Vector2f vp_dimensions(1.f);
				if (Context* context = element->GetContext())
				{
					dp_ratio = context->GetDensityIndependentPixelRatio();
					vp_dimensions = Vector2f(context->GetDimensions());
				}
				float document_font_size = DefaultComputedValues().font_size();
				if (ElementDocument* document = element->GetOwnerDocument())
					document_font_size = document->GetComputedValues().font_size();
				float value = 0.f;
				if (ComputeDefiniteLength(p, font_size(), document_font_size, dp_ratio, vp_dimensions, value))
					return value;
				return 0.f;
			}
#endif
			return element->ResolveLength(p->GetNumericValue());
		}
	}
	return 0.f;
}

float Style::ComputedValues::GetLocalNumericProperty(PropertyId id, float default_value) const
{
	if (const Property* p = GetLocalPropertyWithResolvedVariables(id))
	{
#ifdef RMLUI_MATH_EXPRESSIONS
		if (p->unit == Unit::CALCULATION)
		{
			float value = default_value;
			if (ComputeDefiniteNumber(p, 0.f, 0.f, 1.f, Vector2f(1.f), value))
				return value;
			return default_value;
		}
#endif
		return p->Get<float>();
	}
	return default_value;
}

const Property* ComputedValues::GetLocalPropertyWithResolvedVariables(PropertyId id) const
{
	return element->GetStyle()->GetLocalPropertyWithResolvedVariables(id);
}

float ResolveValueOr(Style::LengthPercentageAuto length, float base_value, float default_value)
{
	if (length.type == Style::LengthPercentageAuto::Length)
		return length.value;
	else if (length.type == Style::LengthPercentageAuto::Percentage && base_value >= 0.f)
		return length.value * 0.01f * base_value;
#ifdef RMLUI_MATH_EXPRESSIONS
	else if (length.type == Style::LengthPercentageAuto::Calculation && length.calculation)
	{
		float result = 0.f;
		if (ResolveUsedCalculation(*length.calculation, base_value, result))
			return result;
	}
#endif
	return default_value;
}

float ResolveValueOr(Style::LengthPercentage length, float base_value, float default_value)
{
	if (length.type == Style::LengthPercentage::Length)
		return length.value;
	else if (length.type == Style::LengthPercentage::Percentage && base_value >= 0.f)
		return length.value * 0.01f * base_value;
#ifdef RMLUI_MATH_EXPRESSIONS
	else if (length.type == Style::LengthPercentage::Calculation && length.calculation)
	{
		float result = 0.f;
		if (ResolveUsedCalculation(*length.calculation, base_value, result))
			return result;
	}
#endif
	return default_value;
}

} // namespace Rml
