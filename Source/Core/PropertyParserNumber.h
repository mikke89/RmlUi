#pragma once

#include "../../Include/RmlUi/Core/PropertyParser.h"
#include "ControlledLifetimeResource.h"
#ifdef RMLUI_MATH_EXPRESSIONS
	#include "CalculationParser.h"
#endif

namespace Rml {

/**
    A property parser that parses a floating-point number with an optional unit.
 */

class PropertyParserNumber : public PropertyParser {
public:
	PropertyParserNumber(Units units, Unit zero_unit = Unit::UNKNOWN);
#ifdef RMLUI_MATH_EXPRESSIONS
	PropertyParserNumber(Units units, Unit zero_unit, CalculationParseTarget calculation_target);
#endif
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

#ifdef RMLUI_MATH_EXPRESSIONS
	CalculationParseTarget calculation_target;
#endif
};

} // namespace Rml
