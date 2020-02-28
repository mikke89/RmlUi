/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019 The RmlUi Team, and contributors
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

#ifndef RMLUICOREPROPERTYSHORTHANDDEFINITION_H
#define RMLUICOREPROPERTYSHORTHANDDEFINITION_H

#include "../../Include/RmlUi/Core/ID.h"

namespace Rml {
namespace Core {

enum class ShorthandType;
class PropertyDefinition;
struct ShorthandDefinition;

enum class ShorthandItemType { Invalid, Property, Shorthand };

// Each entry in a shorthand points either to another shorthand or a property
struct ShorthandItem {
	ShorthandItem() : type(ShorthandItemType::Invalid), property_id(PropertyId::Invalid), property_definition(nullptr), optional(false) {}
	ShorthandItem(PropertyId id, const PropertyDefinition* definition, bool optional) : type(ShorthandItemType::Property), property_id(id), property_definition(definition), optional(optional) {}
	ShorthandItem(ShorthandId id, const ShorthandDefinition* definition, bool optional) : type(ShorthandItemType::Shorthand), shorthand_id(id), shorthand_definition(definition), optional(optional) {}

	ShorthandItemType type;
	union {
		PropertyId property_id;
		ShorthandId shorthand_id;
	};
	union {
		const PropertyDefinition* property_definition;
		const ShorthandDefinition* shorthand_definition;
	};
	bool optional;
};

// A list of shorthands or properties
using ShorthandItemList = std::vector< ShorthandItem >;

struct ShorthandDefinition
{
	ShorthandId id;
	ShorthandItemList items; 
	ShorthandType type;
};

}
}

#endif
