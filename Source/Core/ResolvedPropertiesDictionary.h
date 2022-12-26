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

#ifndef RMLUI_CORE_RESOLVEDPROPERTIESDICTIONARY_H
#define RMLUI_CORE_RESOLVEDPROPERTIESDICTIONARY_H

#include "../../Include/RmlUi/Core/PropertyDictionary.h"
#include "../../Include/RmlUi/Core/Types.h"

namespace Rml {

/**
    Wrapper around a property dictionary computing variable-dependent values
    @author Maximilian Stark
 */

class ElementStyle;
class ElementDefinition;

class ResolvedPropertiesDictionary {
public:
	ResolvedPropertiesDictionary(ElementStyle* parent);
	ResolvedPropertiesDictionary(ElementStyle* parent, const ElementDefinition* source);

	const Property* GetProperty(PropertyId id) const;
	const Property* GetVariable(VariableId id) const;

	void SetProperty(PropertyId id, const Property& value);
	void SetVariable(VariableId id, const Property& value);

	bool RemoveProperty(PropertyId id);
	bool RemoveVariable(VariableId id);

	const PropertyDictionary& GetProperties() const;

private:
	void ResolveVariableTerm(String& result, const VariableTerm& term);
	void ResolveProperty(PropertyId id);
	void ResolveVariable(VariableId id);
	void UpdatePropertyDependencies(PropertyId id);
	void UpdateVariableDependencies(VariableId id);

	ElementStyle* parent;

	PropertyDictionary source_properties;
	PropertyDictionary resolved_properties;

	UnorderedMultimap<VariableId, PropertyId> property_dependencies;
	UnorderedMultimap<VariableId, VariableId> variable_dependencies;
};

} // namespace Rml
#endif
