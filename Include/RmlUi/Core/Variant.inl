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

namespace Rml {
namespace Core {

inline Variant::Type Variant::GetType() const
{
	return type;
}

template< typename T >
Variant::Variant(T&& t) : type(NONE)
{
	Set(std::forward<T>(t));
}

template< typename T >
Variant& Variant::operator=(T&& t)
{
	Clear();
	Set(std::forward<T>(t));
	return *this;
}

// Templatised data accessor.
template< typename T >
bool Variant::GetInto(T& value) const
{
	switch (type)
	{
	case BYTE:
		return TypeConverter< byte, T >::Convert(*(byte*)data, value);
		break;

	case CHAR:
		return TypeConverter< char, T >::Convert(*(char*)data, value);
		break;

	case FLOAT:
		return TypeConverter< float, T >::Convert(*(float*)data, value);
		break;

	case INT:
		return TypeConverter< int, T >::Convert(*(int*)data, value);
		break;

	case STRING:
		return TypeConverter< String, T >::Convert(*(String*)data, value);
		break;

	case WORD:
		return TypeConverter< Character, T >::Convert(*(Character*)data, value);
		break;

	case VECTOR2:
		return TypeConverter< Vector2f, T >::Convert(*(Vector2f*)data, value);
		break;

	case VECTOR3:
		return TypeConverter< Vector3f, T >::Convert(*(Vector3f*)data, value);
		break;

	case VECTOR4:
		return TypeConverter< Vector4f, T >::Convert(*(Vector4f*)data, value);
		break;

	case COLOURF:
		return TypeConverter< Colourf, T >::Convert(*(Colourf*)data, value);
		break;

	case COLOURB:
		return TypeConverter< Colourb, T >::Convert(*(Colourb*)data, value);
		break;

	case SCRIPTINTERFACE:
		return TypeConverter< ScriptInterface*, T >::Convert(*(ScriptInterface * *)data, value);
		break;

	case VOIDPTR:
		return TypeConverter< void*, T >::Convert(*(void**)data, value);
		break;

	case TRANSFORMPTR:
		return TypeConverter< TransformPtr, T >::Convert(*(TransformPtr*)data, value);
		break;

	case TRANSITIONLIST:
		return TypeConverter< TransitionList, T >::Convert(*(TransitionList*)data, value);
		break;

	case ANIMATIONLIST:
		return TypeConverter< AnimationList, T >::Convert(*(AnimationList*)data, value);
		break;

	case DECORATORSPTR:
		return TypeConverter< DecoratorsPtr, T >::Convert(*(DecoratorsPtr*)data, value);
		break;

	case FONTEFFECTSPTR:
		return TypeConverter< FontEffectsPtr, T >::Convert(*(FontEffectsPtr*)data, value);
		break;
	case NONE:
		break;
	}

	return false;
}

// Templatised data accessor.
template< typename T >
T Variant::Get(T default_value) const
{
	GetInto(default_value);
	return default_value;
}

template<typename T>
inline const T& Variant::GetReference() const
{
	return *(T*)data;
}

}
}
