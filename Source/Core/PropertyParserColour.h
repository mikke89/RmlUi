#pragma once

#include "../../Include/RmlUi/Core/PropertyParser.h"
#include "../../Include/RmlUi/Core/Types.h"
#include "ControlledLifetimeResource.h"

namespace Rml {

/**
    A property parser that parses a colour value.
 */

class PropertyParserColour : public PropertyParser {
public:
	PropertyParserColour();
	virtual ~PropertyParserColour();

	/// Called to parse a RCSS colour declaration.
	/// @param[out] property The property to set the parsed value on.
	/// @param[in] value The raw value defined for this property.
	/// @param[in] parameters The parameters defined for this property; not used for this parser.
	/// @return True if the value was parsed successfully, false otherwise.
	bool ParseValue(Property& property, const String& value, const ParameterMap& parameters) const override;

	/// Parse a colour directly.
	static bool ParseColour(Colourb& colour, const String& value);

	static void Initialize();
	static void Shutdown();

private:
	static ControlledLifetimeResource<struct PropertyParserColourData> parser_data;

	// Parse a colour in hex-code form (e.g. #FF00FF or #00FF00FF).
	static bool ParseHexColour(Colourb& colour, const String& value);

	// Parse a colour in RGB form (e.g. rgb(255, 0, 255) or rgba(0, 255, 0, 255)).
	static bool ParseRGBColour(Colourb& colour, const String& value);

	// Parse a colour in HSL form (e.g. hsl(0, 100%, 50%) or hsla(0, 100%, 50%, 1.0)).
	static bool ParseHSLColour(Colourb& colour, const String& value);

	// Parse a colour in CIELAB form (e.g. lab(100.0 0.0 0.0) or lab(50.0 -60.0 60.0 / 0.5)
	//     or CIELCh form (e.g. lch(100.0 0.0 30) or lch(100.0 0.0 60 / 0.5)).
	static bool ParseCIELABColour(Colourb& colour, const String& value);

	// Parse a colour in Oklab form (e.g. oklab(1.0 0.0 0.0) or oklab(0.5 -0.2 0.2 / 0.5))
	//     or Oklch form (e.g. oklch(1.0 0.0 30) or oklch(1.0 0.0 60 / 0.5)).
	static bool ParseOklabColour(Colourb& colour, const String& value);

	static bool GetColourFunctionValues(StringList& values, const String& value, bool is_comma_separated);
};

} // namespace Rml
