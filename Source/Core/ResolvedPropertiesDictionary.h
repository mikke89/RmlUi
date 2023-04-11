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

struct IdWrapper {
	enum Type : uint8_t {
		Variable, Shorthand, Property
	} type;
	
	String variable;
	PropertyId property;
	ShorthandId shorthand;
	
	IdWrapper() {}
	IdWrapper(String     variable_name) : type(Variable), variable(variable_name) {}
	IdWrapper(ShorthandId shorthand_id) : type(Shorthand), shorthand(shorthand_id) {}
	IdWrapper(PropertyId   property_id) : type(Property), property(property_id) {}
	
	template<typename T> T const& get() const;
	
	bool operator==(IdWrapper const& o) const {
		if (type != o.type) return false;
		switch(type) {
		case Variable: return variable == o.variable;
		case Shorthand: return shorthand == o.shorthand;
		case Property: return property == o.property;
		}
		return false;
	}
	bool operator<(IdWrapper const& o) const {
		if (type != o.type) return type < o.type;
		switch(type) {
		case Variable: return variable < o.variable;
		case Shorthand: return shorthand < o.shorthand;
		case Property: return property < o.property;
		}
		return false;
	}
};

template<> inline      String const& IdWrapper::get() const { return variable; }
template<> inline ShorthandId const& IdWrapper::get() const { return shorthand; }
template<> inline  PropertyId const& IdWrapper::get() const { return property; }


} // namespace Rml

namespace std {
// Hash specialization for the node list, so it can be used as key in UnorderedMap.
template <>
struct hash<::Rml::IdWrapper> {
	std::size_t operator()(const ::Rml::IdWrapper& v) const noexcept
	{
		switch(v.type) {
		case ::Rml::IdWrapper::Variable: return std::hash<::Rml::String>()(v.variable);
		case ::Rml::IdWrapper::Property: return static_cast<size_t>(v.property);
		case ::Rml::IdWrapper::Shorthand: return static_cast<size_t>(v.shorthand);
		default: return 0;
		}
	}
};
} // namespace std


namespace Rml {

/**
    Wrapper around a property dictionary computing variable-dependent values
    @author Maximilian Stark
 */

class ElementStyle;
class ElementDefinition;


class ResolvedPropertiesDictionary {
public:
	ResolvedPropertiesDictionary(ElementStyle* parent, const ElementDefinition* source = nullptr);

	const Property* GetProperty(PropertyId id) const;
	const Property* GetPropertyVariable(String const& name) const;

	void SetProperty(PropertyId id, const Property& value);
	void SetPropertyVariable(String const& name, const Property& value);
	void SetDependentShorthand(ShorthandId id, const PropertyVariableTerm& value);

	bool RemoveProperty(PropertyId id);
	bool RemovePropertyVariable(String const& name);
	
	bool AnyPropertiesDirty() const;
	
	const PropertyDictionary& GetProperties() const;
	void ResolveDirtyValues();

private:
	void ResolveVariableTerm(String& result, const PropertyVariableTerm& term);
	void ResolveProperty(PropertyId id);
	void ResolveShorthand(ShorthandId id);
	void ResolvePropertyVariable(String const& name);
	void UpdatePropertyDependencies(PropertyId id);
	void UpdateShorthandDependencies(ShorthandId id);
	void UpdatePropertyVariableDependencies(String const& name);

	ElementStyle* parent;

	PropertyDictionary source_properties;
	PropertyDictionary resolved_properties;
	
	UnorderedSet<IdWrapper> dirty_values;
	// key: dependency variable name, value: dependent value
	UnorderedMap<String, SmallUnorderedSet<IdWrapper>> dependencies;
};

} // namespace Rml

#endif
