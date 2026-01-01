#pragma once

#include "Header.h"
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

	/// Returns the number of properties in the dictionary.
	int GetNumProperties() const;
	/// Returns the map of properties in the dictionary.
	const PropertyMap& GetProperties() const;

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

	/// Set the source of all properties in the dictionary to the given one.
	void SetSourceOfAllProperties(const SharedPtr<const PropertySource>& property_source);

private:
	// Sets a property on the dictionary and its specificity if there is no name conflict, or its
	// specificity (given by the parameter, not read from the property itself) is at least equal to
	// the specificity of the conflicting property.
	void SetProperty(PropertyId id, const Property& property, int specificity);

	PropertyMap properties;
};

} // namespace Rml
