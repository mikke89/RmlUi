#include "PropertyParserNumber.h"
#include <stdlib.h>

namespace Rml {

struct PropertyParserNumberData {
	const UnorderedMap<String, Unit> unit_string_map = {
		{"", Unit::NUMBER},
		{"%", Unit::PERCENT},
		{"px", Unit::PX},
		{"dp", Unit::DP},
		{"x", Unit::X},
		{"vw", Unit::VW},
		{"vh", Unit::VH},
		{"em", Unit::EM},
		{"rem", Unit::REM},
		{"in", Unit::INCH},
		{"cm", Unit::CM},
		{"mm", Unit::MM},
		{"pt", Unit::PT},
		{"pc", Unit::PC},
		{"deg", Unit::DEG},
		{"rad", Unit::RAD},
	};
};

ControlledLifetimeResource<PropertyParserNumberData> PropertyParserNumber::parser_data;

void PropertyParserNumber::Initialize()
{
	parser_data.Initialize();
}

void PropertyParserNumber::Shutdown()
{
	parser_data.Shutdown();
}

PropertyParserNumber::PropertyParserNumber(Units units, Unit zero_unit) : units(units), zero_unit(zero_unit) {}

PropertyParserNumber::~PropertyParserNumber() {}

bool PropertyParserNumber::ParseValue(Property& property, const String& value, const ParameterMap& /*parameters*/) const
{
	// Find the beginning of the unit string in 'value'.
	size_t unit_pos = 0;
	for (size_t i = value.size(); i--;)
	{
		const char c = value[i];
		if ((c >= '0' && c <= '9') || StringUtilities::IsWhitespace(c))
		{
			unit_pos = i + 1;
			break;
		}
	}

	String str_number = value.substr(0, unit_pos);
	String str_unit = StringUtilities::ToLower(value.substr(unit_pos));

	char* str_end = nullptr;
	float float_value = strtof(str_number.c_str(), &str_end);
	if (str_number.c_str() == str_end)
	{
		// Number conversion failed
		return false;
	}

	const auto it = parser_data->unit_string_map.find(str_unit);
	if (it == parser_data->unit_string_map.end())
	{
		// Invalid unit name
		return false;
	}

	const Unit unit = it->second;

	if (Any(unit & units))
	{
		property.value = float_value;
		property.unit = unit;
		return true;
	}

	// Detected unit not allowed.
	// However, we allow a value of "0" if zero_unit is set and no unit specified (that is, unit is a pure NUMBER).
	if (unit == Unit::NUMBER)
	{
		if (zero_unit != Unit::UNKNOWN && float_value == 0.0f)
		{
			property.unit = zero_unit;
			property.value = Variant(0.0f);
			return true;
		}
	}

	return false;
}

} // namespace Rml
