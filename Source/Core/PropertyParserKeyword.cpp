#include "PropertyParserKeyword.h"

namespace Rml {

PropertyParserKeyword::PropertyParserKeyword() {}

PropertyParserKeyword::~PropertyParserKeyword() {}

bool PropertyParserKeyword::ParseValue(Property& property, const String& value, const ParameterMap& parameters) const
{
	ParameterMap::const_iterator iterator = parameters.find(StringUtilities::ToLower(value));
	if (iterator == parameters.end())
		return false;

	property.value = Variant((*iterator).second);
	property.unit = Unit::KEYWORD;

	return true;
}

} // namespace Rml
