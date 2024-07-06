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

namespace Rml {

template <typename SourceType, typename DestType>
bool TypeConverter<SourceType, DestType>::Convert(const SourceType& /*src*/, DestType& /*dest*/)
{
	RMLUI_ERRORMSG("No converter specified.");
	return false;
}

#if defined(RMLUI_PLATFORM_WIN32) && defined(__MINGW32__)
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wformat"
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wformat-extra-args"
#endif

///
/// Full Specialisations
///

#define BASIC_CONVERTER(s, d)                      \
	template <>                                    \
	class TypeConverter<s, d> {                    \
	public:                                        \
		static bool Convert(const s& src, d& dest) \
		{                                          \
			dest = (d)src;                         \
			return true;                           \
		}                                          \
	}

#define BASIC_CONVERTER_BOOL(s, d)                 \
	template <>                                    \
	class TypeConverter<s, d> {                    \
	public:                                        \
		static bool Convert(const s& src, d& dest) \
		{                                          \
			dest = src != 0;                       \
			return true;                           \
		}                                          \
	}

#define PASS_THROUGH(t) BASIC_CONVERTER(t, t)

/////////////////////////////////////////////////
// Simple pass through definitions for converting
// to the same type (direct copy)
/////////////////////////////////////////////////
PASS_THROUGH(int);
PASS_THROUGH(unsigned int);
PASS_THROUGH(long);
PASS_THROUGH(unsigned long);
PASS_THROUGH(long long);
PASS_THROUGH(unsigned long long);
PASS_THROUGH(float);
PASS_THROUGH(double);
PASS_THROUGH(bool);
PASS_THROUGH(char);
PASS_THROUGH(Character);
PASS_THROUGH(Vector2i);
PASS_THROUGH(Vector2f);
PASS_THROUGH(Vector3i);
PASS_THROUGH(Vector3f);
PASS_THROUGH(Vector4i);
PASS_THROUGH(Vector4f);
PASS_THROUGH(Colourf);
PASS_THROUGH(Colourb);
PASS_THROUGH(String);

// Pointer types need to be typedef'd
class ScriptInterface;
typedef ScriptInterface* ScriptInterfacePtr;
PASS_THROUGH(ScriptInterfacePtr);
typedef void* voidPtr;
PASS_THROUGH(voidPtr);

/////////////////////////////////////////////////
// Simple Types
/////////////////////////////////////////////////
BASIC_CONVERTER(bool, int);
BASIC_CONVERTER(bool, unsigned int);
BASIC_CONVERTER(bool, long);
BASIC_CONVERTER(bool, unsigned long);
BASIC_CONVERTER(bool, long long);
BASIC_CONVERTER(bool, unsigned long long);
BASIC_CONVERTER(bool, float);
BASIC_CONVERTER(bool, double);

BASIC_CONVERTER_BOOL(int, bool);
BASIC_CONVERTER(int, unsigned int);
BASIC_CONVERTER(int, long);
BASIC_CONVERTER(int, unsigned long);
BASIC_CONVERTER(int, long long);
BASIC_CONVERTER(int, unsigned long long);
BASIC_CONVERTER(int, float);
BASIC_CONVERTER(int, double);

BASIC_CONVERTER_BOOL(unsigned int, bool);
BASIC_CONVERTER(unsigned int, int);
BASIC_CONVERTER(unsigned int, long);
BASIC_CONVERTER(unsigned int, unsigned long);
BASIC_CONVERTER(unsigned int, long long);
BASIC_CONVERTER(unsigned int, unsigned long long);
BASIC_CONVERTER(unsigned int, float);
BASIC_CONVERTER(unsigned int, double);

BASIC_CONVERTER_BOOL(long, bool);
BASIC_CONVERTER(long, int);
BASIC_CONVERTER(long, unsigned int);
BASIC_CONVERTER(long, unsigned long);
BASIC_CONVERTER(long, long long);
BASIC_CONVERTER(long, unsigned long long);
BASIC_CONVERTER(long, float);
BASIC_CONVERTER(long, double);

BASIC_CONVERTER_BOOL(unsigned long, bool);
BASIC_CONVERTER(unsigned long, int);
BASIC_CONVERTER(unsigned long, unsigned int);
BASIC_CONVERTER(unsigned long, long);
BASIC_CONVERTER(unsigned long, long long);
BASIC_CONVERTER(unsigned long, unsigned long long);
BASIC_CONVERTER(unsigned long, float);
BASIC_CONVERTER(unsigned long, double);

BASIC_CONVERTER_BOOL(long long, bool);
BASIC_CONVERTER(long long, int);
BASIC_CONVERTER(long long, unsigned int);
BASIC_CONVERTER(long long, long);
BASIC_CONVERTER(long long, unsigned long);
BASIC_CONVERTER(long long, unsigned long long);
BASIC_CONVERTER(long long, float);
BASIC_CONVERTER(long long, double);

BASIC_CONVERTER_BOOL(unsigned long long, bool);
BASIC_CONVERTER(unsigned long long, int);
BASIC_CONVERTER(unsigned long long, unsigned int);
BASIC_CONVERTER(unsigned long long, long);
BASIC_CONVERTER(unsigned long long, unsigned long);
BASIC_CONVERTER(unsigned long long, long long);
BASIC_CONVERTER(unsigned long long, float);
BASIC_CONVERTER(unsigned long long, double);

BASIC_CONVERTER_BOOL(float, bool);
BASIC_CONVERTER(float, int);
BASIC_CONVERTER(float, long);
BASIC_CONVERTER(float, unsigned long);
BASIC_CONVERTER(float, long long);
BASIC_CONVERTER(float, unsigned long long);
BASIC_CONVERTER(float, double);
BASIC_CONVERTER(float, unsigned int);

BASIC_CONVERTER_BOOL(double, bool);
BASIC_CONVERTER(double, int);
BASIC_CONVERTER(double, long);
BASIC_CONVERTER(double, unsigned long);
BASIC_CONVERTER(double, long long);
BASIC_CONVERTER(double, unsigned long long);
BASIC_CONVERTER(double, float);
BASIC_CONVERTER(double, unsigned int);

BASIC_CONVERTER(char, Character);

/////////////////////////////////////////////////
// From string converters
/////////////////////////////////////////////////

#define STRING_FLOAT_CONVERTER(type)                       \
	template <>                                            \
	class TypeConverter<String, type> {                    \
	public:                                                \
		static bool Convert(const String& src, type& dest) \
		{                                                  \
			dest = (type)atof(src.c_str());                \
			return true;                                   \
		}                                                  \
	}
STRING_FLOAT_CONVERTER(float);
STRING_FLOAT_CONVERTER(double);

template <>
class TypeConverter<String, int> {
public:
	static bool Convert(const String& src, int& dest) { return sscanf(src.c_str(), "%d", &dest) == 1; }
};

template <>
class TypeConverter<String, unsigned int> {
public:
	static bool Convert(const String& src, unsigned int& dest) { return sscanf(src.c_str(), "%u", &dest) == 1; }
};

template <>
class TypeConverter<String, long> {
public:
	static bool Convert(const String& src, long& dest) { return sscanf(src.c_str(), "%ld", &dest) == 1; }
};

template <>
class TypeConverter<String, unsigned long> {
public:
	static bool Convert(const String& src, unsigned long& dest) { return sscanf(src.c_str(), "%lu", &dest) == 1; }
};

template <>
class TypeConverter<String, long long> {
public:
	static bool Convert(const String& src, long long& dest) { return sscanf(src.c_str(), "%lld", &dest) == 1; }
};

template <>
class TypeConverter<String, unsigned long long> {
public:
	static bool Convert(const String& src, unsigned long long& dest) { return sscanf(src.c_str(), "%llu", &dest) == 1; }
};

template <>
class TypeConverter<String, byte> {
public:
	static bool Convert(const String& src, byte& dest) { return sscanf(src.c_str(), "%hhu", &dest) == 1; }
};

template <>
class TypeConverter<String, bool> {
public:
	static bool Convert(const String& src, bool& dest)
	{
		String lower = StringUtilities::ToLower(src);
		if (lower == "1" || lower == "true")
		{
			dest = true;
			return true;
		}
		else if (lower == "0" || lower == "false")
		{
			dest = false;
			return true;
		}
		return false;
	}
};

template <typename DestType, typename InternalType, int count>
class TypeConverterStringVector {
public:
	static bool Convert(const String& src, DestType& dest)
	{
		StringList string_list;
		StringUtilities::ExpandString(string_list, src);
		if (string_list.size() < count)
			return false;
		for (int i = 0; i < count; i++)
		{
			if (!TypeConverter<String, InternalType>::Convert(string_list[i], dest[i]))
				return false;
		}
		return true;
	}
};

#define STRING_VECTOR_CONVERTER(type, internal_type, count)                                   \
	template <>                                                                               \
	class TypeConverter<String, type> {                                                       \
	public:                                                                                   \
		static bool Convert(const String& src, type& dest)                                    \
		{                                                                                     \
			return TypeConverterStringVector<type, internal_type, count>::Convert(src, dest); \
		}                                                                                     \
	}

STRING_VECTOR_CONVERTER(Vector2i, int, 2);
STRING_VECTOR_CONVERTER(Vector2f, float, 2);
STRING_VECTOR_CONVERTER(Vector3i, int, 3);
STRING_VECTOR_CONVERTER(Vector3f, float, 3);
STRING_VECTOR_CONVERTER(Vector4i, int, 4);
STRING_VECTOR_CONVERTER(Vector4f, float, 4);
STRING_VECTOR_CONVERTER(Colourf, float, 4);

/////////////////////////////////////////////////
// To String Converters
/////////////////////////////////////////////////

#define FLOAT_STRING_CONVERTER(type)                       \
	template <>                                            \
	class TypeConverter<type, String> {                    \
	public:                                                \
		static bool Convert(const type& src, String& dest) \
		{                                                  \
			if (FormatString(dest, "%.3f", src) == 0)      \
				return false;                              \
			StringUtilities::TrimTrailingDotZeros(dest);   \
			return true;                                   \
		}                                                  \
	}
FLOAT_STRING_CONVERTER(float);
FLOAT_STRING_CONVERTER(double);

template <>
class TypeConverter<int, String> {
public:
	static bool Convert(const int& src, String& dest) { return FormatString(dest, "%d", src) > 0; }
};

template <>
class TypeConverter<unsigned int, String> {
public:
	static bool Convert(const unsigned int& src, String& dest) { return FormatString(dest, "%u", src) > 0; }
};

template <>
class TypeConverter<long, String> {
public:
	static bool Convert(const long& src, String& dest) { return FormatString(dest, "%ld", src) > 0; }
};

template <>
class TypeConverter<unsigned long, String> {
public:
	static bool Convert(const unsigned long& src, String& dest) { return FormatString(dest, "%lu", src) > 0; }
};

template <>
class TypeConverter<long long, String> {
public:
	static bool Convert(const long long& src, String& dest) { return FormatString(dest, "%lld", src) > 0; }
};

template <>
class TypeConverter<unsigned long long, String> {
public:
	static bool Convert(const unsigned long long& src, String& dest) { return FormatString(dest, "%llu", src) > 0; }
};

template <>
class TypeConverter<byte, String> {
public:
	static bool Convert(const byte& src, String& dest) { return FormatString(dest, "%hhu", src) > 0; }
};

template <>
class TypeConverter<bool, String> {
public:
	static bool Convert(const bool& src, String& dest)
	{
		dest = src ? "1" : "0";
		return true;
	}
};

template <>
class TypeConverter<char*, String> {
public:
	static bool Convert(char* const& src, String& dest)
	{
		dest = src;
		return true;
	}
};

template <>
class TypeConverter<void*, String> {
public:
	static bool Convert(void* const& src, String& dest) { return FormatString(dest, "%p", src) > 0; }
};

template <>
class TypeConverter<ScriptInterface*, String> {
public:
	static bool Convert(ScriptInterface* const& src, String& dest) { return FormatString(dest, "%p", static_cast<void*>(src)) > 0; }
};

template <>
class TypeConverter<char, String> {
public:
	static bool Convert(const char& src, String& dest) { return FormatString(dest, "%c", src) > 0; }
};

template <typename SourceType, typename InternalType, int count>
class TypeConverterVectorString {
public:
	static bool Convert(const SourceType& src, String& dest)
	{
		dest = "";
		for (int i = 0; i < count; i++)
		{
			String value;
			if (!TypeConverter<InternalType, String>::Convert(src[i], value))
				return false;

			dest += value;
			if (i < count - 1)
				dest += ", ";
		}
		return true;
	}
};

#define VECTOR_STRING_CONVERTER(type, internal_type, count)                                   \
	template <>                                                                               \
	class TypeConverter<type, String> {                                                       \
	public:                                                                                   \
		static bool Convert(const type& src, String& dest)                                    \
		{                                                                                     \
			return TypeConverterVectorString<type, internal_type, count>::Convert(src, dest); \
		}                                                                                     \
	}

VECTOR_STRING_CONVERTER(Vector2i, int, 2);
VECTOR_STRING_CONVERTER(Vector2f, float, 2);
VECTOR_STRING_CONVERTER(Vector3i, int, 3);
VECTOR_STRING_CONVERTER(Vector3f, float, 3);
VECTOR_STRING_CONVERTER(Vector4i, int, 4);
VECTOR_STRING_CONVERTER(Vector4f, float, 4);
VECTOR_STRING_CONVERTER(Colourf, float, 4);

template <typename SourceType>
class TypeConverter<SourceType, String> {
public:
	template <typename...>
	struct AlwaysFalse : std::integral_constant<bool, false> {};

	static bool Convert(const SourceType& /*src*/, String& /*dest*/)
	{
		static_assert(AlwaysFalse<SourceType>{},
			"The type converter was invoked on a type without a string converter, please define a converter from SourceType to String.");
		return false;
	}
};

#undef PASS_THROUGH
#undef BASIC_CONVERTER
#undef BASIC_CONVERTER_BOOL
#undef FLOAT_STRING_CONVERTER
#undef STRING_FLOAT_CONVERTER
#undef STRING_VECTOR_CONVERTER
#undef VECTOR_STRING_CONVERTER

#if defined(RMLUI_PLATFORM_WIN32) && defined(__MINGW32__)
	#pragma GCC diagnostic pop
	#pragma GCC diagnostic pop
#endif

} // namespace Rml
