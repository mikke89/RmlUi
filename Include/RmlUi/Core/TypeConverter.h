#pragma once

#include "Log.h"
#include "Platform.h"
#include "StringUtilities.h"
#include "Types.h"
#include <stdio.h>
#include <stdlib.h>
#include <type_traits>

namespace Rml {

enum class Unit;

/**
    Templatised TypeConverters with Template Specialisation.

    These converters convert from source types to destination types.
    They're mainly useful in things like dictionaries and serialisers.
*/

template <typename SourceType, typename DestType>
class TypeConverter {
public:
	static bool Convert(const SourceType& src, DestType& dest);
};

template <typename T>
inline String ToString(const T& value, String default_value = String())
{
	String result = default_value;
	TypeConverter<T, String>::Convert(value, result);
	return result;
}

template <typename T>
inline T FromString(const String& string, T default_value = T())
{
	T result = default_value;
	TypeConverter<String, T>::Convert(string, result);
	return result;
}

// Some more complex types are defined in cpp-file

template <>
class TypeConverter<Unit, String> {
public:
	RMLUICORE_API static bool Convert(const Unit& src, String& dest);
};
template <>
class TypeConverter<Colourb, String> {
public:
	RMLUICORE_API static bool Convert(const Colourb& src, String& dest);
};
template <>
class TypeConverter<String, Colourb> {
public:
	RMLUICORE_API static bool Convert(const String& src, Colourb& dest);
};

template <>
class TypeConverter<TransformPtr, TransformPtr> {
public:
	RMLUICORE_API static bool Convert(const TransformPtr& src, TransformPtr& dest);
};

template <>
class TypeConverter<TransformPtr, String> {
public:
	RMLUICORE_API static bool Convert(const TransformPtr& src, String& dest);
};

template <>
class TypeConverter<TransitionList, TransitionList> {
public:
	RMLUICORE_API static bool Convert(const TransitionList& src, TransitionList& dest);
};
template <>
class TypeConverter<TransitionList, String> {
public:
	RMLUICORE_API static bool Convert(const TransitionList& src, String& dest);
};

template <>
class TypeConverter<AnimationList, AnimationList> {
public:
	RMLUICORE_API static bool Convert(const AnimationList& src, AnimationList& dest);
};
template <>
class TypeConverter<AnimationList, String> {
public:
	RMLUICORE_API static bool Convert(const AnimationList& src, String& dest);
};

template <>
class TypeConverter<DecoratorsPtr, DecoratorsPtr> {
public:
	RMLUICORE_API static bool Convert(const DecoratorsPtr& src, DecoratorsPtr& dest);
};
template <>
class TypeConverter<DecoratorsPtr, String> {
public:
	RMLUICORE_API static bool Convert(const DecoratorsPtr& src, String& dest);
};
template <>
class TypeConverter<FiltersPtr, FiltersPtr> {
public:
	RMLUICORE_API static bool Convert(const FiltersPtr& src, FiltersPtr& dest);
};
template <>
class TypeConverter<FiltersPtr, String> {
public:
	RMLUICORE_API static bool Convert(const FiltersPtr& src, String& dest);
};

template <>
class TypeConverter<FontEffectsPtr, FontEffectsPtr> {
public:
	RMLUICORE_API static bool Convert(const FontEffectsPtr& src, FontEffectsPtr& dest);
};
template <>
class TypeConverter<FontEffectsPtr, String> {
public:
	RMLUICORE_API static bool Convert(const FontEffectsPtr& src, String& dest);
};

template <>
class TypeConverter<ColorStopList, ColorStopList> {
public:
	RMLUICORE_API static bool Convert(const ColorStopList& src, ColorStopList& dest);
};
template <>
class TypeConverter<ColorStopList, String> {
public:
	RMLUICORE_API static bool Convert(const ColorStopList& src, String& dest);
};

template <>
class TypeConverter<BoxShadowList, BoxShadowList> {
public:
	RMLUICORE_API static bool Convert(const BoxShadowList& src, BoxShadowList& dest);
};
template <>
class TypeConverter<BoxShadowList, String> {
public:
	RMLUICORE_API static bool Convert(const BoxShadowList& src, String& dest);
};

} // namespace Rml

#include "TypeConverter.inl"
