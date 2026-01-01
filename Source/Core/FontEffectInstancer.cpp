#include "../../Include/RmlUi/Core/FontEffectInstancer.h"
#include "../../Include/RmlUi/Core/PropertyDefinition.h"

namespace Rml {

FontEffectInstancer::FontEffectInstancer() : properties(10, 10) {}

FontEffectInstancer::~FontEffectInstancer() {}

const PropertySpecification& FontEffectInstancer::GetPropertySpecification() const
{
	return properties;
}

PropertyDefinition& FontEffectInstancer::RegisterProperty(const String& property_name, const String& default_value, bool affects_generation)
{
	PropertyDefinition& definition = properties.RegisterProperty(property_name, default_value, false, false);
	if (affects_generation)
		volatile_properties.insert(definition.GetId());

	return definition;
}

ShorthandId FontEffectInstancer::RegisterShorthand(const String& shorthand_name, const String& property_names, ShorthandType type)
{
	return properties.RegisterShorthand(shorthand_name, property_names, type);
}

} // namespace Rml
