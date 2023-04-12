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

#ifndef RMLUI_CORE_XMLPARSETOOLS_H
#define RMLUI_CORE_XMLPARSETOOLS_H

#include "../../Include/RmlUi/Core/Types.h"

namespace Rml {

class Element;

/**
    Tools for aiding in parsing XML documents.

    @author Lloyd Weehuizen
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
#endif
