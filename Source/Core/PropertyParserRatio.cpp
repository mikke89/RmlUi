#include "PropertyParserRatio.h"

namespace Rml {

PropertyParserRatio::PropertyParserRatio() {}

PropertyParserRatio::~PropertyParserRatio() {}

bool PropertyParserRatio::ParseValue(Property& property, const String& value, const ParameterMap& /*parameters*/) const
{
	StringList parts;
	StringUtilities::ExpandString(parts, value, '/');

	if (parts.size() != 2)
	{
		return false;
	}

	float first_value = 0;
	if (!TypeConverter<String, float>::Convert(parts[0], first_value))
	{
		// Number conversion failed
		return false;
	}

	float second_value = 0;
	if (!TypeConverter<String, float>::Convert(parts[1], second_value))
	{
		// Number conversion failed
		return false;
	}

	property.value = Variant(Vector2f(first_value, second_value));
	property.unit = Unit::RATIO;

	return true;
}

} // namespace Rml
