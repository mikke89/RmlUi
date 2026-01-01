#include "../../Include/RmlUi/Core/EffectSpecification.h"
#include "../../Include/RmlUi/Core/PropertyDefinition.h"

namespace Rml {

EffectSpecification::EffectSpecification() : properties(10, 10) {}

EffectSpecification::~EffectSpecification() {}

const PropertySpecification& EffectSpecification::GetPropertySpecification() const
{
	return properties;
}

PropertyDefinition& EffectSpecification::RegisterProperty(const String& property_name, const String& default_value)
{
	return properties.RegisterProperty(property_name, default_value, false, false);
}

ShorthandId EffectSpecification::RegisterShorthand(const String& shorthand_name, const String& property_names, ShorthandType type)
{
	return properties.RegisterShorthand(shorthand_name, property_names, type);
}

} // namespace Rml
