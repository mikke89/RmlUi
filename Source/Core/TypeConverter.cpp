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

#include "../../Include/RmlUi/Core/TypeConverter.h"
#include "../../Include/RmlUi/Core/Animation.h"
#include "../../Include/RmlUi/Core/DecoratorInstancer.h"
#include "../../Include/RmlUi/Core/PropertyDictionary.h"
#include "../../Include/RmlUi/Core/PropertySpecification.h"
#include "../../Include/RmlUi/Core/StyleSheetSpecification.h"
#include "../../Include/RmlUi/Core/StyleSheetTypes.h"
#include "../../Include/RmlUi/Core/Transform.h"
#include "../../Include/RmlUi/Core/TransformPrimitive.h"
#include "TransformUtilities.h"

namespace Rml {

bool TypeConverter<Unit, String>::Convert(const Unit& src, String& dest)
{
	switch (src)
	{
	// clang-format off
	case Unit::NUMBER:  dest = "";    return true;
	case Unit::PERCENT: dest = "%";   return true;

	case Unit::PX:      dest = "px";  return true;
	case Unit::DP:      dest = "dp";  return true;
	case Unit::VW:      dest = "vw";  return true;
	case Unit::VH:      dest = "vh";  return true;
	case Unit::X:       dest = "x";   return true;
	case Unit::EM:      dest = "em";  return true;
	case Unit::REM:     dest = "rem"; return true;

	case Unit::INCH:    dest = "in";  return true;
	case Unit::CM:      dest = "cm";  return true;
	case Unit::MM:      dest = "mm";  return true;
	case Unit::PT:      dest = "pt";  return true;
	case Unit::PC:      dest = "pc";  return true;

	case Unit::DEG:     dest = "deg"; return true;
	case Unit::RAD:     dest = "rad"; return true;
	// clang-format on
	default: break;
	}

	return false;
}

bool TypeConverter<TransformPtr, TransformPtr>::Convert(const TransformPtr& src, TransformPtr& dest)
{
	dest = src;
	return true;
}

bool TypeConverter<TransformPtr, String>::Convert(const TransformPtr& src, String& dest)
{
	if (src)
	{
		dest.clear();
		const Transform::PrimitiveList& primitives = src->GetPrimitives();
		for (size_t i = 0; i < primitives.size(); i++)
		{
			dest += TransformUtilities::ToString(primitives[i]);
			if (i != primitives.size() - 1)
				dest += ' ';
		}
	}
	else
	{
		dest = "none";
	}
	return true;
}

bool TypeConverter<TransitionList, TransitionList>::Convert(const TransitionList& src, TransitionList& dest)
{
	dest = src;
	return true;
}

bool TypeConverter<TransitionList, String>::Convert(const TransitionList& src, String& dest)
{
	if (src.none)
	{
		dest = "none";
		return true;
	}
	String tmp;
	for (size_t i = 0; i < src.transitions.size(); i++)
	{
		const Transition& t = src.transitions[i];
		dest += StyleSheetSpecification::GetPropertyName(t.id) + ' ';
		dest += t.tween.to_string() + ' ';
		if (TypeConverter<float, String>::Convert(t.duration, tmp))
			dest += tmp + "s ";
		if (t.delay > 0.0f && TypeConverter<float, String>::Convert(t.delay, tmp))
			dest += tmp + "s ";
		if (t.reverse_adjustment_factor > 0.0f && TypeConverter<float, String>::Convert(t.reverse_adjustment_factor, tmp))
			dest += tmp + ' ';
		if (dest.size() > 0)
			dest.resize(dest.size() - 1);
		if (i != src.transitions.size() - 1)
			dest += ", ";
	}
	return true;
}

bool TypeConverter<AnimationList, AnimationList>::Convert(const AnimationList& src, AnimationList& dest)
{
	dest = src;
	return true;
}

bool TypeConverter<AnimationList, String>::Convert(const AnimationList& src, String& dest)
{
	String tmp;
	for (size_t i = 0; i < src.size(); i++)
	{
		const Animation& a = src[i];
		if (TypeConverter<float, String>::Convert(a.duration, tmp))
			dest += tmp + "s ";
		dest += a.tween.to_string() + " ";
		if (a.delay > 0.0f && TypeConverter<float, String>::Convert(a.delay, tmp))
			dest += tmp + "s ";
		if (a.alternate)
			dest += "alternate ";
		if (a.paused)
			dest += "paused ";
		if (a.num_iterations == -1)
			dest += "infinite ";
		else if (TypeConverter<int, String>::Convert(a.num_iterations, tmp))
			dest += tmp + " ";
		dest += a.name;
		if (i != src.size() - 1)
			dest += ", ";
	}
	return true;
}

bool TypeConverter<DecoratorsPtr, DecoratorsPtr>::Convert(const DecoratorsPtr& src, DecoratorsPtr& dest)
{
	dest = src;
	return true;
}

bool TypeConverter<DecoratorsPtr, String>::Convert(const DecoratorsPtr& src, String& dest)
{
	if (!src || src->list.empty())
		dest = "none";
	else if (!src->value.empty())
		dest += src->value;
	else
	{
		dest.clear();
		for (const DecoratorDeclaration& declaration : src->list)
		{
			dest += declaration.type;
			if (auto instancer = declaration.instancer)
			{
				dest += '(' + instancer->GetPropertySpecification().PropertiesToString(declaration.properties, false, ' ') + ')';
			}
			dest += ", ";
		}
		if (dest.size() > 2)
			dest.resize(dest.size() - 2);
	}
	return true;
}

bool TypeConverter<FontEffectsPtr, FontEffectsPtr>::Convert(const FontEffectsPtr& src, FontEffectsPtr& dest)
{
	dest = src;
	return true;
}

bool TypeConverter<FontEffectsPtr, String>::Convert(const FontEffectsPtr& src, String& dest)
{
	if (!src || src->list.empty())
		dest = "none";
	else
		dest += src->value;
	return true;
}

} // namespace Rml
