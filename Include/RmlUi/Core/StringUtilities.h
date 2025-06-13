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

#ifndef RMLUI_CORE_STRINGUTILITIES_H
#define RMLUI_CORE_STRINGUTILITIES_H

#include "Header.h"
#include "Types.h"

namespace Rml {

/**
    Helper functions for string manipulation.
    @author Lloyd Weehuizen
 */

class StringView;

/// Construct a string using sprintf-style syntax.
RMLUICORE_API String CreateString(const char* format, ...) RMLUI_ATTRIBUTE_FORMAT_PRINTF(1, 2);

/// Format to a string using sprintf-style syntax.
RMLUICORE_API int FormatString(String& string, const char* format, ...) RMLUI_ATTRIBUTE_FORMAT_PRINTF(2, 3);

namespace StringUtilities {
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
	RMLUICORE_API void ExpandString(StringList& string_list, const String& string, const char delimiter, char quote_character, char unquote_character,
		bool ignore_repeated_delimiters = false);
	/// Joins a list of string values into a single string separated by a character delimiter.
	/// @param[out] string Resulting concatenated string.
	/// @param[in] string_list Input list of string values.
	/// @param[in] delimiter Delimiter to insert between the individual values.
	RMLUICORE_API void JoinString(String& string, const StringList& string_list, const char delimiter = ',');

	/// Converts upper-case characters in string to lower-case.
	RMLUICORE_API String ToLower(String string);
	/// Converts lower-case characters in string to upper-case.
	RMLUICORE_API String ToUpper(String string);

	/// Encode RML characters, eg. '<' to '&lt;'
	RMLUICORE_API String EncodeRml(const String& string);

	/// Decode RML characters, eg. '&lt;' to '<'
	RMLUICORE_API String DecodeRml(const String& string);

	// Replaces all occurrences of 'search' in 'subject' with 'replace'.
	RMLUICORE_API String Replace(String subject, const String& search, const String& replace);
	// Replaces all occurrences of 'search' in 'subject' with 'replace'.
	RMLUICORE_API String Replace(String subject, char search, char replace);

	/// Checks if a given value is a whitespace character.
	inline bool IsWhitespace(const char x)
	{
		return (x == '\r' || x == '\n' || x == ' ' || x == '\t');
	}

	/// Strip whitespace characters from the beginning and end of a string.
	RMLUICORE_API String StripWhitespace(const String& string);

	/// Strip whitespace characters from the beginning and end of a string.
	RMLUICORE_API String StripWhitespace(StringView string);

	/// Trim trailing zeros and the dot from a string-representation of a number with a decimal point.
	/// @warning If the string does not represent a number _with_ a decimal point, the result is ill-defined.
	RMLUICORE_API void TrimTrailingDotZeros(String& string);

	/// Returns true if the string starts with the given value.
	RMLUICORE_API bool StartsWith(StringView string, StringView start);

	/// Case insensitive string comparison. Returns true if they compare equal.
	RMLUICORE_API bool StringCompareCaseInsensitive(StringView lhs, StringView rhs);

	// Decode the first code point in a zero-terminated UTF-8 string.
	RMLUICORE_API Character ToCharacter(const char* p, const char* p_end);

	// Encode a single code point as a UTF-8 string.
	RMLUICORE_API String ToUTF8(Character character);

	// Encode an array of code points as a UTF-8 string.
	RMLUICORE_API String ToUTF8(const Character* characters, int num_characters);

	/// Returns number of characters in a UTF-8 string.
	RMLUICORE_API size_t LengthUTF8(StringView string_view);

	// Seek forward in a UTF-8 string, skipping continuation bytes.
	inline const char* SeekForwardUTF8(const char* p, const char* p_end)
	{
		while (p != p_end && (*p & 0b1100'0000) == 0b1000'0000)
			++p;
		return p;
	}
	// Seek backward in a UTF-8 string, skipping continuation bytes.
	inline const char* SeekBackwardUTF8(const char* p, const char* p_begin)
	{
		while ((p + 1) != p_begin && (*p & 0b1100'0000) == 0b1000'0000)
			--p;
		return p;
	}

	/// Converts a character position in a UTF-8 string to a byte offset.
	RMLUICORE_API int ConvertCharacterOffsetToByteOffset(StringView string, int character_offset);

	/// Converts a byte offset of a UTF-8 string to a character position.
	RMLUICORE_API int ConvertByteOffsetToCharacterOffset(StringView string, int byte_offset);
} // namespace StringUtilities

/*
    A poor man's string view.

    The string view is agnostic to the underlying encoding, any operation will strictly operate on bytes.
*/

class RMLUICORE_API StringView {
public:
	StringView();
	StringView(const char* p_begin, const char* p_end);
	StringView(const String& string);
	StringView(const String& string, size_t offset);
	StringView(const String& string, size_t offset, size_t count);

	// Constructor for null-terminated string literals.
	template <size_t N>
	StringView(const char (&string)[N]) : p_begin(string), p_end(string + (N - 1))
	{
		static_assert(N >= 1, "");
	}

	// String comparison to another view
	bool operator==(const StringView& other) const;
	inline bool operator!=(const StringView& other) const { return !(*this == other); }

	inline const char* begin() const { return p_begin; }
	inline const char* end() const { return p_end; }

	inline bool empty() const { return p_begin == p_end; }
	inline size_t size() const { return size_t(p_end - p_begin); }

	explicit inline operator String() const { return String(p_begin, p_end); }

private:
	const char* p_begin;
	const char* p_end;
};

/*
    An iterator for UTF-8 strings.

    The increment and decrement operations will move to the beginning of the next or the previous
    UTF-8 character, respectively. The dereference operator will resolve the current code point.

*/

class RMLUICORE_API StringIteratorU8 {
public:
	StringIteratorU8(const char* p_begin, const char* p, const char* p_end);
	StringIteratorU8(StringView string);
	StringIteratorU8(const String& string);
	StringIteratorU8(const String& string, size_t offset);
	StringIteratorU8(const String& string, size_t offset, size_t count);

	// Seeks forward to the next UTF-8 character. Iterator must be valid.
	StringIteratorU8& operator++();
	// Seeks back to the previous UTF-8 character. Iterator must be valid.
	StringIteratorU8& operator--();

	// Returns the codepoint at the current position. The iterator must be dereferencable.
	inline Character operator*() const { return StringUtilities::ToCharacter(p, view.end()); }

	// Returns false when the iterator is located just outside the valid part of the string.
	explicit inline operator bool() const { return (p != view.begin() - 1) && (p != view.end()); }

	bool operator==(const StringIteratorU8& other) const { return p == other.p; }
	bool operator!=(const StringIteratorU8& other) const { return !(*this == other); }

	// Return a pointer to the current position.
	inline const char* get() const { return p; }

	// Return offset from the beginning of string. Note: Can return negative if decremented.
	std::ptrdiff_t offset() const { return p - view.begin(); }

private:
	StringView view;
	// 'p' can be dereferenced if and only if inside [view.begin, view.end)
	const char* p;

	inline void SeekForward();
	inline void SeekBack();
};

} // namespace Rml
#endif
