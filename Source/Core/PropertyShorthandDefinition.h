/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019-2023 The RmlUi Team, and contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#ifndef RMLUI_CORE_PROPERTYSHORTHANDDEFINITION_H
#define RMLUI_CORE_PROPERTYSHORTHANDDEFINITION_H

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
#endif
