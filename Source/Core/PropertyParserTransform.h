#pragma once

#include "../../Include/RmlUi/Core/PropertyParser.h"
#include "PropertyParserNumber.h"

namespace Rml {

/**
    A property parser that parses a RCSS transform property specification.
 */
class PropertyParserTransform : public PropertyParser {
public:
	PropertyParserTransform();
	virtual ~PropertyParserTransform();

	/// Called to parse a RCSS transform declaration.
	/// @param[out] property The property to set the parsed value on.
	/// @param[in] value The raw value defined for this property.
	/// @param[in] parameters The parameters defined for this property.
	/// @return True if the value was validated successfully, false otherwise.
	bool ParseValue(Property& property, const String& value, const ParameterMap& parameters) const override;

private:
	/// Scan a string for a parameterized keyword with a certain number of numeric arguments.
	/// @param[out] out_bytes_read The number of bytes read if the keyword occurs at the beginning of str, 0 otherwise.
	/// @param[in] str The string to search for the parameterized keyword
	/// @param[in] keyword The name of the keyword to search for
	/// @param[in] parsers The numeric argument parsers
	/// @param[out] args The numeric arguments encountered
	/// @param[in] nargs The number of numeric arguments expected
	/// @return True if parsed successfully, false otherwise.
	bool Scan(int& out_bytes_read, const char* str, const char* keyword, const PropertyParser** parsers, NumericValue* args, int nargs) const;

	PropertyParserNumber number, length, length_pct, angle;
};

} // namespace Rml
