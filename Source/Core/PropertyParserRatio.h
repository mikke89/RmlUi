#pragma once

#include "../../Include/RmlUi/Core/PropertyParser.h"

namespace Rml {

/**
    A property parser that parses an ratio in the format of x/y, like 16/9.
 */

class PropertyParserRatio : public PropertyParser {
public:
	PropertyParserRatio();
	virtual ~PropertyParserRatio();

	/// Called to parse a RCSS string declaration.
	/// @param[out] property The property to set the parsed value on.
	/// @param[in] value The raw value defined for this property.
	/// @param[in] parameters The parameters defined for this property; not used for this parser.
	/// @return True if the value was validated successfully, false otherwise.
	bool ParseValue(Property& property, const String& value, const ParameterMap& parameters) const override;
};

} // namespace Rml
