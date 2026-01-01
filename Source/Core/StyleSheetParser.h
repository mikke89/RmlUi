#pragma once

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
 */

class StyleSheetParser {
public:
	StyleSheetParser();
	~StyleSheetParser();

	/// Parses the given stream into the style sheet
	/// @param[out] style_sheets The collection of style sheets to write into, organized into media blocks
	/// @param[in] stream The stream to read
	/// @param[in] begin_line_number The used line number for the first line in the stream, for reporting errors.
	/// @return True on success, false on failure.
	bool Parse(MediaBlockList& style_sheets, Stream* stream, int begin_line_number);

	/// Parses the given string into the property dictionary
	/// @param[out] parsed_properties The properties dictionary the properties will be read into
	/// @param[in] properties The properties to parse
	/// @return True if the parse was successful, or false if an error occurred.
	void ParseProperties(PropertyDictionary& parsed_properties, const String& properties);

	/// Converts a selector query to a tree of nodes.
	/// @param[out] root_node Node to construct into.
	/// @param[in] selectors The selector rules as a string value.
	/// @return The list of leaf nodes in the constructed tree, which are all owned by the root node.
	static StyleSheetNodeListRaw ConstructNodes(StyleSheetNode& root_node, const String& selectors);

	/// Initialises property parsers. Call after initialisation of StylesheetSpecification.
	static void Initialise();
	/// Reset property parsers.
	static void Shutdown();

private:
	/// Parses properties from the parse buffer.
	/// @param[in-out] property_parser An abstract parser which specifies how the properties are parsed and stored.
	void ReadProperties(AbstractPropertyParser& property_parser);

	/// Import properties into the stylesheet node
	/// @param[out] node Node to import into
	/// @param[in] rule The rule name to parse
	/// @param[in] properties The dictionary of properties
	/// @param[in] rule_specificity The specific of the rule
	/// @return The leaf node of the rule, or nullptr on parse failure.
	static StyleSheetNode* ImportProperties(StyleSheetNode* node, const String& rule, const PropertyDictionary& properties, int rule_specificity);

	/// Attempts to parse a @keyframes block
	bool ParseKeyframeBlock(KeyframesMap& keyframes_map, const String& identifier, const String& rules, const PropertyDictionary& properties);

	/// Attempts to parse a @decorator block
	bool ParseDecoratorBlock(const String& at_name, NamedDecoratorMap& named_decorator_map, const SharedPtr<const PropertySource>& source);

	/// Attempts to parse the properties of a @media query.
	/// @param[in] rules The rules to parse.
	/// @param[out] properties Parsed properties representing all values to be matched.
	/// @param[out] modifier Media query modifier.
	bool ParseMediaFeatureMap(const String& rules, PropertyDictionary& properties, MediaQueryModifier& modifier);

	/// Attempts to find one of the given character tokens in the active stream.
	/// If it's found, buffer is filled with all content up until the token, cursor advances past the token.
	/// @param[out] buffer The buffer that receives the content.
	/// @param[in] tokens The character tokens to find.
	/// @return The found token character, or '\0' if none was found.
	char FindAnyToken(String& buffer, const char* tokens);

	/// Attempts to find the next character in the active stream.
	/// If it's found, buffer is filled with the character
	/// @param[out] buffer The buffer that receives the character, if read.
	/// @return True if a character was read, false on end of stream.
	bool ReadCharacter(char& buffer);

	/// Fill the internal parse buffer
	bool FillBuffer();

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
};

} // namespace Rml
