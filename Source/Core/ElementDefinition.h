#pragma once

#include "../../Include/RmlUi/Core/PropertyDictionary.h"
#include "../../Include/RmlUi/Core/PropertyIdSet.h"
#include "../../Include/RmlUi/Core/Traits.h"

namespace Rml {

class StyleSheetNode;
class ElementDefinitionIterator;

/**
    ElementDefinition provides an element's applicable properties from its stylesheet.
 */

class ElementDefinition : public NonCopyMoveable {
public:
	ElementDefinition(const Vector<const StyleSheetNode*>& style_sheet_nodes);

	/// Returns a specific property from the element definition.
	/// @param[in] id The id of the property to return.
	/// @return The property defined against the give name, or nullptr if no such property was found.
	const Property* GetProperty(PropertyId id) const;

	/// Returns the list of property ids this element definition defines.
	const PropertyIdSet& GetPropertyIds() const;

	const PropertyDictionary& GetProperties() const { return properties; }

private:
	PropertyDictionary properties;
	PropertyIdSet property_ids;
};

} // namespace Rml
