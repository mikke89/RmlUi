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
#include "../../Include/RmlUi/Core/DecorationTypes.h"
#include "../../Include/RmlUi/Core/Decorator.h"
#include "../../Include/RmlUi/Core/Filter.h"
#include "../../Include/RmlUi/Core/PropertyDictionary.h"
#include "../../Include/RmlUi/Core/PropertySpecification.h"
#include "../../Include/RmlUi/Core/StyleSheetSpecification.h"
#include "../../Include/RmlUi/Core/StyleSheetTypes.h"
#include "../../Include/RmlUi/Core/Transform.h"
#include "../../Include/RmlUi/Core/TransformPrimitive.h"
#include "PropertyParserColour.h"
#include "PropertyParserDecorator.h"
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

template <typename EffectDeclaration>
void AppendPaintArea(const EffectDeclaration& /*declaration*/, String& /*dest*/)
{}
template <>
void AppendPaintArea(const DecoratorDeclaration& declaration, String& dest)
{
	if (declaration.paint_area >= BoxArea::Border && declaration.paint_area <= BoxArea::Padding)
		dest += " " + PropertyParserDecorator::ConvertAreaToString(declaration.paint_area);
}

template <typename EffectsPtr>
static bool ConvertEffectToString(const EffectsPtr& src, String& dest, const String& separator)
{
	if (!src || src->list.empty())
		dest = "none";
	else if (!src->value.empty())
		dest += src->value;
	else
	{
		dest.clear();
		for (const auto& declaration : src->list)
		{
			dest += declaration.type;
			if (auto* instancer = declaration.instancer)
				dest += '(' + instancer->GetPropertySpecification().PropertiesToString(declaration.properties, false, ' ') + ')';

			AppendPaintArea(declaration, dest);
			if (&declaration != &src->list.back())
				dest += separator;
		}
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
	return ConvertEffectToString(src, dest, ", ");
}

bool TypeConverter<FiltersPtr, FiltersPtr>::Convert(const FiltersPtr& src, FiltersPtr& dest)
{
	dest = src;
	return true;
}

bool TypeConverter<FiltersPtr, String>::Convert(const FiltersPtr& src, String& dest)
{
	return ConvertEffectToString(src, dest, " ");
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

bool TypeConverter<ColorStopList, ColorStopList>::Convert(const ColorStopList& src, ColorStopList& dest)
{
	dest = src;
	return true;
}

bool TypeConverter<ColorStopList, String>::Convert(const ColorStopList& src, String& dest)
{
	dest.clear();
	for (size_t i = 0; i < src.size(); i++)
	{
		const ColorStop& stop = src[i];
		dest += ToString(stop.color.ToNonPremultiplied());

		if (Any(stop.position.unit & Unit::NUMBER_LENGTH_PERCENT))
			dest += " " + ToString(stop.position.number) + ToString(stop.position.unit);

		if (i < src.size() - 1)
			dest += ", ";
	}
	return true;
}

bool TypeConverter<BoxShadowList, BoxShadowList>::Convert(const BoxShadowList& src, BoxShadowList& dest)
{
	dest = src;
	return true;
}

bool TypeConverter<BoxShadowList, String>::Convert(const BoxShadowList& src, String& dest)
{
	dest.clear();
	String temp, str_unit;
	for (size_t i = 0; i < src.size(); i++)
	{
		const BoxShadow& shadow = src[i];
		for (const NumericValue* value : {&shadow.offset_x, &shadow.offset_y, &shadow.blur_radius, &shadow.spread_distance})
		{
			if (TypeConverter<Unit, String>::Convert(value->unit, str_unit))
				temp += " " + ToString(value->number) + str_unit;
		}

		if (shadow.inset)
			temp += " inset";

		dest += ToString(shadow.color.ToNonPremultiplied()) + temp;

		if (i < src.size() - 1)
		{
			dest += ", ";
			temp.clear();
		}
	}
	return true;
}

bool TypeConverter<Colourb, String>::Convert(const Colourb& src, String& dest)
{
	if (src.alpha == 255)
		return FormatString(dest, "#%02hhx%02hhx%02hhx", src.red, src.green, src.blue) > 0;
	else
		return FormatString(dest, "#%02hhx%02hhx%02hhx%02hhx", src.red, src.green, src.blue, src.alpha) > 0;
}

bool TypeConverter<String, Colourb>::Convert(const String& src, Colourb& dest)
{
	return PropertyParserColour::ParseColour(dest, src);
}

} // namespace Rml
