#pragma once

#include "../../Include/RmlUi/Core/ID.h"

namespace Rml {

enum class ShorthandType;
class PropertyDefinition;
struct ShorthandDefinition;

enum class ShorthandItemType { Invalid, Property, Shorthand };

// Each entry in a shorthand points either to another shorthand or a property
struct ShorthandItem {
	ShorthandItem() : type(ShorthandItemType::Invalid), property_id(PropertyId::Invalid), property_definition(nullptr) {}
	ShorthandItem(PropertyId id, const PropertyDefinition* definition, bool optional, bool repeats) :
		type(ShorthandItemType::Property), optional(optional), repeats(repeats), property_id(id), property_definition(definition)
	{}
	ShorthandItem(ShorthandId id, const ShorthandDefinition* definition, bool optional, bool repeats) :
		type(ShorthandItemType::Shorthand), optional(optional), repeats(repeats), shorthand_id(id), shorthand_definition(definition)
	{}

	ShorthandItemType type = ShorthandItemType::Invalid;
	bool optional = false;
	bool repeats = false;
	union {
		PropertyId property_id;
		ShorthandId shorthand_id;
	};
	union {
		const PropertyDefinition* property_definition;
		const ShorthandDefinition* shorthand_definition;
	};
};

// A list of shorthands or properties
using ShorthandItemList = Vector<ShorthandItem>;

struct ShorthandDefinition {
	ShorthandId id;
	ShorthandItemList items;
	ShorthandType type;
};

} // namespace Rml
