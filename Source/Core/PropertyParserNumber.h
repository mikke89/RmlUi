#pragma once

#include "../../Include/RmlUi/Core/PropertyParser.h"
#include "ControlledLifetimeResource.h"

namespace Rml {

/**
    A property parser that parses a floating-point number with an optional unit.
 */

class PropertyParserNumber : public PropertyParser {
public:
	PropertyParserNumber(Units units, Unit zero_unit = Unit::UNKNOWN);
	virtual ~PropertyParserNumber();

	/// Called to parse a RCSS number declaration.
	/// @param[out] property The property to set the parsed value on.
	/// @param[in] value The raw value defined for this property.
	/// @param[in] parameters The parameters defined for this property.
	/// @return True if the value was validated successfully, false otherwise.
	bool ParseValue(Property& property, const String& value, const ParameterMap& parameters) const override;

	static void Initialize();
	static void Shutdown();

private:
	static ControlledLifetimeResource<struct PropertyParserNumberData> parser_data;

	// Stores a bit mask of allowed units.
	Units units;

	// If zero unit is set and pure numbers are not allowed, parsing of "0" is still allowed and assigned the given unit.
	Unit zero_unit;
};

} // namespace Rml
