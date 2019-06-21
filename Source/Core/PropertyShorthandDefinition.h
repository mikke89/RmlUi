/*
 * This source file is part of libRocket, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://www.librocket.com
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
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

#ifndef ROCKETCOREPROPERTYSHORTHANDDEFINITION_H
#define ROCKETCOREPROPERTYSHORTHANDDEFINITION_H

#include "../../Include/Rocket/Core/PropertySpecification.h"

namespace Rocket {
namespace Core {

class PropertyDefinition;

/**
	@author Peter Curry
 */



struct ShorthandItem {
	ShorthandItem() : type(ShorthandItemType::Invalid), property_id(PropertyId::Invalid), property_definition(nullptr) {}
	ShorthandItem(PropertyId id, const PropertyDefinition* definition) : type(ShorthandItemType::Property), property_id(id), property_definition(definition) {}
	ShorthandItem(ShorthandId id, const ShorthandDefinition* definition) : type(ShorthandItemType::Shorthand), shorthand_id(id), shorthand_definition(definition) {}

	ShorthandItemType type;
	union {
		struct {
			PropertyId property_id;
			const PropertyDefinition* property_definition;
		};
		struct {
			ShorthandId shorthand_id;
			const ShorthandDefinition* shorthand_definition;
		};
	};
};

// A list of shorthands or properties
typedef std::vector< ShorthandItem > ShorthandItemList;

struct ShorthandDefinition
{
	ShorthandId id;
	ShorthandItemList items; 
	ShorthandType type;
};

}
}

#endif
