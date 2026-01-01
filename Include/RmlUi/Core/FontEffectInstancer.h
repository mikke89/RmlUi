#pragma once

#include "Header.h"
#include "PropertyDictionary.h"
#include "PropertySpecification.h"
#include "Traits.h"

namespace Rml {

class Factory;
class FontEffect;

/**
    A font effect instancer provides a method for allocating and deallocating font effects.

    It is important that the same instancer that allocated a font effect releases it. This ensures there are no issues
    with memory from different DLLs getting mixed up.
 */

class RMLUICORE_API FontEffectInstancer {
public:
	FontEffectInstancer();
	virtual ~FontEffectInstancer();

	/// Instances a font effect given the property tag and attributes from the RCSS file.
	/// @param[in] name The type of font effect desired. For example, "font-effect: outline(1px black);" is declared as type "outline".
	/// @param[in] properties All RCSS properties associated with the font effect.
	/// @return A shared_ptr to the font-effect if it was instanced successfully.
	virtual SharedPtr<FontEffect> InstanceFontEffect(const String& name, const PropertyDictionary& properties) = 0;

	/// Returns the property specification associated with the instancer.
	const PropertySpecification& GetPropertySpecification() const;

protected:
	/// Registers a property for the font effect.
	/// @param[in] property_name The name of the new property (how it is specified through RCSS).
	/// @param[in] default_value The default value to be used.
	/// @param[in] affects_generation True if this property affects the effect's texture data or glyph size, false if not.
	/// @return The new property definition, ready to have parsers attached.
	PropertyDefinition& RegisterProperty(const String& property_name, const String& default_value, bool affects_generation = true);
	/// Registers a shorthand property definition.
	/// @param[in] shorthand_name The name to register the new shorthand property under.
	/// @param[in] property_names A comma-separated list of the properties this definition is shorthand for. The order
	/// in which they are specified here is the order in which the values will be processed.
	/// @param[in] type The type of shorthand to declare.
	/// @return An ID for the new shorthand, or 'Invalid' if the shorthand declaration is invalid.
	ShorthandId RegisterShorthand(const String& shorthand_name, const String& property_names, ShorthandType type);

private:
	PropertySpecification properties;

	// Properties that define the geometry.
	SmallUnorderedSet<PropertyId> volatile_properties;

	friend class Rml::Factory;
};

} // namespace Rml
