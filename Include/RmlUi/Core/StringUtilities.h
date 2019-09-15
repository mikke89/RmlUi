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

#ifndef RMLUICORESTRINGUTILITIES_H
#define RMLUICORESTRINGUTILITIES_H

#include <algorithm>
#include <stddef.h>
#include "Header.h"
#include "Types.h"

namespace Rml {
namespace Core {

/**
	Helper functions for string manipulation.
	@author Lloyd Weehuizen
 */

// Redefine Windows APIs as their STDC counterparts.
#ifdef RMLUI_PLATFORM_WIN32
	#define strcasecmp stricmp
	#define strncasecmp strnicmp
#endif


/// Construct a string using sprintf-style syntax.
RMLUICORE_API String CreateString(size_t max_size, const char* format, ...);

/// Format to a string using sprintf-style syntax.
RMLUICORE_API int FormatString(String& string, size_t max_size, const char* format, ...);


namespace StringUtilities
{
	/// Expands character-delimited list of values in a single string to a whitespace-trimmed list
	/// of values.
	/// @param[out] string_list Resulting list of values.
	/// @param[in] string String to expand.
	/// @param[in] delimiter Delimiter found between entries in the string list.
	RMLUICORE_API void ExpandString(StringList& string_list, const String& string, const char delimiter = ',');
	/// Expands character-delimited list of values with custom quote characters.
	/// @param[out] string_list Resulting list of values.
	/// @param[in] string String to expand.
	/// @param[in] delimiter Delimiter found between entries in the string list.
	/// @param[in] quote_character Begin quote
	/// @param[in] unquote_character End quote
	/// @param[in] ignore_repeated_delimiters If true, repeated values of the delimiter will not add additional entries to the list.
	RMLUICORE_API void ExpandString(StringList& string_list, const String& string, const char delimiter, char quote_character, char unquote_character, bool ignore_repeated_delimiters = false);
	/// Joins a list of string values into a single string separated by a character delimiter.
	/// @param[out] string Resulting concatenated string.
	/// @param[in] string_list Input list of string values.
	/// @param[in] delimiter Delimiter to insert between the individual values.
	RMLUICORE_API void JoinString(String& string, const StringList& string_list, const char delimiter = ',');

	/// Converts a string in UTF-8 encoding to a wide string in UCS-2 encoding. The UCS-2 words will
	/// be encoded as either big- or little-endian, depending on the host processor.
	/// Reports a warning if the conversion fails.
	RMLUICORE_API WString ToUCS2(const String& str);

	/// Convert UTF8 string to UTF16.
	RMLUICORE_API WString ToUTF16(const String& str);

	/// Converts a wide string in UTF16 encoding into a string in UTF8 encoding. This
	/// function assumes the endianness of the input words to be the same as the host processor.
	/// Reports a warning if the conversion fails.
	RMLUICORE_API String ToUTF8(const WString& wstr);

	/// Returns number of characters in UTF8 string.
	RMLUICORE_API size_t LengthU8(const String& str);

	/// Converts upper-case characters in string to lower-case.
	RMLUICORE_API String ToLower(const String& string);

	// Replaces all occurences of 'search' in 'subject' with 'replace'.
	RMLUICORE_API String Replace(String subject, const String& search, const String& replace);
	// Replaces all occurences of 'search' in 'subject' with 'replace'.
	RMLUICORE_API String Replace(String subject, char search, char replace);

	/// Checks if a given value is a whitespace character.
	/// @param[in] x The character to evaluate.
	/// @return True if the character is whitespace, false otherwise.
	template < typename CharacterType >
	inline bool IsWhitespace(CharacterType x)
	{
		return (x == '\r' || x == '\n' || x == ' ' || x == '\t');
	}

	/// Strip whitespace characters from the beginning and end of a string.
	/// @param[in] string The string to trim.
	/// @return The stripped string.
	RMLUICORE_API String StripWhitespace(const String& string);

	/// Operator for STL containers using strings.
	struct RMLUICORE_API StringComparei
	{
		bool operator()(const String& lhs, const String& rhs) const;
	};

	RMLUICORE_API CodePoint ToCodePoint(const char* p);
	RMLUICORE_API String ToUTF8(CodePoint code_point);


	//struct StringView {
	//	const char* p_begin;
	//	const char* p_end;

	//	StringView(const String& string) : p_begin(string.data()), p_end(string.data() + string.size()) { }
	//	StringView(const String& string, size_t offset) : p_begin(string.data() + offset), p_end(string.data() + string.size()) { }
	//	StringView(const String& string, size_t offset, size_t count) : p_begin(string.data() + offset), p_end(string.data() + std::min(offset + count, string.size())) { }
	//};

	inline const char* SeekForwardU8(const char* p, const char* p_end)
	{
		while (p != p_end && (*p & 0b1100'0000) == 0b1000'0000)
			++p;
		return p;
	}
	inline const char* SeekBackU8(const char* p, const char* p_begin)
	{
		while ((p + 1) != p_begin && (*p & 0b1100'0000) == 0b1000'0000)
			--p;
		return p;
	}



}

	class UTF8Iterator {
		// p in [p_begin, p_end)
		const char* p_begin;
		const char* p;
		const char* p_end;

		inline void SeekBack() {
			p = StringUtilities::SeekBackU8(p, p_end);
		}

		inline void SeekForward() {
			p = StringUtilities::SeekForwardU8(p, p_end);
		}
		
	public:
		UTF8Iterator(const char* p_begin, const char* p, const char* p_end) : p_begin(p_begin), p(p), p_end(p_end) { SeekForward(); }
		UTF8Iterator(const String& string) : p_begin(string.data()), p(string.data()), p_end(string.data() + string.size()) { SeekForward(); }
		UTF8Iterator(const String& string, size_t offset) : p_begin(string.data()), p(string.data() + offset), p_end(string.data() + string.size()) { SeekForward(); }
		//UTF8Iterator(const String& string, size_t offset, size_t count) : p_begin(string.data()), p(string.data() + offset), p_end(string.data() + std::min(offset + count, string.size())) { SeekForward(); }

		UTF8Iterator& operator++() {
			RMLUI_ASSERT(p != p_end);
			++p;
			SeekForward();
			return *this;
		}
		UTF8Iterator& operator--() {
			RMLUI_ASSERT(p - 1 != p_begin);
			--p;
			SeekBack();
			return *this;
		}

		CodePoint operator*() const { return StringUtilities::ToCodePoint(p); }

		operator bool() const { return (p != p_begin - 1) && (p != p_end); }

		bool operator==(const UTF8Iterator& other) const { return p == other.p; }
		bool operator!=(const UTF8Iterator& other) const { return !(*this == other); }
	};



	class UTF8Parser {
		UTF8Iterator _begin;
		UTF8Iterator _end;

	public:

		UTF8Parser(const String& string) : _begin(string.data()), _end(string.data() + string.size()) {}
		UTF8Parser(const String& string, size_t offset) : _begin(string.data() + offset), _end(string.data() + string.size()) {}
		//UTF8Parser(const String& string, size_t offset, size_t count) : _begin(string.data() + offset), _end(string.data() + std::min(offset + count, string.size())) {}

		UTF8Iterator begin() const {
			return _begin;
		}

		UTF8Iterator end() const {
			return _end;
		}
	};




}
}

#endif
