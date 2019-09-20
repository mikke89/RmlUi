#include "..\..\Include\RmlUi\Core\StringUtilities.h"
#include "..\..\Include\RmlUi\Core\StringUtilities.h"
#include "..\..\Include\RmlUi\Core\StringUtilities.h"
#include "..\..\Include\RmlUi\Core\StringUtilities.h"
#include "..\..\Include\RmlUi\Core\StringUtilities.h"
#include "..\..\Include\RmlUi\Core\StringUtilities.h"
#include "..\..\Include\RmlUi\Core\StringUtilities.h"
#include "..\..\Include\RmlUi\Core\StringUtilities.h"
#include "..\..\Include\RmlUi\Core\StringUtilities.h"
#include "..\..\Include\RmlUi\Core\StringUtilities.h"
#include "..\..\Include\RmlUi\Core\StringUtilities.h"
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

static bool UTF8toUTF16(const String& input, WString& output);
static bool UTF16toUTF8(const WString& input, String& output);


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

WString StringUtilities::ToUTF16(const String& str)
{
	WString result;
	if (!UTF8toUTF16(str, result))
		Log::Message(Log::LT_WARNING, "Invalid characters encountered while converting UTF-8 string to UTF-16.");
	return result;
}

String StringUtilities::ToUTF8(const WString& wstr)
{
	String result;
	if(!UTF16toUTF8(wstr, result))
		Log::Message(Log::LT_WARNING, "Invalid characters encountered while converting UTF-16 string to UTF-8.");
	return result;
}

