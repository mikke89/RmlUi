/*
 * Copyright (c) 2006 - 2008
 * Wandering Monster Studios Limited
 *
 * Any use of this program is governed by the terms of Wandering Monster
 * Studios Limited's Licence Agreement included with this program, a copy
 * of which can be obtained by contacting Wandering Monster Studios
 * Limited at info@wanderingmonster.co.nz.
 *
 */

template <typename SourceType, typename DestType>
bool TypeConverter<SourceType, DestType>::Convert(const SourceType& /*src*/, DestType& /*dest*/)
{
	EMP_ERRORMSG("No converter specified.");
	return false;
}

///
/// Partial specialisations at the top, as they full specialisations should be prefered.
///
template< typename T >
class TypeConverter< T, Stream >
{
public:
	static bool Convert(const T& src, Stream* dest)
	{
		String string_dest;
		bool result = TypeConverter< T, String >::Convert(src, string_dest);
		if (result)
			dest->Write(string_dest);

		return result;
	}
};

template< typename T >
class TypeConverter< Stream, T >
{
public:
	static bool Convert(Stream* src, T& dest, size_t length = String::npos)
	{
		String string_src;
		src->Read(string_src, src->Length() < length ? src->Length() : length);
		return TypeConverter< String, T >::Convert(string_src, dest);
	}
};

///
/// Full Specialisations
///

#define BASIC_CONVERTER(s, d) \
template<>	\
class TypeConverter< s, d > \
{ \
public: \
	static bool Convert(const s& src, d& dest) \
	{ \
		dest = (d)src; \
		return true; \
	} \
}

#define BASIC_CONVERTER_BOOL(s, d) \
template<>	\
class TypeConverter< s, d > \
{ \
public: \
	static bool Convert(const s& src, d& dest) \
	{ \
		dest = src != 0; \
		return true; \
	} \
}

#define PASS_THROUGH(t)	BASIC_CONVERTER(t, t)

/////////////////////////////////////////////////
// Simple pass through definitions for converting 
// to the same type (direct copy)
/////////////////////////////////////////////////
PASS_THROUGH(int);
PASS_THROUGH(unsigned int);
PASS_THROUGH(float);
PASS_THROUGH(bool);
PASS_THROUGH(char);
PASS_THROUGH(word);
PASS_THROUGH(Vector2i);
PASS_THROUGH(Vector2f);
PASS_THROUGH(Colourf);
PASS_THROUGH(Colourb);
PASS_THROUGH(String);

// Pointer types need to be typedef'd
class ScriptInterface;
typedef ScriptInterface* ScriptInterfacePtr;
PASS_THROUGH(ScriptInterfacePtr);
/*typedef void* voidPtr;
PASS_THROUGH(voidPtr);*/

template<>
class TypeConverter< Stream, Stream >
{
public:
	static bool Convert(Stream* src, Stream* dest)
	{
		return src->Write(dest, src->Length()) == src->Length();
	}
};

/////////////////////////////////////////////////
// Simple Types
/////////////////////////////////////////////////
BASIC_CONVERTER(bool, int);
BASIC_CONVERTER(bool, unsigned int);
BASIC_CONVERTER(bool, float);

BASIC_CONVERTER(int, unsigned int);
BASIC_CONVERTER_BOOL(int, bool);
BASIC_CONVERTER(int, float);

BASIC_CONVERTER_BOOL(float, bool);
BASIC_CONVERTER(float, int);
BASIC_CONVERTER(float, unsigned int);

/////////////////////////////////////////////////
// From string converters
/////////////////////////////////////////////////

#define STRING_FLOAT_CONVERTER(type) \
template<> \
class TypeConverter< String, type > \
{ \
public: \
	static bool Convert(const String& src, type& dest) \
	{ \
		dest = (type) atof(src.CString()); \
		return true; \
	} \
};
STRING_FLOAT_CONVERTER(float);
STRING_FLOAT_CONVERTER(double);

template<>
class TypeConverter< String, int >
{
public:
	static bool Convert(const String& src, int& dest)
	{
		return sscanf(src.CString(), "%d", &dest) == 1;
	}
};

template<>
class TypeConverter< String, unsigned int >
{
public:
	static bool Convert(const String& src, unsigned int& dest)
	{
		return sscanf(src.CString(), "%u", &dest) == 1;
	}
};

template<>
class TypeConverter< String, byte >
{
public:
	static bool Convert(const String& src, byte& dest)
	{
		int value;
		bool ret = sscanf(src.CString(), "%d", &value) == 1;
		dest = (byte) value;
		return ret && (value <= 255);
	}
};

