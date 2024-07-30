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

#ifndef RMLUI_CORE_PROPERTYSPECIFICATION_H
#define RMLUI_CORE_PROPERTYSPECIFICATION_H

#include "Header.h"
#include "ID.h"
#include "PropertyIdSet.h"
#include "Types.h"

namespace Rml {

class StyleSheetSpecification;
class PropertyDefinition;
class PropertyDictionary;
class PropertyIdNameMap;
class ShorthandIdNameMap;
struct ShorthandDefinition;

enum class ShorthandType {
	// Normal; properties that fail to parse fall-through to the next until they parse correctly, and any
	// undeclared are not set.
	FallThrough,
	// A single failed parse will abort, and any undeclared are replicated from the last declared property.
	Replicate,
	// For 'padding', 'margin', etc; up to four properties are expected.
	Box,
	// Repeatedly resolves the full value string on each property, whether it is a normal property or another shorthand.
	RecursiveRepeat,
	// Comma-separated list of properties or shorthands, the number of declared values must match the specified.
	RecursiveCommaSeparated,
	// The 'flex' shorthand has some special behavior but otherwise acts like 'FallThrough'.
	Flex
};

/**
    A property specification stores a group of property definitions.

    @author Peter Curry
 */

class RMLUICORE_API PropertySpecification {
public:
	PropertySpecification(size_t reserve_num_properties, size_t reserve_num_shorthands);
	~PropertySpecification();

	/// Registers a property with a new definition.
	/// @param[in] property_name The name to register the new property under.
	/// @param[in] default_value The default value to be used for an element if it has no other definition provided.
	/// @param[in] inherited True if this property is inherited from parent to child, false otherwise.
	/// @param[in] forces_layout True if this property requires its parent to be reformatted if changed.
	/// @param[in] id If 'Invalid' then automatically assigns a new id, otherwise assigns the given id.
	/// @return The new property definition, ready to have parsers attached.
	PropertyDefinition& RegisterProperty(const String& property_name, const String& default_value, bool inherited, bool forces_layout,
		PropertyId id = PropertyId::Invalid);
	/// Returns a property definition.
	/// @param[in] id The id of the desired property.
	/// @return The appropriate property definition if it could be found, nullptr otherwise.
	const PropertyDefinition* GetProperty(PropertyId id) const;
	const PropertyDefinition* GetProperty(const String& property_name) const;

	/// Returns the id set of all registered property definitions.
	const PropertyIdSet& GetRegisteredProperties() const;
	/// Returns the id set of all registered inherited property definitions.
	const PropertyIdSet& GetRegisteredInheritedProperties() const;
	/// Returns the id set of all registered property definitions that may dirty the layout.
	const PropertyIdSet& GetRegisteredPropertiesForcingLayout() const;

	/// Registers a shorthand property definition.
	/// @param[in] shorthand_name The name to register the new shorthand property under.
	/// @param[in] property_names A comma-separated list of the properties this definition is shorthand for. The order
	/// in which they are specified here is the order in which the values will be processed.
	/// @param[in] type The type of shorthand to declare.
	/// @param[in] id If 'Invalid' then automatically assigns a new id, otherwise assigns the given id.
	/// @return An ID for the new shorthand, or 'Invalid' if the shorthand declaration is invalid.
	ShorthandId RegisterShorthand(const String& shorthand_name, const String& property_names, ShorthandType type,
		ShorthandId id = ShorthandId::Invalid);
	/// Returns a shorthand definition.
	/// @param[in] id The id of the desired shorthand.
	/// @return The appropriate shorthand definition if it could be found, nullptr otherwise.
	const ShorthandDefinition* GetShorthand(ShorthandId id) const;
	const ShorthandDefinition* GetShorthand(const String& shorthand_name) const;

	/// Parse declaration by name, whether it's a property or shorthand.
	bool ParsePropertyDeclaration(PropertyDictionary& dictionary, const String& property_name, const String& property_value) const;
	/// Parse property declaration by ID.
	bool ParsePropertyDeclaration(PropertyDictionary& dictionary, PropertyId property_id, const String& property_value) const;
	/// Parses a shorthand declaration, setting any parsed and validated properties on the given dictionary.
	/// @return True if all properties were parsed successfully, false otherwise.
	bool ParseShorthandDeclaration(PropertyDictionary& dictionary, ShorthandId shorthand_id, const String& property_value) const;

	/// Sets all undefined properties in the dictionary to their defaults.
	/// @param dictionary[in-out] The dictionary to set the default values on.
	void SetPropertyDefaults(PropertyDictionary& dictionary) const;

	/// Returns the properties of dictionary converted to a string.
	String PropertiesToString(const PropertyDictionary& dictionary, bool include_name, char delimiter) const;

private:
	using Properties = Vector<UniquePtr<PropertyDefinition>>;
	using Shorthands = Vector<UniquePtr<ShorthandDefinition>>;

	Properties properties;
	Shorthands shorthands;

	UniquePtr<PropertyIdNameMap> property_map;
	UniquePtr<ShorthandIdNameMap> shorthand_map;

	PropertyIdSet property_ids;
	PropertyIdSet property_ids_inherited;
	PropertyIdSet property_ids_forcing_layout;

	enum class SplitOption { None, Whitespace, Comma };
	bool ParsePropertyValues(StringList& values_list, const String& values, SplitOption split_option) const;

	friend class Rml::StyleSheetSpecification;
	friend class TestPropertySpecification;
};

} // namespace Rml
#endif
