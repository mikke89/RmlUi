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

#include "../../Include/RmlUi/Core/BaseXMLParser.h"
#include "../../Include/RmlUi/Core/Profiling.h"
#include "../../Include/RmlUi/Core/Stream.h"
#include <string.h>

namespace Rml {
namespace Core {

// Most file layers cache 4k.
const int DEFAULT_BUFFER_SIZE = 4096;

BaseXMLParser::BaseXMLParser()
{
	read = nullptr;
	buffer = nullptr;
	buffer_used = 0;
	buffer_size = 0;
	open_tag_depth = 0;
	treat_content_as_cdata = false;
}

BaseXMLParser::~BaseXMLParser()
{
}

// Registers a tag as containing general character data.
void BaseXMLParser::RegisterCDATATag(const String& tag)
{
	if (!tag.empty())
		cdata_tags.insert(StringUtilities::ToLower(tag));
}

// Parses the given stream as an XML file, and calls the handlers when
// interesting phenomenon are encountered.
void BaseXMLParser::Parse(Stream* stream)
{
	xml_source = stream;
	buffer_size = DEFAULT_BUFFER_SIZE;
	treat_content_as_cdata = false;

	buffer = (unsigned char*) malloc(buffer_size);
	read = buffer;
	line_number = 1;
	FillBuffer();

	// Read (er ... skip) the header, if one exists.
	ReadHeader();
	// Read the XML body.
	ReadBody();

	free(buffer);
}

// Get the current file line number
int BaseXMLParser::GetLineNumber() const
{
	return line_number;
}

int BaseXMLParser::GetLineNumberOpenTag() const
{
	return line_number_open_tag;
}

// Called when the parser finds the beginning of an element tag.
void BaseXMLParser::HandleElementStart(const String& RMLUI_UNUSED_PARAMETER(name), const XMLAttributes& RMLUI_UNUSED_PARAMETER(attributes))
{
	RMLUI_UNUSED(name);
	RMLUI_UNUSED(attributes);
}

// Called when the parser finds the end of an element tag.
void BaseXMLParser::HandleElementEnd(const String& RMLUI_UNUSED_PARAMETER(name))
{
	RMLUI_UNUSED(name);
}

// Called when the parser encounters data.
void BaseXMLParser::HandleData(const String& RMLUI_UNUSED_PARAMETER(data))
{
	RMLUI_UNUSED(data);
}

void BaseXMLParser::TreatElementContentAsCDATA()
{
	treat_content_as_cdata = true;
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

	for(;;)
	{
		// Find the next open tag.
		if (!FindString("<", data, true))
			break;

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
			if (!ReadCloseTag())
				break;

			// Bail if we've hit the end of the XML data.
			if (open_tag_depth == 0)
			{
				xml_source->Seek((long)((read - buffer) - buffer_used), SEEK_CUR);
				break;
			}
		}
		else
		{
			if (ReadOpenTag())
				line_number_open_tag = line_number;
			else
				break;
		}
	}

	// Check for error conditions
	if (open_tag_depth > 0)
	{
		Log::Message(Log::LT_WARNING, "XML parse error on line %d of %s.", GetLineNumber(), xml_source->GetSourceURL().GetURL().c_str());
	}
}

bool BaseXMLParser::ReadOpenTag()
{
	// Increase the open depth
	open_tag_depth++;

	treat_content_as_cdata = false;

	// Opening tag; send data immediately and open the tag.
	if (!data.empty())
	{
		HandleData(data);
		data.clear();
	}

	String tag_name;
	if (!FindWord(tag_name, "/>"))
		return false;

	bool section_opened = false;

	if (PeekString(">"))
	{
		// Simple open tag.
		HandleElementStart(tag_name, XMLAttributes());
		section_opened = true;
	}
	else if (PeekString("/") &&
			 PeekString(">"))
	{
		// Empty open tag.
		HandleElementStart(tag_name, XMLAttributes());
		HandleElementEnd(tag_name);

		// Tag immediately closed, reduce count
		open_tag_depth--;
	}
	else
	{
		// It appears we have some attributes. Let's parse them.
		XMLAttributes attributes;
		if (!ReadAttributes(attributes))
			return false;

		if (PeekString(">"))
		{
			HandleElementStart(tag_name, attributes);
			section_opened = true;
		}
		else if (PeekString("/") &&
				 PeekString(">"))
		{
			HandleElementStart(tag_name, attributes);
			HandleElementEnd(tag_name);

			// Tag immediately closed, reduce count
			open_tag_depth--;
		}
		else
		{
			return false;
		}
	}

	// Check if this tag needs to be processed as CDATA.
	if (section_opened)
	{
		String lcase_tag_name = StringUtilities::ToLower(tag_name);
		bool is_cdata_tag = (cdata_tags.find(lcase_tag_name) != cdata_tags.end());

		if (treat_content_as_cdata || is_cdata_tag)
		{
			if (ReadCDATA(lcase_tag_name.c_str(), !is_cdata_tag))
			{
				open_tag_depth--;
				if (!data.empty())
				{
					HandleData(data);
					data.clear();
				}
				HandleElementEnd(tag_name);

				return true;
			}

			return false;
		}
	}

	return true;
}

bool BaseXMLParser::ReadCloseTag()
{
	// Closing tag; send data immediately and close the tag.
	if (!data.empty())
	{
		HandleData(data);
		data.clear();
	}

	String tag_name;
	if (!FindString(">", tag_name))
		return false;

	HandleElementEnd(StringUtilities::StripWhitespace(tag_name));

	// Tag closed, reduce count
	open_tag_depth--;

	return true;
}

bool BaseXMLParser::ReadAttributes(XMLAttributes& attributes)
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

