#include "../../Include/RmlUi/Core/PropertyDictionary.h"
#include "../../Include/RmlUi/Core/ID.h"

namespace Rml {

PropertyDictionary::PropertyDictionary() {}

void PropertyDictionary::SetProperty(PropertyId id, const Property& property)
{
	RMLUI_ASSERT(id != PropertyId::Invalid);
	properties[id] = property;
}

void PropertyDictionary::RemoveProperty(PropertyId id)
{
	RMLUI_ASSERT(id != PropertyId::Invalid);
	properties.erase(id);
}

const Property* PropertyDictionary::GetProperty(PropertyId id) const
{
	PropertyMap::const_iterator iterator = properties.find(id);
	if (iterator == properties.end())
		return nullptr;

	return &(*iterator).second;
}

int PropertyDictionary::GetNumProperties() const
{
	return (int)properties.size();
}

const PropertyMap& PropertyDictionary::GetProperties() const
{
	return properties;
}

void PropertyDictionary::Import(const PropertyDictionary& other, int property_specificity)
{
	for (const auto& pair : other.properties)
	{
		const PropertyId id = pair.first;
		const Property& property = pair.second;
		SetProperty(id, property, property_specificity > 0 ? property_specificity : property.specificity);
	}
}

void PropertyDictionary::Merge(const PropertyDictionary& other, int specificity_offset)
{
	for (const auto& pair : other.properties)
	{
		const PropertyId id = pair.first;
		const Property& property = pair.second;
		SetProperty(id, property, property.specificity + specificity_offset);
	}
}

void PropertyDictionary::SetSourceOfAllProperties(const SharedPtr<const PropertySource>& property_source)
{
	for (auto& p : properties)
		p.second.source = property_source;
}

void PropertyDictionary::SetProperty(PropertyId id, const Property& property, int specificity)
{
	PropertyMap::iterator iterator = properties.find(id);
	if (iterator != properties.end() && iterator->second.specificity > specificity)
		return;

	Property& new_property = (properties[id] = property);
	new_property.specificity = specificity;
}

} // namespace Rml
