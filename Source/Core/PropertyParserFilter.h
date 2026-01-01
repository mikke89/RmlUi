#pragma once

#include "../../Include/RmlUi/Core/PropertyParser.h"

namespace Rml {

/**
    A property parser for the filter property.
 */

class PropertyParserFilter : public PropertyParser {
public:
	PropertyParserFilter();
	virtual ~PropertyParserFilter();

	/// Called to parse a decorator declaration.
	bool ParseValue(Property& property, const String& value, const ParameterMap& parameters) const override;
};

} // namespace Rml
