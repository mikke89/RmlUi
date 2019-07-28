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

#ifndef RMLUICOREELEMENTDEFINITION_H
#define RMLUICOREELEMENTDEFINITION_H

#include "../../Include/RmlUi/Core/Dictionary.h"
#include "../../Include/RmlUi/Core/ReferenceCountable.h"

namespace Rml {
namespace Core {

class StyleSheetNode;
class ElementDefinitionIterator;

/**
	ElementDefinition provides an element's applicable properties from its stylesheet.

	@author Peter Curry
 */

class ElementDefinition : public NonCopyMoveable
{
public:
	ElementDefinition(const std::vector< const StyleSheetNode* >& style_sheet_nodes);

	/// Returns a specific property from the element definition's base properties.
	/// @param[in] name The name of the property to return.
	/// @param[in] pseudo_classes The pseudo-classes currently active on the calling element.
	/// @return The property defined against the give name, or NULL if no such property was found.
	const Property* GetProperty(PropertyId id) const;

	/// Returns the list of properties this element definition defines for an element with the given set of
	/// pseudo-classes.
	/// @param[out] property_names The list to store the defined properties in.
	/// @param[in] pseudo_classes The pseudo-classes defined on the querying element.
	void GetDefinedProperties(PropertyNameList& property_names) const;

	const PropertyDictionary& GetProperties() const { return properties; }

private:
	// The attributes for the default state of the element, with no pseudo-classes.
	PropertyDictionary properties;
};

}
}

#endif