size_t StringUtilities::LengthUTF8(StringView string_view)
{
	const char* const p_end = string_view.end();

	// Skip any continuation bytes at the beginning
	const char* p = string_view.begin();

	size_t num_continuation_bytes = 0;

	while (p != p_end)
	{
		if ((*p & 0b1100'0000) == 0b1000'0000)
			++num_continuation_bytes;
		++p;
	}

	return string_view.size() - num_continuation_bytes;
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

CodePoint StringUtilities::ToCodePoint(const char* p)
{
	if ((*p & (1 << 7)) == 0)
		return static_cast<CodePoint>(*p);

	int num_bytes = 0;
	int code = 0;

	if ((*p & 0b1110'0000) == 0b1100'0000)
	{
		num_bytes = 2;
		code = (*p & 0b0001'1111);
	}
	else if ((*p & 0b1111'0000) == 0b1110'0000)
	{
		num_bytes = 3;
		code = (*p & 0b0000'1111);
	}
	else if ((*p & 0b1111'1000) == 0b1111'0000)
	{
		num_bytes = 4;
		code = (*p & 0b0000'0111);
	}
	else
	{
		// Invalid begin byte
		return CodePoint::Null;
	}

	for (int i = 1; i < num_bytes; i++)
	{
		const char byte = *(p + i);
		if ((byte & 0b1100'0000) != 0b1000'0000)
		{
			// Invalid continuation byte
			++p;
			return CodePoint::Null;
		}

		code = ((code << 6) | (byte & 0b0011'1111));
	}

	return static_cast<CodePoint>(code);
}

String StringUtilities::ToUTF8(CodePoint code_point)
{
	return ToUTF8(&code_point, 1);
}

String StringUtilities::ToUTF8(const CodePoint* code_points, int num_code_points)
{
	String result;

	bool invalid_code_point = false;

	for (int i = 0; i < num_code_points; i++)
	{
		unsigned int c = (unsigned int)code_points[i];

		constexpr int l3 = 0b0000'0111;
		constexpr int l4 = 0b0000'1111;
		constexpr int l5 = 0b0001'1111;
		constexpr int l6 = 0b0011'1111;
		constexpr int h1 = 0b1000'0000;
		constexpr int h2 = 0b1100'0000;
		constexpr int h3 = 0b1110'0000;
		constexpr int h4 = 0b1111'0000;

		if (c < 0x80)
			result += (char)c;
		else if (c < 0x800)
			result += { char(((c >> 6)& l5) | h2), char((c& l6) | h1) };
		else if (c < 0x10000)
			result += { char(((c >> 12)& l4) | h3), char(((c >> 6)& l6) | h1), char((c& l6) | h1) };
		else if (c <= 0x10FFFF)
			result += { char(((c >> 18)& l3) | h4), char(((c >> 12)& l6) | h1), char(((c >> 6)& l6) | h1), char((c& l6) | h1) };
		else
			invalid_code_point = true;
	}

	if (invalid_code_point)
		Log::Message(Log::LT_WARNING, "One or more invalid code points encountered while encoding to UTF-8.");

	return result;
}

// Operators for STL containers using strings.
bool StringUtilities::StringComparei::operator()(const String& lhs, const String& rhs) const
{
	return strcasecmp(lhs.c_str(), rhs.c_str()) < 0;
}



// Converts a character array in UTF-8 encoding to a wide string in UTF-16 encoding.
static bool UTF8toUTF16(const String& input, WString& output)
{
	if (input.empty())
		return true;

	std::vector<CodePoint> code_points;
	code_points.reserve(input.size());

	for (auto it = StringIteratorU8(input); it; ++it)
		code_points.push_back(*it);

	output.reserve(input.size());

	bool valid_characters = true;

	for (CodePoint code_point : code_points)
	{
		unsigned int c = (unsigned int)code_point;

		if (c <= 0xD7FF || (c >= 0xE000 && c <= 0xFFFF))
		{
			// Single 16-bit code unit.
			output += (wchar_t)c;
		}
		else if (c >= 0x10000 && c <= 0x10FFFF)
		{
			// Encode as two 16-bit code units.
			unsigned int c_shift = c - 0x10000;
			wchar_t w1 = (0xD800 | ((c_shift >> 10) & 0x3FF));
			wchar_t w2 = (0xDC00 | (c_shift & 0x3FF));
			output += {w1, w2};
		}
		else
		{
			valid_characters = false;
		}
	}

	return valid_characters;
}

// Converts a wide string in UTF-16 encoding into a string in UTF-8 encoding.
static bool UTF16toUTF8(const WString& input, String& output)
{
	std::vector<CodePoint> code_points;
	code_points.reserve(input.size());

	bool valid_input = true;
	wchar_t w1 = 0;

	for (wchar_t w : input)
	{
		if (w <= 0xD7FF || w >= 0xE000)
		{
			// Single 16-bit code unit.
			code_points.push_back((CodePoint)(w));
		}
		else 
		{
			// Two 16-bit code units.
			if (!w1 && w < 0xDC00)
			{
				w1 = w;
			}
			else if (w1 && w >= 0xDC00)
			{
				code_points.push_back((CodePoint)(((((unsigned int)w1 & 0x3FF) << 10) | ((unsigned int)(w) & 0x3FF)) + 0x10000u));
				w1 = 0;
			}
			else
			{
				valid_input = false;
			}
		}
	}

	if(code_points.size() > 0)
		output = StringUtilities::ToUTF8(code_points.data(), (int)code_points.size());

	return valid_input;
}

StringView::StringView(const char* p_begin, const char* p_end) : p_begin(p_begin), p_end(p_end)
{
	RMLUI_ASSERT(p_end >= p_begin);
}
StringView::StringView(const String& string) : p_begin(string.data()), p_end(string.data() + string.size())
{}
StringView::StringView(const String& string, size_t offset) : p_begin(string.data()), p_end(string.data() + string.size())
{}
StringView::StringView(const String& string, size_t offset, size_t count) : p_begin(string.data()), p_end(string.data() + std::min(offset + count, string.size()))
{}

bool StringView::operator==(const StringView& other) const { 
	return (p_end - p_begin) == (other.p_end - other.p_begin) && strcmp(p_begin, other.p_begin) == 0; 
}


// TODO: Remove seek on construction
StringIteratorU8::StringIteratorU8(const char* p_begin, const char* p, const char* p_end) : view(p_begin, p_end), p(p) 
{
	SeekForward();
}
StringIteratorU8::StringIteratorU8(const String& string) : view(string), p(string.data())
{ 
	SeekForward();
}
StringIteratorU8::StringIteratorU8(const String& string, size_t offset) : view(string), p(string.data() + offset)
{ 
	SeekForward();
}
StringIteratorU8::StringIteratorU8(const String& string, size_t offset, size_t count) : view(string, 0, offset + count), p(string.data() + offset)
{
	SeekForward();
}
StringIteratorU8& StringIteratorU8::operator++() {
	RMLUI_ASSERT(p != view.end());
	++p;
	SeekForward();
	return *this;
}
StringIteratorU8& StringIteratorU8::operator--() {
	RMLUI_ASSERT(p - 1 != view.begin());
	--p;
	SeekBack();
	return *this;
}
inline void StringIteratorU8::SeekBack() {
	p = StringUtilities::SeekBackU8(p, view.end());
}

inline void StringIteratorU8::SeekForward() {
	p = StringUtilities::SeekForwardU8(p, view.end());
}

}
}
