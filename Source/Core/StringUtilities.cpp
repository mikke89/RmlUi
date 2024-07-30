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

#include "../../Include/RmlUi/Core/StringUtilities.h"
#include "../../Include/RmlUi/Core/Log.h"
#include <algorithm>
#include <limits.h>
#include <sstream>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

namespace Rml {

static int FormatString(String& string, const char* format, va_list argument_list)
{
	constexpr size_t InternalBufferSize = 256;
	char buffer[InternalBufferSize];
	char* buffer_ptr = buffer;

	size_t max_size = InternalBufferSize;
	int length = 0;

	for (int i = 0; i < 2; i++)
	{
		va_list argument_list_copy;
		va_copy(argument_list_copy, argument_list);

		length = vsnprintf(buffer_ptr, max_size, format, argument_list_copy);

		va_end(argument_list_copy);

		if (length < 0)
		{
			RMLUI_ERRORMSG("Error while formatting string");
			return 0;
		}

		if ((size_t)length < max_size || i > 0)
			break;

		max_size = (size_t)length + 1;
		buffer_ptr = new char[max_size];
	}

	string = buffer_ptr;

	if (buffer_ptr != buffer)
		delete[] buffer_ptr;

	return length;
}

int FormatString(String& string, const char* format, ...)
{
	va_list argument_list;
	va_start(argument_list, format);
	int result = FormatString(string, format, argument_list);
	va_end(argument_list);
	return result;
}
String CreateString(const char* format, ...)
{
	String result;
	va_list argument_list;
	va_start(argument_list, format);
	FormatString(result, format, argument_list);
	va_end(argument_list);
	return result;
}

static inline char CharToLower(char c)
{
	if (c >= 'A' && c <= 'Z')
		c += char('a' - 'A');
	return c;
}

String StringUtilities::ToLower(String string)
{
	std::transform(string.begin(), string.end(), string.begin(), &CharToLower);
	return string;
}

String StringUtilities::ToUpper(String string)
{
	std::transform(string.begin(), string.end(), string.begin(), [](char c) {
		if (c >= 'a' && c <= 'z')
			c -= char('a' - 'A');
		return c;
	});
	return string;
}

RMLUICORE_API String StringUtilities::EncodeRml(const String& string)
{
	String result;
	result.reserve(string.size());
	for (char c : string)
	{
		switch (c)
		{
		case '<': result += "&lt;"; break;
		case '>': result += "&gt;"; break;
		case '&': result += "&amp;"; break;
		case '"': result += "&quot;"; break;
		default: result += c; break;
		}
	}
	return result;
}

String StringUtilities::DecodeRml(const String& s)
{
	String result;
	result.reserve(s.size());
	for (size_t i = 0; i < s.size();)
	{
		if (s[i] == '&')
		{
			if (s[i + 1] == 'l' && s[i + 2] == 't' && s[i + 3] == ';')
			{
				result += "<";
				i += 4;
				continue;
			}
			else if (s[i + 1] == 'g' && s[i + 2] == 't' && s[i + 3] == ';')
			{
				result += ">";
				i += 4;
				continue;
			}
			else if (s[i + 1] == 'a' && s[i + 2] == 'm' && s[i + 3] == 'p' && s[i + 4] == ';')
			{
				result += "&";
				i += 5;
				continue;
			}
			else if (s[i + 1] == 'q' && s[i + 2] == 'u' && s[i + 3] == 'o' && s[i + 4] == 't' && s[i + 5] == ';')
			{
				result += "\"";
				i += 6;
				continue;
			}
			else if (s[i + 1] == '#')
			{
				size_t start = i + 2;
				if (s[i + 2] == 'x')
				{
					start++;
					size_t j = 0;
					for (; j < 8; j++)
					{
						const auto& c = s[start + j];
						if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')))
							break;
					}

					if (j > 0 && s[start + j] == ';')
					{
						String tmp = s.substr(start, j);
						const char* begin = tmp.c_str();
						char* end;
						unsigned long code_point = strtoul(begin, &end, 16);
						if (code_point != 0 && code_point != ULONG_MAX)
						{
							result += ToUTF8(static_cast<Character>(code_point));
							i = start + (end - begin) + 1;
							continue;
						}
					}
				}
				else
				{
					size_t j = 0;
					for (; j < 8; j++)
					{
						const auto& c = s[start + j];
						if (!(c >= '0' && c <= '9'))
							break;
					}

					if (j > 0 && s[start + j] == ';')
					{
						String tmp = s.substr(start, j);
						const char* begin = tmp.c_str();
						char* end;
						unsigned long code_point = strtoul(begin, &end, 10);
						if (code_point != 0 && code_point != ULONG_MAX)
						{
							result += ToUTF8(static_cast<Character>(code_point));
							i = start + (end - begin) + 1;
							continue;
						}
					}
				}
			}
		}
		result += s[i];
		i += 1;
	}
	return result;
}

String StringUtilities::Replace(String subject, const String& search, const String& replace)
{
	size_t pos = 0;
	while ((pos = subject.find(search, pos)) != String::npos)
	{
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
		else if (*ptr == quote && *(ptr - 1) != '\\')
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
		else if (!IsWhitespace(*ptr) || quote)
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

void StringUtilities::ExpandString(StringList& string_list, const String& string, const char delimiter, char quote_character, char unquote_character,
	bool ignore_repeated_delimiters)
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
			else if (!ignore_repeated_delimiters)
				string_list.emplace_back();
			start_ptr = nullptr;
		}
		// Otherwise if its not white space or we're in quote mode, advance the pointers
		else if (!IsWhitespace(*ptr) || quote_mode_depth > 0)
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

void StringUtilities::JoinString(String& string, const StringList& string_list, const char delimiter)
{
	for (size_t i = 0; i < string_list.size(); i++)
	{
		string += string_list[i];
		if (delimiter != '\0' && i < string_list.size() - 1)
			string += delimiter;
	}
}

String StringUtilities::StripWhitespace(const String& string)
{
	return StripWhitespace(StringView(string));
}

RMLUICORE_API String StringUtilities::StripWhitespace(StringView string)
{
	const char* start = string.begin();
	const char* end = string.end();

	while (start < end && IsWhitespace(*start))
		start++;

	while (end > start && IsWhitespace(*(end - 1)))
		end--;

	if (start < end)
		return String(start, end);

	return String();
}

void StringUtilities::TrimTrailingDotZeros(String& string)
{
	size_t new_size = string.size();
	for (size_t i = string.size() - 1; i < string.size(); i--)
	{
		if (string[i] == '.')
		{
			new_size = i;
			break;
		}
		else if (string[i] == '0')
			new_size = i;
		else
			break;
	}

	if (new_size < string.size())
		string.resize(new_size);
}

bool StringUtilities::StartsWith(StringView string, StringView start)
{
	if (string.size() < start.size())
		return false;

	StringView substring(string.begin(), string.begin() + start.size());
	return substring == start;
}

bool StringUtilities::StringCompareCaseInsensitive(const StringView lhs, const StringView rhs)
{
	if (lhs.size() != rhs.size())
		return false;

	const char* left = lhs.begin();
	const char* right = rhs.begin();
	const char* const left_end = lhs.end();

	for (; left != left_end; ++left, ++right)
	{
		if (CharToLower(*left) != CharToLower(*right))
			return false;
	}

	return true;
}

Character StringUtilities::ToCharacter(const char* p, const char* p_end)
{
	RMLUI_ASSERTMSG(p && p != p_end, "ToCharacter expects a valid, non-empty input string");

	if ((*p & (1 << 7)) == 0)
		return static_cast<Character>(*p);

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
		return Character::Null;
	}

	if (p_end - p < num_bytes)
		return Character::Null;

	for (int i = 1; i < num_bytes; i++)
	{
		const char byte = *(p + i);
		if ((byte & 0b1100'0000) != 0b1000'0000)
		{
			// Invalid continuation byte
			return Character::Null;
		}

		code = ((code << 6) | (byte & 0b0011'1111));
	}

	return static_cast<Character>(code);
}

String StringUtilities::ToUTF8(Character character)
{
	return ToUTF8(&character, 1);
}

String StringUtilities::ToUTF8(const Character* characters, int num_characters)
{
	String result;
	result.reserve(num_characters);

	bool invalid_character = false;

	for (int i = 0; i < num_characters; i++)
	{
		char32_t c = (char32_t)characters[i];

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
			result += {char(((c >> 6) & l5) | h2), char((c & l6) | h1)};
		else if (c < 0x10000)
			result += {char(((c >> 12) & l4) | h3), char(((c >> 6) & l6) | h1), char((c & l6) | h1)};
		else if (c <= 0x10FFFF)
			result += {char(((c >> 18) & l3) | h4), char(((c >> 12) & l6) | h1), char(((c >> 6) & l6) | h1), char((c & l6) | h1)};
		else
			invalid_character = true;
	}

	if (invalid_character)
		Log::Message(Log::LT_WARNING, "One or more invalid code points encountered while encoding to UTF-8.");

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

int StringUtilities::ConvertCharacterOffsetToByteOffset(StringView string, int character_offset)
{
	if (character_offset >= (int)string.size())
		return (int)string.size();

	int character_count = 0;
	for (auto it = StringIteratorU8(string.begin(), string.begin(), string.end()); it; ++it)
	{
		character_count += 1;
		if (character_count > character_offset)
			return (int)it.offset();
	}
	return (int)string.size();
}

int StringUtilities::ConvertByteOffsetToCharacterOffset(StringView string, int byte_offset)
{
	int character_count = 0;
	for (auto it = StringIteratorU8(string.begin(), string.begin(), string.end()); it; ++it)
	{
		if (it.offset() >= byte_offset)
			break;
		character_count += 1;
	}
	return character_count;
}

StringView::StringView()
{
	const char* empty_string = "";
	p_begin = empty_string;
	p_end = empty_string;
}

StringView::StringView(const char* p_begin, const char* p_end) : p_begin(p_begin), p_end(p_end)
{
	RMLUI_ASSERT(p_end >= p_begin);
}
StringView::StringView(const String& string) : p_begin(string.data()), p_end(string.data() + string.size()) {}
StringView::StringView(const String& string, size_t offset) : p_begin(string.data() + offset), p_end(string.data() + string.size()) {}
StringView::StringView(const String& string, size_t offset, size_t count) :
	p_begin(string.data() + offset), p_end(string.data() + std::min<size_t>(offset + count, string.size()))
{}

bool StringView::operator==(const StringView& other) const
{
	return size() == other.size() && strncmp(p_begin, other.p_begin, size()) == 0;
}

StringIteratorU8::StringIteratorU8(const char* p_begin, const char* p, const char* p_end) : view(p_begin, p_end), p(p) {}
StringIteratorU8::StringIteratorU8(StringView string) : view(string), p(view.begin()) {}
StringIteratorU8::StringIteratorU8(const String& string) : view(string), p(string.data()) {}
StringIteratorU8::StringIteratorU8(const String& string, size_t offset) : view(string), p(string.data() + offset) {}
StringIteratorU8::StringIteratorU8(const String& string, size_t offset, size_t count) : view(string, 0, offset + count), p(string.data() + offset) {}
StringIteratorU8& StringIteratorU8::operator++()
{
	RMLUI_ASSERT(p < view.end());
	++p;
	SeekForward();
	return *this;
}
StringIteratorU8& StringIteratorU8::operator--()
{
	RMLUI_ASSERT(p >= view.begin());
	--p;
	SeekBack();
	return *this;
}
inline void StringIteratorU8::SeekBack()
{
	p = StringUtilities::SeekBackwardUTF8(p, view.begin());
}

inline void StringIteratorU8::SeekForward()
{
	p = StringUtilities::SeekForwardUTF8(p, view.end());
}

} // namespace Rml
