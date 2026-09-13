#include "PropertyParserBoxShadow.h"
#include "../../Include/RmlUi/Core/DecorationTypes.h"
#include "../../Include/RmlUi/Core/StringUtilities.h"

namespace Rml {

PropertyParserBoxShadow::PropertyParserBoxShadow(PropertyParser* parser_color, PropertyParser* parser_length) :
	parser_color(parser_color), parser_length(parser_length)
{
	RMLUI_ASSERT(parser_color && parser_length);
}

bool PropertyParserBoxShadow::ParseValue(Property& property, const String& value, const ParameterMap& /*parameters*/) const
{
	if (value.empty() || value == "none")
	{
		property.unit = Unit::BOXSHADOWLIST;
		property.value = Variant(BoxShadowList{});
		return true;
	}

	StringList shadows_string_list;
	{
		auto lowercase_value = StringUtilities::ToLower(value);
		StringUtilities::ExpandString(shadows_string_list, lowercase_value, ',', '(', ')');
	}

	if (shadows_string_list.empty())
		return false;

	const ParameterMap empty_parameter_map;

	BoxShadowList shadow_list;
	shadow_list.reserve(shadows_string_list.size());

	for (const String& shadow_str : shadows_string_list)
	{
		StringList arguments;
		StringUtilities::ExpandString(arguments, shadow_str, ' ', '(', ')');
		if (arguments.empty())
			return false;

		shadow_list.push_back({});
		BoxShadow& shadow = shadow_list.back();

		int length_argument_index = 0;

		for (const String& argument : arguments)
		{
			if (argument.empty())
				continue;

			Property prop;
			if (parser_length->ParseValue(prop, argument, empty_parameter_map))
			{
				NumericValue numeric_value;
#ifdef RMLUI_MATH_EXPRESSIONS
				CalculationPtr calculation;
				if (prop.unit == Unit::CALCULATION)
				{
					calculation = prop.value.Get<CalculationPtr>();
					if (!calculation)
						return false;
					numeric_value = NumericValue(0.f, Unit::PX);
				}
				else
#endif
					numeric_value = prop.GetNumericValue();
				switch (length_argument_index)
				{
				case 0: shadow.offset_x = numeric_value;
#ifdef RMLUI_MATH_EXPRESSIONS
					shadow.offset_x_calculation = calculation;
#endif
					break;
				case 1: shadow.offset_y = numeric_value;
#ifdef RMLUI_MATH_EXPRESSIONS
					shadow.offset_y_calculation = calculation;
#endif
					break;
				case 2: shadow.blur_radius = numeric_value;
#ifdef RMLUI_MATH_EXPRESSIONS
					shadow.blur_radius_calculation = calculation;
#endif
					break;
				case 3: shadow.spread_distance = numeric_value;
#ifdef RMLUI_MATH_EXPRESSIONS
					shadow.spread_distance_calculation = calculation;
#endif
					break;
				default: return false;
				}
				length_argument_index += 1;
			}
			else if (argument == "inset")
			{
				shadow.inset = true;
			}
			else if (parser_color->ParseValue(prop, argument, empty_parameter_map))
			{
				shadow.color = prop.Get<Colourb>().ToPremultiplied();
			}
			else
			{
				return false;
			}
		}

		if (length_argument_index < 2)
			return false;
	}

	property.unit = Unit::BOXSHADOWLIST;
	property.value = Variant(std::move(shadow_list));

	return true;
}

} // namespace Rml
