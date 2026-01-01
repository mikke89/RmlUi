#pragma once

#include "../../Include/RmlUi/Core/PropertyParser.h"
#include "../../Include/RmlUi/Core/Types.h"
#include "PropertyParserNumber.h"

namespace Rml {

/**
    A property parser that parses color stop lists, particularly for gradients.
 */

class PropertyParserColorStopList : public PropertyParser {
public:
	PropertyParserColorStopList(PropertyParser* parser_color);
	virtual ~PropertyParserColorStopList();

	bool ParseValue(Property& property, const String& value, const ParameterMap& parameters) const override;

private:
	PropertyParser* parser_color;
	PropertyParserNumber parser_length_percent_angle;
};

} // namespace Rml
