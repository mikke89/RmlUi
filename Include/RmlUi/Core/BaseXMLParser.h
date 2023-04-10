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

#ifndef RMLUI_CORE_BASEXMLPARSER_H
#define RMLUI_CORE_BASEXMLPARSER_H

#include "Dictionary.h"
#include "Header.h"
#include "Types.h"

namespace Rml {

class Stream;
class URL;
using XMLAttributes = Dictionary;

enum class XMLDataType { Text, CData, InnerXML };

/**
    @author Peter Curry
 */

class RMLUICORE_API BaseXMLParser {
public:
	BaseXMLParser();
	virtual ~BaseXMLParser();

	/// Registers a tag as containing general character data. This will mean the contents of the tag will be parsed
	/// similarly to a CDATA tag (ie, no other markup will be recognised until the section's closing tag is found).
	/// @param[in] tag The tag to register as containing generic character data.
	void RegisterCDATATag(const String& tag);

	/// When an XML attribute with the given name is encountered during parsing, then all content below the current
	/// node is treated as data.
	/// @note While children nodes are treated as data (text), it is assumed that the content represents valid XML.
	///         The parsing proceeds as normal except that the Handle...() functions are not called until the
	///         starting node is closed. Then, all its contents are submitted as Data (raw text string).
	/// @note In particular, this behavior is useful for some data-binding views.
	void RegisterInnerXMLAttribute(const String& attribute_name);

	/// Parses the given stream as an XML file, and calls the handlers when
	/// interesting phenomena are encountered.
	void Parse(Stream* stream);

	/// Get the line number in the stream.
	/// @return The line currently being processed in the XML stream.
	int GetLineNumber() const;
	/// Get the line number of the last open tag in the stream.
	int GetLineNumberOpenTag() const;

	/// Called when the parser finds the beginning of an element tag.
	virtual void HandleElementStart(const String& name, const XMLAttributes& attributes);
	/// Called when the parser finds the end of an element tag.
	virtual void HandleElementEnd(const String& name);
	/// Called when the parser encounters data.
	virtual void HandleData(const String& data, XMLDataType type);

protected:
	const URL* GetSourceURLPtr() const;

private:
	const URL* source_url = nullptr;
	String xml_source;
	size_t xml_index = 0;

	void Next();
	bool AtEnd() const;
	char Look() const;

	void HandleElementStartInternal(const String& name, const XMLAttributes& attributes);
	void HandleElementEndInternal(const String& name);
	void HandleDataInternal(const String& data, XMLDataType type);

	void ReadHeader();
	void ReadBody();
	bool ReadOpenTag();

	bool ReadCloseTag(size_t xml_index_tag);
	bool ReadAttributes(XMLAttributes& attributes, bool& parse_raw_xml_content);
	bool ReadCDATA(const char* tag_terminator = nullptr);

	// Reads from the stream until a complete word is found.
	// @param[out] word Word thats been found
	// @param[in] terminators List of characters that terminate the search
	bool FindWord(String& word, const char* terminators = nullptr);
	// Reads from the stream until the given character set is found. All
	// intervening characters will be returned in data.
	bool FindString(const char* string, String& data, bool escape_brackets = false);
	// Returns true if the next sequence of characters in the stream
	// matches the given string. If consume is set and this returns true,
	// the characters will be consumed.
	bool PeekString(const char* string, bool consume = true);

	int line_number = 0;
	int line_number_open_tag = 0;
	int open_tag_depth = 0;

	// Enabled when an attribute for inner xml data is encountered (see description in Register...() above).
	bool inner_xml_data = false;
	int inner_xml_data_terminate_depth = 0;
	size_t inner_xml_data_index_begin = 0;

	// The element attributes being read.
	XMLAttributes attributes;
	// The loose data being read.
	String data;

	SmallUnorderedSet<String> cdata_tags;
	SmallUnorderedSet<String> attributes_for_inner_xml_data;
};

} // namespace Rml
#endif
