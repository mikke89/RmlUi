#pragma once

#include "Header.h"
#include "ID.h"
#include "Property.h"

namespace Rml {

/**
    A dictionary to property names to values.
 */

class RMLUICORE_API PropertyDictionary {
public:
	PropertyDictionary();

	/// Sets a property on the dictionary. Any existing property with the same id will be overwritten.
	void SetProperty(PropertyId id, const Property& property);
	/// Removes a property from the dictionary, if it exists.
	void RemoveProperty(PropertyId id);
	/// Returns the value of the property with the requested id, if one exists.
	const Property* GetProperty(PropertyId id) const;

	/// Sets a custom property on the dictionary.
	void SetCustomProperty(String name, Property property);
	/// Removes a custom property from the dictionary, if it exists. Returns true if it existed.
	bool RemoveCustomProperty(const String& name);
	/// Returns the value of the custom property with the requested name, if one exists.
	const Property* GetCustomProperty(const String& name) const;

	/// Sets a var shorthand (shorthand with var expression) on the dictionary.
	void SetVarShorthand(ShorthandId id, Property property);
	/// Removes a var shorthand from the dictionary, if it exists. Returns true if it existed.
	bool RemoveVarShorthand(ShorthandId id);
	/// Returns the var shorthand value with the requested id, if one exists.
	const Property* GetVarShorthand(ShorthandId id) const;

	/// Returns true if the dictionary contains no properties, custom properties, or var shorthands.
	bool Empty() const;
	/// Returns the number of plain properties in the dictionary, excludes custom properties and shorthands.
	int GetNumProperties() const;
	/// Returns the map of plain properties in the dictionary.
	const PropertyMap& GetProperties() const;
	/// Returns the map of custom properties in the dictionary.
	const UnorderedMap<String, Property>& GetCustomProperties() const;
	/// Returns the map of var shorthands in the dictionary.
	const UnorderedMap<ShorthandId, Property>& GetVarShorthands() const;

	/// Imports into the dictionary, and optionally defines the specificity of, potentially
	/// un-specified properties. In the case of id conflicts, the incoming properties will
	/// overwrite the existing properties if their specificity (or their forced specificity)
	/// are at least equal.
	/// @param[in] property_dictionary The properties to import.
	/// @param[in] specificity The specificity for all incoming properties. If this is not specified, the properties will keep their original
	/// specificity.
	void Import(const PropertyDictionary& property_dictionary, int specificity = -1);

	/// Merges the contents of another fully-specified property dictionary with this one.
	/// Properties defined in the new dictionary will overwrite those with the same id as
	/// appropriate.
	/// @param[in] property_dictionary The dictionary to merge.
	/// @param[in] specificity_offset The specificities of all incoming properties will be offset by this value.
	void Merge(const PropertyDictionary& property_dictionary, int specificity_offset = 0);

	/// Clears the dictionary.
	void Clear();

	/// Set the source of all properties in the dictionary to the given one.
	void SetSourceOfAllProperties(const SharedPtr<const PropertySource>& property_source);

private:
	// Sets a property on the dictionary and its specificity if there is no name conflict, or its
	// specificity (given by the parameter, not read from the property itself) is at least equal to
	// the specificity of the conflicting property.
	void SetProperty(PropertyId id, const Property& property, int specificity);

	// Sets a custom property on the dictionary and its specificity as in `SetProperty(..., specificity)`.
	void SetCustomProperty(const String& name, const Property& property, int specificity);

	// Sets a var shorthand on the dictionary and its specificity as in `SetProperty(..., specificity)`.
	void SetVarShorthand(ShorthandId id, const Property& property, int specificity);

	PropertyMap properties;

	UnorderedMap<String, Property> custom_properties;
	UnorderedMap<ShorthandId, Property> var_shorthands;
};

} // namespace Rml
