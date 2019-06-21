/*
 * This source file is part of libRocket, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://www.librocket.com
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
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

#include "precompiled.h"
#include "../../Include/Rocket/Core/TypeConverter.h"
#include "../../Include/Rocket/Core/Animation.h"
#include "../../Include/Rocket/Core/Transform.h"

namespace Rocket {
namespace Core {

bool TypeConverter<TransformRef, TransformRef>::Convert(const TransformRef& src, TransformRef& dest)
{
	dest = src;
	return true;
}

bool TypeConverter<TransformRef, String>::Convert(const TransformRef& src, String& dest)
{
	if (src)
	{
		dest.clear();
		const Transform::Primitives& primitives = src->GetPrimitives();
		for (size_t i = 0; i < primitives.size(); i++)
		{
			dest += primitives[i].ToString();
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
		// TODO: Add property name from id
		const Transition& t = src.transitions[i];
		//dest += t.id + " ";
		dest += t.tween.to_string() + " ";
		if (TypeConverter< float, String >::Convert(t.duration, tmp)) dest += tmp + "s ";
		if (t.delay > 0.0f && TypeConverter< float, String >::Convert(t.delay, tmp)) dest += tmp + "s ";
		if (t.reverse_adjustment_factor > 0.0f && TypeConverter< float, String >::Convert(t.delay, tmp)) dest += tmp;
		if (dest.size() > 0) dest.resize(dest.size() - 1);
		if (i != src.transitions.size() - 1) dest += ", ";
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
		if (TypeConverter< float, String >::Convert(a.duration, tmp)) dest += tmp + "s ";
		dest += a.tween.to_string() + " ";
		if (a.delay > 0.0f && TypeConverter< float, String >::Convert(a.delay, tmp)) dest += tmp + "s ";
		if (a.alternate) dest += "alternate ";
		if (a.paused) dest += "paused ";
		if (a.num_iterations == -1) dest += "infinite ";
		else if (TypeConverter< int, String >::Convert(a.num_iterations, tmp)) dest += tmp + " ";
		dest += a.name;
		if (i != src.size() - 1) dest += ", ";
	}
	return true;
}

bool TypeConverter<DecoratorList, DecoratorList>::Convert(const DecoratorList& src, DecoratorList& dest)
{
	dest = src;
	return true;
}


bool TypeConverter<DecoratorList, String>::Convert(const DecoratorList& src, String& dest)
{
	// Todo. For now, we just count the number of decorators
	if (!TypeConverter<int, String>::Convert((int)src.size(), dest))
		return false;
	dest += " decorator(s)";
	return true;
}



}
}
