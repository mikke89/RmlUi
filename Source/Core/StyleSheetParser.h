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

#ifndef RMLUI_CORE_STYLESHEETPARSER_H
#define RMLUI_CORE_STYLESHEETPARSER_H

#include "../../Include/RmlUi/Core/StyleSheetTypes.h"
#include "../../Include/RmlUi/Core/Types.h"

namespace Rml {

class PropertyDictionary;
class Stream;
class StyleSheetNode;
class AbstractPropertyParser;
struct PropertySource;
using StyleSheetNodeListRaw = Vector<StyleSheetNode*>;

/**
    Helper class for parsing a style sheet into its memory representation.

    @author Lloyd Weehuizen
 */

class StyleSheetParser {
public:
	StyleSheetParser();
	~StyleSheetParser();

	/// Parses the given stream into the style sheet
	/// @param style_sheets The collection of style sheets to write into, organized into media blocks
	/// @param stream The stream to read
	/// @param begin_line_number The used line number for the first line in the stream, for reporting errors.
	/// @return True on success, false on failure.
	bool Parse(MediaBlockList& style_sheets, Stream* stream, int begin_line_number);

	/// Parses the given string into the property dictionary
	/// @param parsed_properties The properties dictionary the properties will be read into
	/// @param properties The properties to parse
	/// @return True if the parse was successful, or false if an error occured.
	bool ParseProperties(PropertyDictionary& parsed_properties, const String& properties);

	// Converts a selector query to a tree of nodes.
	// @param root_node Node to construct into.
	// @param selectors The selector rules as a string value.
	// @return The list of leaf nodes in the constructed tree, which are all owned by the root node.
	static StyleSheetNodeListRaw ConstructNodes(StyleSheetNode& root_node, const String& selectors);

	// Initialises property parsers. Call after initialisation of StylesheetSpecification.
	static void Initialise();
	// Reset property parsers.
	static void Shutdown();

private:
	// Stream we're parsing from.
	Stream* stream;
	// Parser memory buffer.
	String parse_buffer;
	// How far we've read through the buffer.
	size_t parse_buffer_pos;

	// The name of the file we're parsing.
	String stream_file_name;
	// Current line number we're parsing.
	int line_number;

	// Parses properties from the parse buffer.
	// @param property_parser An abstract parser which specifies how the properties are parsed and stored.
	bool ReadProperties(AbstractPropertyParser& property_parser);

	// Import properties into the stylesheet node
	// @param node Node to import into
	// @param rule The rule name to parse
	// @param properties The dictionary of properties
	// @param rule_specificity The specifity of the rule
	// @return The leaf node of the rule, or nullptr on parse failure.
	static StyleSheetNode* ImportProperties(StyleSheetNode* node, const String& rule, const PropertyDictionary& properties, int rule_specificity);

	// Attempts to parse a @keyframes block
	bool ParseKeyframeBlock(KeyframesMap& keyframes_map, const String& identifier, const String& rules, const PropertyDictionary& properties);

	// Attempts to parse a @decorator block
	bool ParseDecoratorBlock(const String& at_name, NamedDecoratorMap& named_decorator_map, const SharedPtr<const PropertySource>& source);

    /// Attempts to parse the properties of a @media query.
	/// @param[in] rules The rules to parse.
	/// @param[out] properties Parsed properties representing all values to be matched.
	/// @param[out] modifier Media query modifier.
	bool ParseMediaFeatureMap(const String& rules, PropertyDictionary& properties, MediaQueryModifier &modifier);

	// Attempts to find one of the given character tokens in the active stream
	// If it's found, buffer is filled with all content up until the token
	// @param buffer The buffer that receives the content
	// @param characters The character tokens to find
	// @param remove_token If the token that caused the find to stop should be removed from the stream
	char FindToken(String& buffer, const char* tokens, bool remove_token);

	// Attempts to find the next character in the active stream.
	// If it's found, buffer is filled with the character
	// @param buffer The buffer that receives the character, if read.
	bool ReadCharacter(char& buffer);

	// Fill the internal parse buffer
	bool FillBuffer();
};

} // namespace Rml
#endif
