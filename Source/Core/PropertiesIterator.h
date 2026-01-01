#pragma once

#include "../../Include/RmlUi/Core/PropertyIdSet.h"
#include "../../Include/RmlUi/Core/Types.h"

namespace Rml {

// An iterator for local properties defined on an element.
// Note: Modifying the underlying style invalidates the iterator.
class PropertiesIterator {
public:
	using ValueType = Pair<PropertyId, const Property&>;
	using PropertyIt = PropertyMap::const_iterator;

	PropertiesIterator(PropertyIt it_style, PropertyIt it_style_end, PropertyIt it_definition, PropertyIt it_definition_end) :
		it_style(it_style), it_style_end(it_style_end), it_definition(it_definition), it_definition_end(it_definition_end)
	{
		ProceedToNextValid();
	}

	PropertiesIterator& operator++()
	{
		if (it_style != it_style_end)
			// We iterate over the local style properties first
			++it_style;
		else
			// .. and then over the properties given by the element's definition
			++it_definition;
		// If we reached the end of one of the iterator pairs, we need to continue iteration on the next pair.
		ProceedToNextValid();
		return *this;
	}

	ValueType operator*() const
	{
		if (it_style != it_style_end)
			return {it_style->first, it_style->second};
		return {it_definition->first, it_definition->second};
	}

	bool AtEnd() const { return at_end; }

private:
	PropertyIdSet iterated_properties;
	PropertyIt it_style, it_style_end;
	PropertyIt it_definition, it_definition_end;
	bool at_end = false;

	inline bool IsDirtyRemove(PropertyId id)
	{
		if (!iterated_properties.Contains(id))
		{
			iterated_properties.Insert(id);
			return true;
		}
		return false;
	}

	inline void ProceedToNextValid()
	{
		for (; it_style != it_style_end; ++it_style)
		{
			if (IsDirtyRemove(it_style->first))
				return;
		}

		for (; it_definition != it_definition_end; ++it_definition)
		{
			if (IsDirtyRemove(it_definition->first))
				return;
		}

		// All iterators are now at the end
		at_end = true;
	}
};

} // namespace Rml
