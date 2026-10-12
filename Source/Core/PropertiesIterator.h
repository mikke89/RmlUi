#pragma once

#include "../../Include/RmlUi/Core/PropertyIdSet.h"
#include "../../Include/RmlUi/Core/Types.h"
#include "ElementStyle.h"
#include "PropertyShorthandDefinition.h"
#include <variant>

namespace Rml {

// An iterator for local properties defined on an element.
// Note: Modifying the underlying style invalidates the iterator.
class PlainPropertiesIterator {
public:
	using ValueType = Pair<PropertyId, const Property&>;
	using PropertyIt = PropertyMap::const_iterator;

	PlainPropertiesIterator(const PropertyMap& inline_properties, const PropertyMap* definition_properties) :
		it_style(inline_properties.begin()), it_style_end(inline_properties.end()), it_definition{}, it_definition_end{}
	{
		if (definition_properties)
		{
			it_definition = definition_properties->begin();
			it_definition_end = definition_properties->end();
		}
		ProceedToNextValid();
	}

	PlainPropertiesIterator& operator++()
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
	PropertyIdSet visited_properties;
	PropertyIt it_style, it_style_end;
	PropertyIt it_definition, it_definition_end;
	bool at_end = false;

	bool IsFirstVisit(PropertyId id)
	{
		if (!visited_properties.Contains(id))
		{
			visited_properties.Insert(id);
			return true;
		}
		return false;
	}

	void ProceedToNextValid()
	{
		for (; it_style != it_style_end; ++it_style)
		{
			if (IsFirstVisit(it_style->first))
				return;
		}

		for (; it_definition != it_definition_end; ++it_definition)
		{
			if (IsFirstVisit(it_definition->first))
				return;
		}

		// All iterators are now at the end
		at_end = true;
	}
};

class AllPropertiesIterator {
public:
	using ValueType = Pair<String, const Property&>;
	using PlainIt = PropertyMap::const_iterator;
	using CustomIt = UnorderedMap<String, Property>::const_iterator;
	using ShorthandIt = UnorderedMap<ShorthandId, Property>::const_iterator;

	AllPropertiesIterator(const PropertyDictionary& inline_properties, const PropertyDictionary* definition_properties,
		ElementStyle* filter_inherited_by) :
		inline_properties{inline_properties}, definition_properties{definition_properties}, filter_inherited_by{filter_inherited_by}
	{
		SetIterators(inline_properties);
		UpdatePhase();
		SkipFilter();
	}

	AllPropertiesIterator& operator++()
	{
		Increment();
		UpdatePhase();
		SkipFilter();
		return *this;
	}

	ValueType operator*() const
	{
		if (it_plain != it_plain_end)
			return {StyleSheetSpecification::GetPropertyName(it_plain->first), it_plain->second};
		if (it_custom != it_custom_end)
			return {it_custom->first, it_custom->second};
		RMLUI_ASSERT(it_shorthand != it_shorthand_end);
		return {StyleSheetSpecification::GetShorthandName(it_shorthand->first), it_shorthand->second};
	}

	bool AtEnd() const { return phase == Phase::End; }

private:
	void Increment()
	{
		RMLUI_ASSERT(!AtEnd());
		if (it_plain != it_plain_end)
			++it_plain;
		else if (it_custom != it_custom_end)
			++it_custom;
		else if (it_shorthand != it_shorthand_end)
			++it_shorthand;
	}

	void SkipFilter()
	{
		while (true)
		{
			bool increment = false;
			if (HasVisitedCurrent())
			{
				increment = true;
			}
			else if (filter_inherited_by && !AtEnd())
			{
				if (it_plain != it_plain_end)
					increment = (filter_inherited_by->GetSpecifiedProperty(it_plain->first) != &it_plain->second);
				else if (it_custom != it_custom_end)
					increment = (filter_inherited_by->GetSpecifiedCustomProperty(it_custom->first) != &it_custom->second);
				else if (it_shorthand != it_shorthand_end)
					increment = (filter_inherited_by->GetSpecifiedShorthand(it_shorthand->first) != &it_shorthand->second);
			}

			if (!increment && it_plain != it_plain_end)
			{
				// Skip shorthand placeholders, they are defined directly by shorthands which are iterated over later.
				increment = (it_plain->second.unit == Unit::SHORTHAND_PLACEHOLDER);
			}

			if (increment)
			{
				Increment();
				UpdatePhase();
			}
			else
			{
				break;
			}
		}
	}

	void UpdatePhase()
	{
		if (it_plain != it_plain_end)
			return;
		if (it_custom != it_custom_end)
			return;
		if (it_shorthand != it_shorthand_end)
			return;

		if (phase == Phase::InlineProperties && definition_properties)
		{
			SetIterators(*definition_properties);
			phase = Phase::DefinitionProperties;
			UpdatePhase();
		}
		else
		{
			phase = Phase::End;
		}
	}

	bool HasVisitedCurrent() const
	{
		if (phase != Phase::DefinitionProperties)
			return false;

		if (it_plain != it_plain_end)
			return inline_properties.GetProperties().count(it_plain->first);
		if (it_custom != it_custom_end)
			return inline_properties.GetCustomProperties().count(it_custom->first);

		RMLUI_ASSERT(it_shorthand != it_shorthand_end);
		return inline_properties.GetVarShorthands().count(it_shorthand->first);
	}

	void SetIterators(const PropertyDictionary& dictionary)
	{
		it_plain = dictionary.GetProperties().begin();
		it_plain_end = dictionary.GetProperties().end();
		it_custom = dictionary.GetCustomProperties().begin();
		it_custom_end = dictionary.GetCustomProperties().end();
		it_shorthand = dictionary.GetVarShorthands().begin();
		it_shorthand_end = dictionary.GetVarShorthands().end();
	}

	enum class Phase { InlineProperties, DefinitionProperties, End };
	Phase phase = Phase::InlineProperties;

	const PropertyDictionary& inline_properties;
	const PropertyDictionary* definition_properties;
	ElementStyle* filter_inherited_by;

	PlainIt it_plain, it_plain_end;
	CustomIt it_custom, it_custom_end;
	ShorthandIt it_shorthand, it_shorthand_end;
};

} // namespace Rml
