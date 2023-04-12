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

#include "../../Include/RmlUi/Core/BaseXMLParser.h"
#include "../../Include/RmlUi/Core/Profiling.h"
#include "../../Include/RmlUi/Core/Stream.h"
#include "XMLParseTools.h"
#include <string.h>

namespace Rml {

BaseXMLParser::BaseXMLParser() {}

BaseXMLParser::~BaseXMLParser() {}

void BaseXMLParser::RegisterCDATATag(const String& tag)
{
	if (!tag.empty())
		cdata_tags.insert(StringUtilities::ToLower(tag));
}

void BaseXMLParser::RegisterInnerXMLAttribute(const String& attribute_name)
{
	attributes_for_inner_xml_data.insert(attribute_name);
}

void BaseXMLParser::Parse(Stream* stream)
{
	source_url = &stream->GetSourceURL();

	xml_source.clear();

	// We read in the whole XML file here.
	// TODO: It doesn't look like the Stream interface is used for anything useful. We
	//   might as well just use a span or StringView, and get completely rid of it.
	// @performance Otherwise, use the temporary allocator.
	const size_t source_size = stream->Length();
	stream->Read(xml_source, source_size);

	xml_index = 0;
	line_number = 1;
	line_number_open_tag = 1;

	inner_xml_data = false;
	inner_xml_data_terminate_depth = 0;
	inner_xml_data_index_begin = 0;

	// Read (er ... skip) the header, if one exists.
	ReadHeader();
	// Read the XML body.
	ReadBody();

	xml_source.clear();
	source_url = nullptr;
}

int BaseXMLParser::GetLineNumber() const
{
	return line_number;
}

int BaseXMLParser::GetLineNumberOpenTag() const
{
	return line_number_open_tag;
}

void BaseXMLParser::HandleElementStart(const String& /*name*/, const XMLAttributes& /*attributes*/) {}

void BaseXMLParser::HandleElementEnd(const String& /*name*/) {}

void BaseXMLParser::HandleData(const String& /*data*/, XMLDataType /*type*/) {}

const URL* BaseXMLParser::GetSourceURLPtr() const
{
	return source_url;
}

void BaseXMLParser::Next()
{
	xml_index += 1;
}

bool BaseXMLParser::AtEnd() const
{
	return xml_index >= xml_source.size();
}

char BaseXMLParser::Look() const
{
	RMLUI_ASSERT(!AtEnd());
	return xml_source[xml_index];
}

void BaseXMLParser::HandleElementStartInternal(const String& name, const XMLAttributes& attributes)
{
	line_number_open_tag = line_number;
	if (!inner_xml_data)
		HandleElementStart(name, attributes);
}

void BaseXMLParser::HandleElementEndInternal(const String& name)
{
	if (!inner_xml_data)
		HandleElementEnd(name);
}

void BaseXMLParser::HandleDataInternal(const String& data, XMLDataType type)
{
	if (!inner_xml_data)
		HandleData(data, type);
}

void BaseXMLParser::ReadHeader()
{
	if (PeekString("<?"))
	{
		String temp;
		FindString(">", temp);
	}
}

void BaseXMLParser::ReadBody()
{
	RMLUI_ZoneScoped;

	open_tag_depth = 0;
	line_number_open_tag = 0;

	for (;;)
	{
		// Find the next open tag.
		if (!FindString("<", data, true))
			break;

		const size_t xml_index_tag = xml_index - 1;

		// Check what kind of tag this is.
		if (PeekString("!--"))
		{
			// Comment.
			String temp;
			if (!FindString("-->", temp))
				break;
		}
		else if (PeekString("![CDATA["))
		{
			// CDATA tag; read everything (including markup) until the ending
			// CDATA tag.
			if (!ReadCDATA())
				break;
		}
		else if (PeekString("/"))
		{
			if (!ReadCloseTag(xml_index_tag))
				break;

			// Bail if we've hit the end of the XML data.
			if (open_tag_depth == 0)
				break;
		}
		else
		{
			if (!ReadOpenTag())
				break;
		}
	}

	// Check for error conditions
	if (open_tag_depth > 0)
	{
		Log::Message(Log::LT_WARNING, "XML parse error on line %d of %s.", GetLineNumber(), source_url->GetURL().c_str());
	}
}

bool BaseXMLParser::ReadOpenTag()
{
	// Increase the open depth
	open_tag_depth++;

	// Opening tag; send data immediately and open the tag.
	if (!data.empty())
	{
		HandleDataInternal(data, XMLDataType::Text);
		data.clear();
	}

	String tag_name;
	if (!FindWord(tag_name, "/>"))
		return false;

	bool section_opened = false;

	if (PeekString(">"))
	{
		// Simple open tag.
		HandleElementStartInternal(tag_name, XMLAttributes());
		section_opened = true;
	}
	else if (PeekString("/") && PeekString(">"))
	{
		// Empty open tag.
		HandleElementStartInternal(tag_name, XMLAttributes());
		HandleElementEndInternal(tag_name);

		// Tag immediately closed, reduce count
		open_tag_depth--;
	}
	else
	{
		// It appears we have some attributes. Let's parse them.
		bool parse_inner_xml_as_data = false;
		XMLAttributes attributes;
		if (!ReadAttributes(attributes, parse_inner_xml_as_data))
			return false;

		if (PeekString(">"))
		{
			HandleElementStartInternal(tag_name, attributes);
			section_opened = true;
		}
		else if (PeekString("/") && PeekString(">"))
		{
			HandleElementStartInternal(tag_name, attributes);
			HandleElementEndInternal(tag_name);

			// Tag immediately closed, reduce count
			open_tag_depth--;
		}
		else
		{
			return false;
		}

		if (section_opened && parse_inner_xml_as_data && !inner_xml_data)
		{
			inner_xml_data = true;
			inner_xml_data_terminate_depth = open_tag_depth;
			inner_xml_data_index_begin = xml_index;
		}
	}

	// Check if this tag needs to be processed as CDATA.
	if (section_opened)
	{
		const String lcase_tag_name = StringUtilities::ToLower(tag_name);
		bool is_cdata_tag = (cdata_tags.find(lcase_tag_name) != cdata_tags.end());

		if (is_cdata_tag)
		{
			if (ReadCDATA(lcase_tag_name.c_str()))
			{
				open_tag_depth--;
				if (!data.empty())
				{
					HandleDataInternal(data, XMLDataType::CData);
					data.clear();
				}
				HandleElementEndInternal(tag_name);

				return true;
			}

			return false;
		}
	}

	return true;
}

bool BaseXMLParser::ReadCloseTag(const size_t xml_index_tag)
{
	if (inner_xml_data && open_tag_depth == inner_xml_data_terminate_depth)
	{
		// Closing the tag that initiated the inner xml data parsing. Set all its contents as Data to be
		// submitted next, and disable the mode to resume normal parsing behavior.
		RMLUI_ASSERT(inner_xml_data_index_begin <= xml_index_tag);
		inner_xml_data = false;
		data = xml_source.substr(inner_xml_data_index_begin, xml_index_tag - inner_xml_data_index_begin);
		HandleDataInternal(data, XMLDataType::InnerXML);
		data.clear();
	}

	// Closing tag; send data immediately and close the tag.
	if (!data.empty())
	{
		HandleDataInternal(data, XMLDataType::Text);
		data.clear();
	}

	String tag_name;
	if (!FindString(">", tag_name))
		return false;

	HandleElementEndInternal(StringUtilities::StripWhitespace(tag_name));

	// Tag closed, reduce count
	open_tag_depth--;

	return true;
}

bool BaseXMLParser::ReadAttributes(XMLAttributes& attributes, bool& parse_raw_xml_content)
{
	for (;;)
	{
		String attribute;
		String value;

		// Get the attribute name
		if (!FindWord(attribute, "=/>"))
		{
			return false;
		}

		// Check if theres an assigned value
		if (PeekString("="))
		{
			if (PeekString("\""))
			{
				if (!FindString("\"", value))
					return false;
			}
			else if (PeekString("'"))
			{
				if (!FindString("'", value))
					return false;
			}
			else if (!FindWord(value, "/>"))
			{
				return false;
			}
		}

		if (attributes_for_inner_xml_data.count(attribute) == 1)
			parse_raw_xml_content = true;

		attributes[attribute] = StringUtilities::DecodeRml(value);

		// Check for the end of the tag.
		if (PeekString("/", false) || PeekString(">", false))
			return true;
	}
}

bool BaseXMLParser::ReadCDATA(const char* tag_terminator)
{
	String cdata;
	if (tag_terminator == nullptr)
	{
		FindString("]]>", cdata);
		data += cdata;
		return true;
	}
	else
	{
		for (;;)
		{
			// Search for the next tag opening.
			if (!FindString("<", cdata))
				return false;

			if (PeekString("/", false))
			{
				String tag;
				if (FindString(">", tag))
				{
					size_t slash_pos = tag.find('/');
					String tag_name = StringUtilities::StripWhitespace(slash_pos == String::npos ? tag : tag.substr(slash_pos + 1));
					if (StringUtilities::ToLower(std::move(tag_name)) == tag_terminator)
					{
						data += cdata;
						return true;
					}
					else
					{
						cdata += '<' + tag + '>';
					}
				}
				else
					cdata += "<";
			}
			else
				cdata += "<";
		}
	}
}

bool BaseXMLParser::FindWord(String& word, const char* terminators)
{
	while (!AtEnd())
	{
		char c = Look();

		// Count line numbers
		if (c == '\n')
		{
			line_number++;
		}

		// Ignore white space
		if (StringUtilities::IsWhitespace(c))
		{
			if (word.empty())
			{
				Next();
				continue;
			}
			else
				return true;
		}

		// Check for termination condition
		if (terminators && strchr(terminators, c))
		{
			return !word.empty();
		}

		word += c;
		Next();
	}

	return false;
}

bool BaseXMLParser::FindString(const char* string, String& data, bool escape_brackets)
{
	int index = 0;
	bool in_brackets = false;
	bool in_string = false;
	char previous = 0;

	while (string[index])
	{
		if (AtEnd())
			return false;

		const char c = Look();

		// Count line numbers
		if (c == '\n')
		{
			line_number++;
		}

		if (escape_brackets)
		{
			const char* error_str = XMLParseTools::ParseDataBrackets(in_brackets, in_string, c, previous);
			if (error_str)
			{
				Log::Message(Log::LT_WARNING, "XML parse error. %s", error_str);
				return false;
			}
		}

		if (c == string[index] && !in_brackets)
		{
			index += 1;
		}
		else
		{
			if (index > 0)
			{
				data += String(string, index);
				index = 0;
			}

			data += c;
		}

		previous = c;
		Next();
	}

	return true;
}

bool BaseXMLParser::PeekString(const char* string, bool consume)
{
	const size_t start_index = xml_index;
	const int start_line = line_number;
	bool success = true;
	int i = 0;
	while (string[i])
	{
		if (AtEnd())
		{
			success = false;
			break;
		}

		const char c = Look();

		// Count line numbers
		if (c == '\n')
		{
			line_number++;
		}

		// Seek past all the whitespace if we haven't hit the initial character yet.
		if (i == 0 && StringUtilities::IsWhitespace(c))
		{
			Next();
		}
		else
		{
			if (c != string[i])
			{
				success = false;
				break;
			}

			i++;
			Next();
		}
	}

	// Set the index to the start index unless we are consuming.
	if (!consume || !success)
	{
		xml_index = start_index;
		line_number = start_line;
	}

	return success;
}

} // namespace Rml
