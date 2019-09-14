#include "..\..\Include\RmlUi\Core\StringUtilities.h"
#include "..\..\Include\RmlUi\Core\StringUtilities.h"
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

#include "precompiled.h"
#include "../../Include/RmlUi/Core/StringUtilities.h"
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>

namespace Rml {
namespace Core {

static bool UTF8toUCS2(const String& input, WString& output);
static bool UCS2toUTF8(const WString& input, String& output);


static int FormatString(String& string, size_t max_size, const char* format, va_list argument_list)
{
	const int INTERNAL_BUFFER_SIZE = 1024;
	static char buffer[INTERNAL_BUFFER_SIZE];
	char* buffer_ptr = buffer;

	if (max_size + 1 > INTERNAL_BUFFER_SIZE)
		buffer_ptr = new char[max_size + 1];

	int length = vsnprintf(buffer_ptr, max_size, format, argument_list);
	buffer_ptr[length >= 0 ? length : max_size] = '\0';
#ifdef RMLUI_DEBUG
	if (length == -1)
	{
		Log::Message(Log::LT_WARNING, "FormatString: String truncated to %d bytes when processing %s", max_size, format);
	}
#endif

	string = buffer_ptr;

	if (buffer_ptr != buffer)
		delete[] buffer_ptr;

	return length;
}

int FormatString(String& string, size_t max_size, const char* format, ...)
{
	va_list argument_list;
	va_start(argument_list, format);
	int result = FormatString(string, (int)max_size, format, argument_list);
	va_end(argument_list);
	return result;
}
String CreateString(size_t max_size, const char* format, ...)
{
	String result;
	result.reserve(max_size);
	va_list argument_list;
	va_start(argument_list, format);
	FormatString(result, max_size, format, argument_list);
	va_end(argument_list);
	return result;
}


String StringUtilities::ToLower(const String& string) {
	String str_lower = string;
	std::transform(str_lower.begin(), str_lower.end(), str_lower.begin(), ::tolower);
	return str_lower;
}

WString StringUtilities::ToUCS2(const String& str)
{
	WString result;
	if (!UTF8toUCS2(str, result))
		Log::Message(Log::LT_WARNING, "Failed to convert UTF8 string to UCS2.");
	return result;
}

WString StringUtilities::ToUTF16(const String& str)
{
	// TODO: Convert to UTF16 instead of UCS2
	return ToUCS2(str);
}

String StringUtilities::ToUTF8(const WString& wstr)
{
	String result;
	if(!UCS2toUTF8(wstr, result))
		Log::Message(Log::LT_WARNING, "Failed to convert UCS2 string to UTF8.");
	return result;
}

int StringUtilities::LengthUTF8(const String& str)
{
	// TODO: Actually consider multibyte characters
	return (int)str.size();
}

String StringUtilities::Replace(String subject, const String& search, const String& replace)
{
	size_t pos = 0;
	while ((pos = subject.find(search, pos)) != String::npos) {
		subject.replace(pos, search.length(), replace);
		pos += replace.length();
	}
	return subject;
}

String StringUtilities::Replace(String subject, char search, char replace)
{
	const size_t size = subject.size();
	for (size_t i = 0; i < size; i++)
	{
		if (subject[i] == search)
			subject[i] = replace;
	}
	return subject;
}


// Expands character-delimited list of values in a single string to a whitespace-trimmed list of values.
void StringUtilities::ExpandString(StringList& string_list, const String& string, const char delimiter)
{	
	char quote = 0;
	bool last_char_delimiter = true;
	const char* ptr = string.c_str();
	const char* start_ptr = nullptr;
	const char* end_ptr = ptr;

	size_t num_delimiter_values = std::count(string.begin(), string.end(), delimiter);
	if (num_delimiter_values == 0)
	{
		string_list.push_back(StripWhitespace(string));
		return;
	}
	string_list.reserve(string_list.size() + num_delimiter_values + 1);

	while (*ptr)
	{
		// Switch into quote mode if the last char was a delimeter ( excluding whitespace )
		// and we're not already in quote mode
		if (last_char_delimiter && !quote && (*ptr == '"' || *ptr == '\''))
		{			
			quote = *ptr;
		}
		// Switch out of quote mode if we encounter a quote that hasn't been escaped
		else if (*ptr == quote && *(ptr-1) != '\\')
		{
			quote = 0;
		}
		// If we encounter a delimiter while not in quote mode, add the item to the list
		else if (*ptr == delimiter && !quote)
		{
			if (start_ptr)
				string_list.emplace_back(start_ptr, end_ptr + 1);
			else
				string_list.emplace_back();
			last_char_delimiter = true;
			start_ptr = nullptr;
		}
		// Otherwise if its not white space or we're in quote mode, advance the pointers
		else if (!isspace(*ptr) || quote)
		{
			if (!start_ptr)
				start_ptr = ptr;
			end_ptr = ptr;
			last_char_delimiter = false;
		}

		ptr++;
	}

	// If there's data pending, add it.
	if (start_ptr)
		string_list.emplace_back(start_ptr, end_ptr + 1);
}


void StringUtilities::ExpandString(StringList& string_list, const String& string, const char delimiter, char quote_character, char unquote_character, bool ignore_repeated_delimiters)
{
	int quote_mode_depth = 0;
	const char* ptr = string.c_str();
	const char* start_ptr = nullptr;
	const char* end_ptr = ptr;

	while (*ptr)
	{
		// Increment the quote depth for each quote character encountered
		if (*ptr == quote_character)
		{
			++quote_mode_depth;
		}
		// And decrement it for every unquote character
		else if (*ptr == unquote_character)
		{
			--quote_mode_depth;
		}

		// If we encounter a delimiter while not in quote mode, add the item to the list
		if (*ptr == delimiter && quote_mode_depth == 0)
		{
			if (start_ptr)
				string_list.emplace_back(start_ptr, end_ptr + 1);
			else if(!ignore_repeated_delimiters)
				string_list.emplace_back();
			start_ptr = nullptr;
		}
		// Otherwise if its not white space or we're in quote mode, advance the pointers
		else if (!isspace(*ptr) || quote_mode_depth > 0)
		{
			if (!start_ptr)
				start_ptr = ptr;
			end_ptr = ptr;
		}

		ptr++;
	}

	// If there's data pending, add it.
	if (start_ptr)
		string_list.emplace_back(start_ptr, end_ptr + 1);
}


// Joins a list of string values into a single string separated by a character delimiter.
void StringUtilities::JoinString(String& string, const StringList& string_list, const char delimiter)
{
	for (size_t i = 0; i < string_list.size(); i++)
	{
		string += string_list[i];
		if (delimiter != '\0' && i < string_list.size() - 1)
			string += delimiter;
	}
}



// Strip whitespace characters from the beginning and end of a string.
String StringUtilities::StripWhitespace(const String& string)
{
	const char* start = string.c_str();
	const char* end = start + string.size();

	while (start < end && IsWhitespace(*start))
		start++;

	while (end > start && IsWhitespace(*(end - 1)))
		end--;

	if (start < end)
		return String(start, end);

	return String();
}

// Operators for STL containers using strings.
bool StringUtilities::StringComparei::operator()(const String& lhs, const String& rhs) const
{
	return strcasecmp(lhs.c_str(), rhs.c_str()) < 0;
}


// Defines, helper functions for the UTF8 / UCS2 conversion functions.
#define _NXT	0x80
#define _SEQ2	0xc0
#define _SEQ3	0xe0
#define _SEQ4	0xf0
#define _SEQ5	0xf8
#define _SEQ6	0xfc
	
#define _BOM	0xfeff
	
static int __wchar_forbidden(unsigned int sym)
{
	// Surrogate pairs
	if (sym >= 0xd800 && sym <= 0xdfff)
		return -1;
	
	return 0;
}

static int __utf8_forbidden(unsigned char octet)
{
	switch (octet)
	{
		case 0xc0:
		case 0xc1:
		case 0xf5:
		case 0xff:
			return -1;
			
		default:
			return 0;
	}
}



// Converts a character array in UTF-8 encoding to a vector of words.
static bool UTF8toUCS2(const String& input, WString& output)
{
	if (input.empty())
		return true;

	output.reserve(input.size());
	
	unsigned char* p = (unsigned char*) input.c_str();
	unsigned char* lim = p + input.size();
	
	// Skip the UTF-8 byte order marker if it exists.
	if (input.substr(0, 3) == "\xEF\xBB\xBF")
		p += 3;
	
	int num_bytes;
	for (; p < lim; p += num_bytes)
	{
		if (__utf8_forbidden(*p) != 0)
			return false;
		
		// Get number of bytes for one wide character.
		word high;
		num_bytes = 1;
		
		if ((*p & 0x80) == 0)
		{
			high = (wchar_t)*p;
		}
		else if ((*p & 0xe0) == _SEQ2)
		{
			num_bytes = 2;
			high = (wchar_t)(*p & 0x1f);
		}
		else if ((*p & 0xf0) == _SEQ3)
		{
			num_bytes = 3;
			high = (wchar_t)(*p & 0x0f);
		}
		else if ((*p & 0xf8) == _SEQ4)
		{
			num_bytes = 4;
			high = (wchar_t)(*p & 0x07);
		}
		else if ((*p & 0xfc) == _SEQ5)
		{
			num_bytes = 5;
			high = (wchar_t)(*p & 0x03);
		}
		else if ((*p & 0xfe) == _SEQ6)
		{
			num_bytes = 6;
			high = (wchar_t)(*p & 0x01);
		}
		else
		{
			return false;
		}
		
		// Does the sequence header tell us the truth about length?
		if (lim - p <= num_bytes - 1)
		{
			return false;
		}
		
		// Validate the sequence. All symbols must have higher bits set to 10xxxxxx.
		if (num_bytes > 1)
		{
			int i;
			for (i = 1; i < num_bytes; i++)
			{
				if ((p[i] & 0xc0) != _NXT)
					break;
			}
			
			if (i != num_bytes)
			{
				return false;
			}
		}
		
		// Make up a single UCS-4 (32-bit) character from the required number of UTF-8 tokens. The first byte has
		// been determined earlier, the second and subsequent bytes contribute the first six of their bits into the
		// final character code.
		unsigned int ucs4_char = 0;
		int num_bits = 0;
		for (int i = 1; i < num_bytes; i++)
		{
			ucs4_char |= (word)(p[num_bytes - i] & 0x3f) << num_bits;
			num_bits += 6;
		}
		ucs4_char |= high << num_bits;
		
		// Check for surrogate pairs.
		if (__wchar_forbidden(ucs4_char) != 0)
		{
			return false;
		}
		
		// Only add the character to the output if it exists in the Basic Multilingual Plane (ie, fits in a single
		// word).
		if (ucs4_char <= 0xffff)
			output.push_back((word) ucs4_char);
	}
	
	return true;
}

// Converts an array of words in UCS-2 encoding into a character array in UTF-8 encoding.
static bool UCS2toUTF8(const WString& input, String& output)
{
	unsigned char *oc;
	size_t n;

	output.reserve(input.size());
	
	const word* w = input.data();
	const word* wlim = w + input.size();
	
	//Log::Message(LC_CORE, Log::LT_ALWAYS, "UCS2TOUTF8 size: %d", input_size);
	for (; w < wlim; w++)
	{
		if (__wchar_forbidden(*w) != 0)
			return false;
		
		if (*w == _BOM)
			continue;
		
		//if (*w < 0)
		//	return false;
		if (*w <= 0x007f)
			n = 1;
		else if (*w <= 0x07ff)
			n = 2;
		else //if (*w <= 0x0000ffff)
			n = 3;
		/*else if (*w <= 0x001fffff)
		 n = 4;
		 else if (*w <= 0x03ffffff)
		 n = 5;
		 else // if (*w <= 0x7fffffff)
		 n = 6;*/
		
		// Convert to little endian.
		word ch = (*w >> 8) & 0x00FF;
		ch |= (*w << 8) & 0xFF00;
		//		word ch = EMPConvertEndian(*w, RMLUI_ENDIAN_BIG);
		
		oc = (unsigned char *)&ch;
		switch (n)
		{
			case 1:
				output += oc[1];
				break;
				
			case 2:
				output += (_SEQ2 | (oc[1] >> 6) | ((oc[0] & 0x07) << 2));
				output += (_NXT | (oc[1] & 0x3f));
				break;
				
			case 3:
				output += (_SEQ3 | ((oc[0] & 0xf0) >> 4));
				output += (_NXT | (oc[1] >> 6) | ((oc[0] & 0x0f) << 2));
				output += (_NXT | (oc[1] & 0x3f));
				break;
				
			case 4:
				break;
				
			case 5:
				break;
				
			case 6:
				break;
		}
		
		//Log::Message(LC_CORE, Log::LT_ALWAYS, "Converting...%c(%d) %d -> %d", *w, *w, w - input, output.size());
	}
	
	return true;
}

}
}
