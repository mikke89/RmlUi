#pragma once

#include "../../Include/RmlUi/Core/PropertyParser.h"

namespace Rml {

/**
    A property parser for the font-effect property.
 */

class PropertyParserFontEffect : public PropertyParser {
public:
	PropertyParserFontEffect();
	virtual ~PropertyParserFontEffect();

	/// Called to parse a font-effect declaration.
	bool ParseValue(Property& property, const String& value, const ParameterMap& parameters) const override;
};

} // namespace Rml