template<>
class TypeConverter< String, bool >
{
public:
	static bool Convert(const String& src, bool& dest)
	{
		String lower = src.ToLower();
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

/*template<>
class TypeConverter< String, URL >
{
public:
	static bool Convert(const String& src, URL& dest)
	{
		return dest.SetURL(src);		
	}
};*/

template< typename DestType, typename InternalType, int count >
class TypeConverterStringVector
{
public:
	static bool Convert(const String& src, DestType& dest)
	{
		StringList string_list;
		StringUtilities::ExpandString(string_list, src);
		if (string_list.size() < count)
			return false;
		for (int i = 0; i < count; i++)
		{
			if (!TypeConverter< String, InternalType >::Convert(string_list[i], dest[i]))
				return false;
		}
		return true;
	}
};

#define STRING_VECTOR_CONVERTER(type, internal_type, count) \
template<> \
class TypeConverter< String, type > \
{ \
public: \
	static bool Convert(const String& src, type& dest) \
	{ \
		return TypeConverterStringVector< type, internal_type, count >::Convert(src, dest); \
	} \
}

STRING_VECTOR_CONVERTER(Vector2i, int, 2);
STRING_VECTOR_CONVERTER(Vector2f, float, 2);
//STRING_VECTOR_CONVERTER(Vector3i, int, 3);
//STRING_VECTOR_CONVERTER(Vector3f, float, 2);
//STRING_VECTOR_CONVERTER(Vector4i, int, 4);
//STRING_VECTOR_CONVERTER(Vector4f, float, 4);
STRING_VECTOR_CONVERTER(Colourf, float, 4);
STRING_VECTOR_CONVERTER(Colourb, byte, 4);
//STRING_VECTOR_CONVERTER(Quaternion, float, 4);

/////////////////////////////////////////////////
// To String Converters
/////////////////////////////////////////////////

#define FLOAT_STRING_CONVERTER(type) \
template<> \
class TypeConverter< type, String > \
{ \
public: \
	static bool Convert(const type& src, String& dest) \
	{ \
		return dest.FormatString(32, "%.4f", src) > 0; \
	} \
};
FLOAT_STRING_CONVERTER(float);
FLOAT_STRING_CONVERTER(double);

template<>
class TypeConverter< int, String >
{
public:
	static bool Convert(const int& src, String& dest)
	{
		return dest.FormatString(32, "%d", src) > 0;
	}
};

template<>
class TypeConverter< unsigned int, String >
{
public:
	static bool Convert(const unsigned int& src, String& dest)
	{
		return dest.FormatString(32, "%u", src) > 0;
	}
};

template<>
class TypeConverter< byte, String >
{
public:
	static bool Convert(const byte& src, String& dest)
	{
		return dest.FormatString(32, "%u", src) > 0;
	}
};

template<>
class TypeConverter< bool, String >
{
public:
	static bool Convert(const bool& src, String& dest)
	{
		dest = src ? "1" : "0";
		return true;
	}
};

template<>
class TypeConverter< char*, String >
{
public:
	static bool Convert(char* const & src, String& dest)
	{
		dest = src;
		return true;
	}
};

/*template<>
class TypeConverter< URL, String >
{
public:
	static bool Convert(const URL& src, String& dest)
	{
		dest = src.GetURL();
		return true;
	}
};*/


template< typename SourceType, typename InternalType, int count >
class TypeConverterVectorString
{
public:
	static bool Convert(const SourceType& src, String& dest)
	{
		dest = "";
		for (int i = 0; i < count; i++)
		{
			String value;
			if (!TypeConverter< InternalType, String >::Convert(src[i], value))
				return false;
			
			dest += value;
			if (i < count - 1)
				dest += ", ";
		}
		return true;
	}
};

#define VECTOR_STRING_CONVERTER(type, internal_type, count) \
template<> \
class TypeConverter< type, String > \
{ \
public: \
	static bool Convert(const type& src, String& dest) \
	{ \
		return TypeConverterVectorString< type, internal_type, count >::Convert(src, dest); \
	} \
}

VECTOR_STRING_CONVERTER(Vector2i, int, 2);
VECTOR_STRING_CONVERTER(Vector2f, float, 2);
//VECTOR_STRING_CONVERTER(Vector3i, int, 3);
//VECTOR_STRING_CONVERTER(Vector3f, float, 3);
//VECTOR_STRING_CONVERTER(Vector4i, int, 4);
//VECTOR_STRING_CONVERTER(Vector4f, float, 4);
VECTOR_STRING_CONVERTER(Colourf, float, 4);
VECTOR_STRING_CONVERTER(Colourb, byte, 4);
//VECTOR_STRING_CONVERTER(Quaternion, float, 4);

#undef PASS_THROUGH
#undef BASIC_CONVERTER
#undef BASIC_CONVERTER_BOOL
#undef STRING_VECTOR_CONVERTER
#undef VECTOR_STRING_CONVERTER

/*#define EMP_ENUM_CONVERTER( enum_type ) \
template<> \
class TypeConverter< enum_type, String > \
{ \
public: \
	static bool Convert(const enum_type& src, String& dest) \
	{ \
		return dest.FormatString(32, "%d", src) > 0; \
	} \
};\
\
template<> \
class TypeConverter< String, enum_type > \
{ \
public: \
	static bool Convert(const String& src, enum_type& dest) \
	{ \
		int temp; \
		if (sscanf(src.CString(), "%d", &temp) == 1) \
		{ \
			dest = (enum_type) temp; \
			return true; \
		} \
\
		return false; \
	} \
};*/
