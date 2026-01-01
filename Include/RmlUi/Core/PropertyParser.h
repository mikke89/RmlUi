#pragma once

#include "Header.h"
#include "Property.h"

namespace Rml {

using ParameterMap = UnorderedMap<String, int>;

/**
    A property parser takes a property declaration in string form, validates it, and converts it to a Property.
 */

class RMLUICORE_API PropertyParser {
public:
	virtual ~PropertyParser() {}

	/// Called to parse a RCSS declaration.
	/// @param[out] property The property to set the parsed value on.
	/// @param[in] value The raw value defined for this property.
	/// @param[in] parameters The list of parameters defined for this property.
	/// @return True if the value was parsed successfully, false otherwise.
	virtual bool ParseValue(Property& property, const String& value, const ParameterMap& parameters) const = 0;
};

} // namespace Rml
