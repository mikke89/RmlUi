#pragma once

#include "../../Include/RmlUi/Core/PropertyParser.h"

namespace Rml {

/**
    A passthrough property parser that parses a string.
 */

class PropertyParserString : public PropertyParser {
public:
	PropertyParserString();
	virtual ~PropertyParserString();

	/// Called to parse a RCSS string declaration.
	/// @param[out] property The property to set the parsed value on.
	/// @param[in] value The raw value defined for this property.
	/// @param[in] parameters The parameters defined for this property; not used for this parser.
	/// @return True if the value was validated successfully, false otherwise.
	bool ParseValue(Property& property, const String& value, const ParameterMap& parameters) const override;
};

} // namespace Rml
