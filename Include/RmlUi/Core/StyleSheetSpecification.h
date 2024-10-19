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

#ifndef RMLUI_CORE_STYLESHEETSPECIFICATION_H
#define RMLUI_CORE_STYLESHEETSPECIFICATION_H

#include "Header.h"
#include "PropertySpecification.h"
#include "Types.h"

namespace Rml {

class PropertyParser;
struct DefaultStyleSheetParsers;

/**
    @author Peter Curry
 */

class RMLUICORE_API StyleSheetSpecification {
public:
	/// Starts up the specification structure and registers default properties and type parsers.
	static void Initialise();
	/// Destroys the specification structure and releases the parsers.
	static void Shutdown();

	/// Registers a parser for use in property definitions.
	/// @param[in] parser_name The name to register the new parser under.
	/// @param[in] parser A non-owning pointer to the parser to register.
	/// @return True if the parser was registered successfully, false otherwise.
	/// @lifetime The parser must be kept alive until after the call to Rml::Shutdown.
	static bool RegisterParser(const String& parser_name, PropertyParser* parser);
	/// Returns the parser registered with a specific name.
	/// @param[in] parser_name The name of the desired parser.
	/// @return The parser registered under the given name, or nullptr if no such parser exists.
	static PropertyParser* GetParser(const String& parser_name);

	/// Registers a custom property with a new definition.
	/// @param[in] property_name The name to register the new property under.
	/// @param[in] default_value The default value to be used for an element if it has no other definition provided.
	/// @param[in] inherited True if this property is inherited from parent to child, false otherwise.
	/// @param[in] forces_layout True if a change in this property on an element will cause the element's layout to possibly change.
	/// @return The new property definition, ready to have parsers attached.
	static PropertyDefinition& RegisterProperty(const String& property_name, const String& default_value, bool inherited, bool forces_layout = false);
	/// Returns a property definition.
	/// @param[in] property_name The name of the desired property.
	/// @return The appropriate property definition if it could be found, nullptr otherwise.
	static const PropertyDefinition* GetProperty(const String& property_name);
	static const PropertyDefinition* GetProperty(PropertyId id);

	/// Returns the id set of all registered property definitions.
	static const PropertyIdSet& GetRegisteredProperties();
	/// Returns the id set of all registered inherited property definitions.
	static const PropertyIdSet& GetRegisteredInheritedProperties();
	/// Returns the id set of all registered property definitions that may dirty the layout.
	static const PropertyIdSet& GetRegisteredPropertiesForcingLayout();

	/// Registers a custom shorthand property definition.
	/// @param[in] shorthand_name The name to register the new shorthand property under.
	/// @param[in] property_names A comma-separated list of the properties this definition is shorthand for. The order in which they are specified
	/// here is the order in which the values will be processed.
	/// @param[in] type The type of shorthand to declare.
	/// @return The new shorthand ID if successfully parsed, Invalid otherwise.
	static ShorthandId RegisterShorthand(const String& shorthand_name, const String& property_names, ShorthandType type);
	/// Returns a shorthand definition.
	/// @param[in] shorthand_name The name of the desired shorthand.
	/// @return The appropriate shorthand definition if it could be found, nullptr otherwise.
	static const ShorthandDefinition* GetShorthand(const String& shorthand_name);
	static const ShorthandDefinition* GetShorthand(ShorthandId id);

	/// Parses a property declaration, setting any parsed and validated properties on the given dictionary.
	/// @param[in] dictionary The property dictionary which will hold all declared properties.
	/// @param[in] property_name The name of the declared property.
	/// @param[in] property_value The values the property is being set to.
	/// @return True if all properties were parsed successfully, false otherwise.
	static bool ParsePropertyDeclaration(PropertyDictionary& dictionary, const String& property_name, const String& property_value);

	static PropertyId GetPropertyId(const String& property_name);
	static ShorthandId GetShorthandId(const String& shorthand_name);
	static const String& GetPropertyName(PropertyId id);
	static const String& GetShorthandName(ShorthandId id);

	// Get the underlying property ids associated by a shorthand.
	static PropertyIdSet GetShorthandUnderlyingProperties(ShorthandId id);

	static const PropertySpecification& GetPropertySpecification();

private:
	StyleSheetSpecification();
	~StyleSheetSpecification();

	PropertyDefinition& RegisterProperty(PropertyId id, const String& property_name, const String& default_value, bool inherited,
		bool forces_layout = false);
	ShorthandId RegisterShorthand(ShorthandId id, const String& shorthand_name, const String& property_names, ShorthandType type);

	// Registers RmlUi's default parsers.
	void RegisterDefaultParsers();
	// Registers RmlUi's default style properties.
	void RegisterDefaultProperties();

	// Parsers used by all property definitions.
	typedef UnorderedMap<String, PropertyParser*> ParserMap;
	ParserMap parsers;

	// The properties defined in the style sheet specification.
	PropertySpecification properties;

	UniquePtr<DefaultStyleSheetParsers> default_parsers;
};

} // namespace Rml
#endif
