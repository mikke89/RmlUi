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

inline Variant::Type Variant::GetType() const
{
	return type;
}

template <typename T, typename>
Variant::Variant(T&& t)
{
	Set(std::forward<T>(t));
}

template <typename T, typename>
Variant& Variant::operator=(T&& t)
{
	Clear();
	Set(std::forward<T>(t));
	return *this;
}

template <typename T, typename std::enable_if<!std::is_enum<T>::value, int>::type>
bool Variant::GetInto(T& value) const
{
	switch (type)
	{
	case BOOL: return TypeConverter<bool, T>::Convert(*reinterpret_cast<const bool*>(data), value);
	case BYTE: return TypeConverter<byte, T>::Convert(*reinterpret_cast<const byte*>(data), value);
	case CHAR: return TypeConverter<char, T>::Convert(*reinterpret_cast<const char*>(data), value);
	case FLOAT: return TypeConverter<float, T>::Convert(*reinterpret_cast<const float*>(data), value);
	case DOUBLE: return TypeConverter<double, T>::Convert(*reinterpret_cast<const double*>(data), value);
	case INT: return TypeConverter<int, T>::Convert(*reinterpret_cast<const int*>(data), value);
	case INT64: return TypeConverter<int64_t, T>::Convert(*reinterpret_cast<const int64_t*>(data), value);
	case UINT: return TypeConverter<unsigned int, T>::Convert(*reinterpret_cast<const unsigned int*>(data), value);
	case UINT64: return TypeConverter<uint64_t, T>::Convert(*reinterpret_cast<const uint64_t*>(data), value);
	case STRING: return TypeConverter<String, T>::Convert(*reinterpret_cast<const String*>(data), value);
	case VECTOR2: return TypeConverter<Vector2f, T>::Convert(*reinterpret_cast<const Vector2f*>(data), value);
	case VECTOR3: return TypeConverter<Vector3f, T>::Convert(*reinterpret_cast<const Vector3f*>(data), value);
	case VECTOR4: return TypeConverter<Vector4f, T>::Convert(*reinterpret_cast<const Vector4f*>(data), value);
	case COLOURF: return TypeConverter<Colourf, T>::Convert(*reinterpret_cast<const Colourf*>(data), value);
	case COLOURB: return TypeConverter<Colourb, T>::Convert(*reinterpret_cast<const Colourb*>(data), value);
	case SCRIPTINTERFACE: return TypeConverter<ScriptInterface*, T>::Convert(*reinterpret_cast<ScriptInterface* const*>(data), value);
	case VOIDPTR: return TypeConverter<void*, T>::Convert(*reinterpret_cast<void* const*>(data), value);
	case TRANSFORMPTR: return TypeConverter<TransformPtr, T>::Convert(*reinterpret_cast<const TransformPtr*>(data), value);
	case TRANSITIONLIST: return TypeConverter<TransitionList, T>::Convert(*reinterpret_cast<const TransitionList*>(data), value);
	case ANIMATIONLIST: return TypeConverter<AnimationList, T>::Convert(*reinterpret_cast<const AnimationList*>(data), value);
	case DECORATORSPTR: return TypeConverter<DecoratorsPtr, T>::Convert(*reinterpret_cast<const DecoratorsPtr*>(data), value);
	case FILTERSPTR: return TypeConverter<FiltersPtr, T>::Convert(*reinterpret_cast<const FiltersPtr*>(data), value);
	case FONTEFFECTSPTR: return TypeConverter<FontEffectsPtr, T>::Convert(*reinterpret_cast<const FontEffectsPtr*>(data), value);
	case COLORSTOPLIST: return TypeConverter<ColorStopList, T>::Convert(*(ColorStopList*)data, value); break;
	case BOXSHADOWLIST: return TypeConverter<BoxShadowList, T>::Convert(*reinterpret_cast<const BoxShadowList*>(data), value);
	case NONE: break;
	}

	return false;
}

template <typename T, typename std::enable_if<std::is_enum<T>::value, int>::type>
bool Variant::GetInto(T& value) const
{
	static_assert(sizeof(T) <= sizeof(int64_t), "Enum underlying type exceeds maximum supported integer type size");
	int64_t stored_value = 0;
	if (GetInto(stored_value))
	{
		value = static_cast<T>(stored_value);
		return true;
	}
	return false;
}

template <typename T>
T Variant::Get(T default_value) const
{
	GetInto(default_value);
	return default_value;
}

template <typename T>
const T& Variant::GetReference() const
{
	return *reinterpret_cast<const T*>(&data);
}

template <typename T, typename>
void Variant::Set(const T value)
{
	static_assert(sizeof(T) <= sizeof(int64_t), "Enum underlying type exceeds maximum supported integer type size");
	type = INT64;
	*(reinterpret_cast<int64_t*>(data)) = static_cast<int64_t>(value);
}

} // namespace Rml
