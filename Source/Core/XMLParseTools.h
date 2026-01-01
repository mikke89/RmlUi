#pragma once

#include "../../Include/RmlUi/Core/Types.h"

namespace Rml {

class Element;

/**
    Tools for aiding in parsing XML documents.
 */

class XMLParseTools {
public:
	/// Searchs a string for the specified tag
	/// @param tag Tag to find, *must* be in lower case
	/// @param string String to search
	/// @param closing_tag Is it the closing tag we're looking for?
	static const char* FindTag(const char* tag, const char* string, bool closing_tag = false);
	/// Reads the next attribute from the string, advancing the pointer
	/// @param[in,out] string String to read the attribute from, pointer will be advanced passed the read
	/// @param[out] name Name of the attribute read
	/// @param[out] value Value of the attribute read
	static bool ReadAttribute(const char*& string, String& name, String& value);

	/// Applies the named template to the specified element
	/// @param element Element to apply the template to
	/// @param template_name Name of the template to apply, in TEMPLATE:ELEMENT_ID form
	/// @returns Element to continue the parse from
	static Element* ParseTemplate(Element* element, const String& template_name);

	/// Determine the presence of data expression brackets inside XML data.
	/// Call this for each iteration through the data string.
	/// 'inside_brackets' should be initialized to false.
	/// 'inside_string' should be initialized to false.
	/// Returns nullptr on success, or an error string on failure.
	static const char* ParseDataBrackets(bool& inside_brackets, bool& inside_string, char c, char previous);
};

} // namespace Rml
