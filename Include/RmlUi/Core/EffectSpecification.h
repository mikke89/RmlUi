#pragma once

#include "Header.h"
#include "PropertySpecification.h"
#include "Types.h"

namespace Rml {

class PropertyDefinition;

/**
    Specifies properties and shorthands for effects (decorators and filters).
 */
class RMLUICORE_API EffectSpecification {
public:
	EffectSpecification();

	/// Returns the property specification associated with the instancer.
	const PropertySpecification& GetPropertySpecification() const;

protected:
	~EffectSpecification();

	/// Registers a property for the effect.
	/// @param[in] property_name The name of the new property (how it is specified through RCSS).
	/// @param[in] default_value The default value to be used.
	/// @return The new property definition, ready to have parsers attached.
	PropertyDefinition& RegisterProperty(const String& property_name, const String& default_value);

	/// Registers a shorthand property definition. Specify a shorthand name of 'decorator' or 'filter' to parse
	/// anonymous decorators or filters, respectively.
	/// @param[in] shorthand_name The name to register the new shorthand property under.
	/// @param[in] property_names A comma-separated list of the properties this definition is a shorthand for. The order
	/// in which they are specified here is the order in which the values will be processed.
	/// @param[in] type The type of shorthand to declare.
	/// @return An ID for the new shorthand, or 'Invalid' if the shorthand declaration is invalid.
	ShorthandId RegisterShorthand(const String& shorthand_name, const String& property_names, ShorthandType type);

private:
	PropertySpecification properties;
};

} // namespace Rml
