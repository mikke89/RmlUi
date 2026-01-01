#include "ElementDefinition.h"
#include "../../Include/RmlUi/Core/PropertyIdSet.h"
#include "StyleSheetNode.h"

namespace Rml {

ElementDefinition::ElementDefinition(const Vector<const StyleSheetNode*>& style_sheet_nodes)
{
	// Initialises the element definition from the list of style sheet nodes.
	for (size_t i = 0; i < style_sheet_nodes.size(); ++i)
		properties.Merge(style_sheet_nodes[i]->GetProperties());

	for (auto& property : properties.GetProperties())
		property_ids.Insert(property.first);
}

const Property* ElementDefinition::GetProperty(PropertyId id) const
{
	return properties.GetProperty(id);
}

const PropertyIdSet& ElementDefinition::GetPropertyIds() const
{
	return property_ids;
}

} // namespace Rml
