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

#include "../../Include/RmlUi/Core/Variant.h"
#include "../../Include/RmlUi/Core/DecorationTypes.h"
#include <string.h>

namespace Rml {

Variant::Variant()
{
	// Make sure our object size assumptions fit inside the static buffer
	static_assert(sizeof(Colourb) <= LOCAL_DATA_SIZE, "Local data too small for Colourb");
	static_assert(sizeof(Colourf) <= LOCAL_DATA_SIZE, "Local data too small for Colourf");
	static_assert(sizeof(Vector4f) <= LOCAL_DATA_SIZE, "Local data too small for Vector4f");
	static_assert(sizeof(String) <= LOCAL_DATA_SIZE, "Local data too small for String");
	static_assert(sizeof(TransformPtr) <= LOCAL_DATA_SIZE, "Local data too small for TransformPtr");
	static_assert(sizeof(TransitionList) <= LOCAL_DATA_SIZE, "Local data too small for TransitionList");
	static_assert(sizeof(AnimationList) <= LOCAL_DATA_SIZE, "Local data too small for AnimationList");
	static_assert(sizeof(DecoratorsPtr) <= LOCAL_DATA_SIZE, "Local data too small for DecoratorsPtr");
	static_assert(sizeof(FiltersPtr) <= LOCAL_DATA_SIZE, "Local data too small for FiltersPtr");
	static_assert(sizeof(FontEffectsPtr) <= LOCAL_DATA_SIZE, "Local data too small for FontEffectsPtr");
	static_assert(sizeof(ColorStopList) <= LOCAL_DATA_SIZE, "Local data too small for ColorStopList");
	static_assert(sizeof(BoxShadowList) <= LOCAL_DATA_SIZE, "Local data too small for BoxShadowList");
}

Variant::Variant(const Variant& copy)
{
	Set(copy);
}

Variant::Variant(Variant&& other) noexcept
{
	Set(std::move(other));
}

Variant::~Variant()
{
	Clear();
}

void Variant::Clear()
{
	// Free any allocated types.
	switch (type)
	{
	case STRING:
	{
		// Clean up the string.
		String* string = (String*)data;
		string->~String();
	}
	break;
	case TRANSFORMPTR:
	{
		// Clean up the transform.
		TransformPtr* transform = (TransformPtr*)data;
		transform->~TransformPtr();
	}
	break;
	case TRANSITIONLIST:
	{
		// Clean up the transition list.
		TransitionList* transition_list = (TransitionList*)data;
		transition_list->~TransitionList();
	}
	break;
	case ANIMATIONLIST:
	{
		// Clean up the transition list.
		AnimationList* animation_list = (AnimationList*)data;
		animation_list->~AnimationList();
	}
	break;
	case DECORATORSPTR:
	{
		DecoratorsPtr* decorators = (DecoratorsPtr*)data;
		decorators->~DecoratorsPtr();
	}
	break;
	case FILTERSPTR:
	{
		FiltersPtr* decorators = (FiltersPtr*)data;
		decorators->~FiltersPtr();
	}
	break;
	case FONTEFFECTSPTR:
	{
		FontEffectsPtr* font_effects = (FontEffectsPtr*)data;
		font_effects->~shared_ptr();
	}
	break;
	case COLORSTOPLIST:
	{
		ColorStopList* value = (ColorStopList*)data;
		value->~ColorStopList();
	}
	break;
	case BOXSHADOWLIST:
	{
		BoxShadowList* value = (BoxShadowList*)data;
		value->~BoxShadowList();
	}
	break;
	default: break;
	}
	type = NONE;
}

#define SET_VARIANT(type) *((type*)data) = value;

void Variant::Set(const Variant& copy)
{
	switch (copy.type)
	{
	case STRING: Set(*reinterpret_cast<const String*>(copy.data)); break;
	case TRANSFORMPTR: Set(*reinterpret_cast<const TransformPtr*>(copy.data)); break;
	case TRANSITIONLIST: Set(*reinterpret_cast<const TransitionList*>(copy.data)); break;
	case ANIMATIONLIST: Set(*reinterpret_cast<const AnimationList*>(copy.data)); break;
	case DECORATORSPTR: Set(*reinterpret_cast<const DecoratorsPtr*>(copy.data)); break;
	case FILTERSPTR: Set(*reinterpret_cast<const FiltersPtr*>(copy.data)); break;
	case FONTEFFECTSPTR: Set(*reinterpret_cast<const FontEffectsPtr*>(copy.data)); break;
	case COLORSTOPLIST: Set(*reinterpret_cast<const ColorStopList*>(copy.data)); break;
	case BOXSHADOWLIST: Set(*reinterpret_cast<const BoxShadowList*>(copy.data)); break;
	default:
		memcpy(data, copy.data, LOCAL_DATA_SIZE);
		type = copy.type;
		break;
	}
	RMLUI_ASSERT(type == copy.type);
}

void Variant::Set(Variant&& other)
{
	switch (other.type)
	{
	case STRING: Set(std::move(*reinterpret_cast<String*>(other.data))); break;
	case TRANSFORMPTR: Set(std::move(*reinterpret_cast<TransformPtr*>(other.data))); break;
	case TRANSITIONLIST: Set(std::move(*reinterpret_cast<TransitionList*>(other.data))); break;
	case ANIMATIONLIST: Set(std::move(*reinterpret_cast<AnimationList*>(other.data))); break;
	case DECORATORSPTR: Set(std::move(*reinterpret_cast<DecoratorsPtr*>(other.data))); break;
	case FILTERSPTR: Set(std::move(*reinterpret_cast<FiltersPtr*>(other.data))); break;
	case FONTEFFECTSPTR: Set(std::move(*reinterpret_cast<FontEffectsPtr*>(other.data))); break;
	case COLORSTOPLIST: Set(std::move(*reinterpret_cast<ColorStopList*>(other.data))); break;
	case BOXSHADOWLIST: Set(std::move(*reinterpret_cast<BoxShadowList*>(other.data))); break;
	default:
		memcpy(data, other.data, LOCAL_DATA_SIZE);
		type = other.type;
		break;
	}
	RMLUI_ASSERT(type == other.type);
}

void Variant::Set(const bool value)
{
	type = BOOL;
	SET_VARIANT(bool);
}

void Variant::Set(const byte value)
{
	type = BYTE;
	SET_VARIANT(byte);
}

void Variant::Set(const char value)
{
	type = CHAR;
	SET_VARIANT(char);
}

void Variant::Set(const float value)
{
	type = FLOAT;
	SET_VARIANT(float);
}

void Variant::Set(const double value)
{
	type = DOUBLE;
	SET_VARIANT(double);
}

void Variant::Set(const int value)
{
	type = INT;
	SET_VARIANT(int);
}

void Variant::Set(const int64_t value)
{
	type = INT64;
	SET_VARIANT(int64_t);
}

void Variant::Set(const unsigned int value)
{
	type = UINT;
	SET_VARIANT(unsigned int);
}

void Variant::Set(const uint64_t value)
{
	type = UINT64;
	SET_VARIANT(uint64_t);
}

void Variant::Set(const char* value)
{
	Set(String(value));
}

void Variant::Set(void* voidptr)
{
	type = VOIDPTR;
	memcpy(data, &voidptr, sizeof(void*));
}

void Variant::Set(const Vector2f value)
{
	type = VECTOR2;
	SET_VARIANT(Vector2f);
}

void Variant::Set(const Vector3f value)
{
	type = VECTOR3;
	SET_VARIANT(Vector3f);
}

void Variant::Set(const Vector4f value)
{
	type = VECTOR4;
	SET_VARIANT(Vector4f);
}

void Variant::Set(const Colourf value)
{
	type = COLOURF;
	SET_VARIANT(Colourf);
}

void Variant::Set(const Colourb value)
{
	type = COLOURB;
	SET_VARIANT(Colourb);
}

void Variant::Set(ScriptInterface* value)
{
	type = SCRIPTINTERFACE;
	memcpy(data, &value, sizeof(ScriptInterface*));
}

void Variant::Set(const String& value)
{
	if (type == STRING)
	{
		(*(String*)data) = value;
	}
	else
	{
		type = STRING;
		new (data) String(value);
	}
}
void Variant::Set(String&& value)
{
	if (type == STRING)
	{
		(*(String*)data) = std::move(value);
	}
	else
	{
		type = STRING;
		new (data) String(std::move(value));
	}
}

void Variant::Set(const TransformPtr& value)
{
	if (type == TRANSFORMPTR)
	{
		SET_VARIANT(TransformPtr);
	}
	else
	{
		type = TRANSFORMPTR;
		new (data) TransformPtr(value);
	}
}
void Variant::Set(TransformPtr&& value)
{
	if (type == TRANSFORMPTR)
	{
		(*(TransformPtr*)data) = std::move(value);
	}
	else
	{
		type = TRANSFORMPTR;
		new (data) TransformPtr(std::move(value));
	}
}

void Variant::Set(const TransitionList& value)
{
	if (type == TRANSITIONLIST)
	{
		*(TransitionList*)data = value;
	}
	else
	{
		type = TRANSITIONLIST;
		new (data) TransitionList(value);
	}
}
void Variant::Set(TransitionList&& value)
{
	if (type == TRANSITIONLIST)
	{
		(*(TransitionList*)data) = std::move(value);
	}
	else
	{
		type = TRANSITIONLIST;
		new (data) TransitionList(std::move(value));
	}
}

void Variant::Set(const AnimationList& value)
{
	if (type == ANIMATIONLIST)
	{
		*(AnimationList*)data = value;
	}
	else
	{
		type = ANIMATIONLIST;
		new (data) AnimationList(value);
	}
}
void Variant::Set(AnimationList&& value)
{
	if (type == ANIMATIONLIST)
	{
		(*(AnimationList*)data) = std::move(value);
	}
	else
	{
		type = ANIMATIONLIST;
		new (data) AnimationList(std::move(value));
	}
}

void Variant::Set(const DecoratorsPtr& value)
{
	if (type == DECORATORSPTR)
	{
		*(DecoratorsPtr*)data = value;
	}
	else
	{
		type = DECORATORSPTR;
		new (data) DecoratorsPtr(value);
	}
}
void Variant::Set(DecoratorsPtr&& value)
{
	if (type == DECORATORSPTR)
	{
		(*(DecoratorsPtr*)data) = std::move(value);
	}
	else
	{
		type = DECORATORSPTR;
		new (data) DecoratorsPtr(std::move(value));
	}
}
void Variant::Set(const FiltersPtr& value)
{
	if (type == FILTERSPTR)
	{
		*(FiltersPtr*)data = value;
	}
	else
	{
		type = FILTERSPTR;
		new (data) FiltersPtr(value);
	}
}
void Variant::Set(FiltersPtr&& value)
{
	if (type == FILTERSPTR)
	{
		(*(FiltersPtr*)data) = std::move(value);
	}
	else
	{
		type = FILTERSPTR;
		new (data) FiltersPtr(std::move(value));
	}
}
void Variant::Set(const FontEffectsPtr& value)
{
	if (type == FONTEFFECTSPTR)
	{
		*(FontEffectsPtr*)data = value;
	}
	else
	{
		type = FONTEFFECTSPTR;
		new (data) FontEffectsPtr(value);
	}
}
void Variant::Set(FontEffectsPtr&& value)
{
	if (type == FONTEFFECTSPTR)
	{
		(*(FontEffectsPtr*)data) = std::move(value);
	}
	else
	{
		type = FONTEFFECTSPTR;
		new (data) FontEffectsPtr(std::move(value));
	}
}
void Variant::Set(const ColorStopList& value)
{
	if (type == COLORSTOPLIST)
	{
		*(ColorStopList*)data = value;
	}
	else
	{
		type = COLORSTOPLIST;
		new (data) ColorStopList(value);
	}
}
void Variant::Set(ColorStopList&& value)
{
	if (type == COLORSTOPLIST)
	{
		(*(ColorStopList*)data) = std::move(value);
	}
	else
	{
		type = COLORSTOPLIST;
		new (data) ColorStopList(std::move(value));
	}
}
void Variant::Set(const BoxShadowList& value)
{
	if (type == BOXSHADOWLIST)
	{
		*(BoxShadowList*)data = value;
	}
	else
	{
		type = BOXSHADOWLIST;
		new (data) BoxShadowList(value);
	}
}
void Variant::Set(BoxShadowList&& value)
{
	if (type == BOXSHADOWLIST)
	{
		(*(BoxShadowList*)data) = std::move(value);
	}
	else
	{
		type = BOXSHADOWLIST;
		new (data) BoxShadowList(std::move(value));
	}
}

Variant& Variant::operator=(const Variant& copy)
{
	if (this == &copy)
		return *this;
	if (copy.type != type)
		Clear();
	Set(copy);
	return *this;
}

Variant& Variant::operator=(Variant&& other) noexcept
{
	if (this == &other)
		return *this;
	if (other.type != type)
		Clear();
	Set(std::move(other));
	return *this;
}

#define DEFAULT_VARIANT_COMPARE(TYPE) static_cast<TYPE>(*(TYPE*)data) == static_cast<TYPE>(*(TYPE*)other.data)

bool Variant::operator==(const Variant& other) const
{
	if (type != other.type)
		return false;

	switch (type)
	{
	case BOOL: return DEFAULT_VARIANT_COMPARE(bool);
	case BYTE: return DEFAULT_VARIANT_COMPARE(byte);
	case CHAR: return DEFAULT_VARIANT_COMPARE(char);
	case FLOAT: return DEFAULT_VARIANT_COMPARE(float);
	case DOUBLE: return DEFAULT_VARIANT_COMPARE(double);
	case INT: return DEFAULT_VARIANT_COMPARE(int);
	case INT64: return DEFAULT_VARIANT_COMPARE(int64_t);
	case UINT: return DEFAULT_VARIANT_COMPARE(unsigned int);
	case UINT64: return DEFAULT_VARIANT_COMPARE(uint64_t);
	case STRING: return DEFAULT_VARIANT_COMPARE(String);
	case VECTOR2: return DEFAULT_VARIANT_COMPARE(Vector2f);
	case VECTOR3: return DEFAULT_VARIANT_COMPARE(Vector3f);
	case VECTOR4: return DEFAULT_VARIANT_COMPARE(Vector4f);
	case COLOURF: return DEFAULT_VARIANT_COMPARE(Colourf);
	case COLOURB: return DEFAULT_VARIANT_COMPARE(Colourb);
	case SCRIPTINTERFACE: return DEFAULT_VARIANT_COMPARE(ScriptInterface*);
	case VOIDPTR: return DEFAULT_VARIANT_COMPARE(void*);
	case TRANSFORMPTR: return DEFAULT_VARIANT_COMPARE(TransformPtr);
	case TRANSITIONLIST: return DEFAULT_VARIANT_COMPARE(TransitionList);
	case ANIMATIONLIST: return DEFAULT_VARIANT_COMPARE(AnimationList);
	case DECORATORSPTR: return DEFAULT_VARIANT_COMPARE(DecoratorsPtr);
	case FILTERSPTR: return DEFAULT_VARIANT_COMPARE(FiltersPtr);
	case FONTEFFECTSPTR: return DEFAULT_VARIANT_COMPARE(FontEffectsPtr);
	case COLORSTOPLIST: return DEFAULT_VARIANT_COMPARE(ColorStopList);
	case BOXSHADOWLIST: return DEFAULT_VARIANT_COMPARE(BoxShadowList);
	case NONE: return true;
	}
	RMLUI_ERRORMSG("Variant comparison not implemented for this type.");
	return false;
}

} // namespace Rml
