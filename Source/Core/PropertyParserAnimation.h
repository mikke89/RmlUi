#pragma once

#include "../../Include/RmlUi/Core/PropertyParser.h"
#include "ControlledLifetimeResource.h"

namespace Rml {

/**
Parses the RCSS 'animation' and 'transition' property specifications.
*/

class PropertyParserAnimation : public PropertyParser {
public:
	enum Type { ANIMATION_PARSER, TRANSITION_PARSER } type;

	/// Constructs the parser for either the animation or the transition type.
	PropertyParserAnimation(Type type);

	/// Called to parse a RCSS animation or transition declaration.
	/// @param[out] property The property to set the parsed value on.
	/// @param[in] value The raw value defined for this property.
	/// @param[in] parameters The parameters defined for this property.
	/// @return True if the value was validated successfully, false otherwise.
	bool ParseValue(Property& property, const String& value, const ParameterMap& parameters) const override;

	static void Initialize();
	static void Shutdown();

private:
	static bool ParseAnimation(Property& property, const StringList& animation_values);
	static bool ParseTransition(Property& property, const StringList& transition_values);

	static ControlledLifetimeResource<struct PropertyParserAnimationData> parser_data;
};

} // namespace Rml
