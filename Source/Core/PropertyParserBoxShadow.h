#pragma once

#include "../../Include/RmlUi/Core/PropertyParser.h"

namespace Rml {

/**
    Parses the RCSS 'box-shadow' property.
*/

class PropertyParserBoxShadow : public PropertyParser {
public:
	PropertyParserBoxShadow(PropertyParser* parser_color, PropertyParser* parser_length);

	/// Called to parse a RCSS declaration.
	/// @param[out] property The property to set the parsed value on.
	/// @param[in] value The raw value defined for this property.
	/// @param[in] parameters The parameters defined for this property.
	/// @return True if the value was validated successfully, false otherwise.
	bool ParseValue(Property& property, const String& value, const ParameterMap& parameters) const override;

private:
	PropertyParser* parser_color;
	PropertyParser* parser_length;
};

} // namespace Rml