 		attributes[attribute] = value;

		// Check for the end of the tag.
		if (PeekString("/", false) ||
			PeekString(">", false))
			return true;
	}
}

bool BaseXMLParser::ReadCDATA(const char* tag_terminator, bool only_terminate_at_same_xml_depth)
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
		int tag_depth = 1;

		// TODO: This doesn't properly handle comments and double brackets,
		// should probably find a way to use the normal parsing flow instead.

		for (;;)
		{
			// Search for the next tag opening.
			if (!FindString("<", cdata))
				return false;

			String node_raw;
			if (!FindString(">", node_raw))
				return false;

			String node_stripped = StringUtilities::StripWhitespace(node_raw);
			bool close_begin = false;
			bool close_end = false;

			if (!node_stripped.empty())
			{
				if (node_stripped.front() == '/')
					close_begin = true;
				else if (node_stripped.back() == '/')
					close_end = true;
			}

			if (!close_begin && !close_end)
				tag_depth += 1;
			else if (close_begin && !close_end)
				tag_depth -= 1;

			if (close_begin && !close_end && (!only_terminate_at_same_xml_depth || tag_depth == 0))
			{
				String tag_name = StringUtilities::StripWhitespace(node_stripped.substr(1));

				if (StringUtilities::ToLower(tag_name) == tag_terminator)
				{
					data += cdata;
					return true;
				}
			}

			if (only_terminate_at_same_xml_depth && tag_depth <= 0)
			{
				return false;
			}

			cdata += '<' + node_raw + '>';
		}
	}
}

// Reads from the stream until a complete word is found.
bool BaseXMLParser::FindWord(String& word, const char* terminators)
{
	for (;;)
	{
		if (read >= buffer + buffer_used)
		{
			if (!FillBuffer())			
				return false;
		}

		// Ignore white space
		if (StringUtilities::IsWhitespace(*read))
		{
			if (word.empty())
			{
				read++;
				continue;
			}
			else
				return true;
		}

		// Check for termination condition
		if (terminators && strchr(terminators, *read))
		{
			return !word.empty();
		}

		word += *read;
		read++;
	}
}

// Reads from the stream until the given character set is found.
bool BaseXMLParser::FindString(const char* string, String& data, bool escape_brackets)
{
	int index = 0;
	bool in_brackets = false;
	char previous = 0;

	while (string[index])
	{
		if (read >= buffer + buffer_used)
		{
			if (!FillBuffer())
				return false;
		}

		const char c = char(*read);

		// Count line numbers
		if (c == '\n')
		{
			line_number++;
		}

		if(escape_brackets)
		{
			if (c == '{' && previous == '{')
				in_brackets = true;
			else if (c == '}' && previous == '}')
				in_brackets = false;
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
		read++;
	}

	return true;
}

// Returns true if the next sequence of characters in the stream matches the
// given string.
bool BaseXMLParser::PeekString(const char* string, bool consume)
{
	unsigned char* peek_read = read;

	int i = 0;
	while (string[i])
	{
		// If we're about to read past the end of the buffer, read into the
		// overflow buffer.
		if ((peek_read - buffer) + i >= buffer_used)
		{
			int peek_offset = (int)(peek_read - read);
			FillBuffer();
			peek_read = read + peek_offset;

			if (peek_read - buffer + i >= buffer_used)
			{
				// Weird, seems our buffer is too small, realloc it bigger.
				buffer_size *= 2;
				int read_offset = (int)(read - buffer);
				unsigned char* new_buffer = (unsigned char*) realloc(buffer, buffer_size);
				RMLUI_ASSERTMSG(new_buffer != nullptr, "Unable to allocate larger buffer for Peek() call");
				if(new_buffer == nullptr)
				{
					return false;
				}
				buffer = new_buffer;
				// Restore the read pointers.
				read = buffer + read_offset;
				peek_read = read + peek_offset;
				
				// Attempt to fill our new buffer size.
				if (!FillBuffer())
					return false;
			}
		}

		// Seek past all the whitespace if we haven't hit the initial character yet.
		if (i == 0 && StringUtilities::IsWhitespace(*peek_read))
		{
			peek_read++;
		}
		else
		{
			if (char(*peek_read) != string[i])
				return false;

			i++;
			peek_read++;
		}
	}

	// Set the read pointer to the end of the peek.
	if (consume)
	{		
		read = peek_read;
	}

	return true;
}

// Fill the buffer as much as possible, without removing any content that is still pending
bool BaseXMLParser::FillBuffer()
{
	int bytes_free = buffer_size;
	int bytes_remaining = Math::Max((int)(buffer_used - (read - buffer)), 0);

	// If theres any data still in the buffer, shift it down, and fill it again
	if (bytes_remaining > 0)
	{
		memmove(buffer, read, bytes_remaining);
		bytes_free = buffer_size - bytes_remaining;
	}
	
	read = buffer;
	size_t bytes_read = xml_source->Read(&buffer[bytes_remaining], bytes_free);
	buffer_used = (int)(bytes_read + bytes_remaining);

	return bytes_read > 0;
}

}
}
