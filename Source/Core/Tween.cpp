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

#include "../../Include/RmlUi/Core/Tween.h"
#include "../../Include/RmlUi/Core/Math.h"
#include <utility>

namespace Rml {

namespace TweenFunctions {

	/**
	    Tweening functions below.
	    Partly based on http://libclaw.sourceforge.net/tweeners.html
	 */

	static inline float square(float t)
	{
		return t * t;
	}

	static float back(float t)
	{
		return t * t * (2.70158f * t - 1.70158f);
	}

	static float bounce(float t)
	{
		if (t > 1.f - 1.f / 2.75f)
			return 1.f - 7.5625f * square(1.f - t);
		else if (t > 1.f - 2.f / 2.75f)
			return 1.0f - (7.5625f * square(1.f - t - 1.5f / 2.75f) + 0.75f);
		else if (t > 1.f - 2.5f / 2.75f)
			return 1.0f - (7.5625f * square(1.f - t - 2.25f / 2.75f) + 0.9375f);
		return 1.0f - (7.5625f * square(1.f - t - 2.625f / 2.75f) + 0.984375f);
	}

	static float circular(float t)
	{
		return 1.f - Math::SquareRoot(1.f - t * t);
	}

	static float cubic(float t)
	{
		return t * t * t;
	}

	static float elastic(float t)
	{
		if (t == 0)
			return t;
		if (t == 1)
			return t;
		return -Math::Exp(7.24f * (t - 1.f)) * Math::Sin((t - 1.1f) * 2.f * Math::RMLUI_PI / 0.4f);
	}

	static float exponential(float t)
	{
		if (t == 0)
			return t;
		if (t == 1)
			return t;
		return Math::Exp(7.24f * (t - 1.f));
	}

	static float linear(float t)
	{
		return t;
	}

	static float quadratic(float t)
	{
		return t * t;
	}

	static float quartic(float t)
	{
		return t * t * t * t;
	}

	static float quintic(float t)
	{
		return t * t * t * t * t;
	}

	static float sine(float t)
	{
		return 1.f - Math::Cos(t * Math::RMLUI_PI * 0.5f);
	}

} // namespace TweenFunctions

Tween::Tween(Type type, Direction direction)
{
	if (direction & In)
		type_in = type;
	if (direction & Out)
		type_out = type;
}
Tween::Tween(Type type_in, Type type_out) : type_in(type_in), type_out(type_out) {}
Tween::Tween(CallbackFnc callback, Direction direction) : callback(callback)
{
	if (direction & In)
		type_in = Callback;
	if (direction & Out)
		type_out = Callback;
}
float Tween::operator()(float t) const
{
	if (type_in != None && type_out == None)
	{
		return in(t);
	}
	if (type_in == None && type_out != None)
	{
		return out(t);
	}
	if (type_in != None && type_out != None)
	{
		return in_out(t);
	}
	return t;
}

void Tween::reverse()
{
	std::swap(type_in, type_out);
}

bool Tween::operator==(const Tween& other) const
{
	return type_in == other.type_in && type_out == other.type_out && callback == other.callback;
}

bool Tween::operator!=(const Tween& other) const
{
	return !(*this == other);
}

String Tween::to_string() const
{
	static const Array<String, size_t(Count)> type_str = {
		{"none", "back", "bounce", "circular", "cubic", "elastic", "exponential", "linear", "quadratic", "quartic", "quintic", "sine", "callback"}};

	if (size_t(type_in) < type_str.size() && size_t(type_out) < type_str.size())
	{
		if (type_in == None && type_out == None)
		{
			return "none";
		}
		else if (type_in == type_out)
		{
			return type_str[size_t(type_in)] + String("-in-out");
		}
		else if (type_in == None)
		{
			return type_str[size_t(type_out)] + String("-out");
		}
		else if (type_out == None)
		{
			return type_str[size_t(type_in)] + String("-in");
		}
		else if (type_in != type_out)
		{
			return type_str[size_t(type_in)] + String("-in-") + type_str[size_t(type_out)] + String("-out");
		}
	}
	return "unknown";
}

float Tween::tween(Type type, float t) const
{
	using namespace TweenFunctions;

	switch (type)
	{
	case Back: return back(t);
	case Bounce: return bounce(t);
	case Circular: return circular(t);
	case Cubic: return cubic(t);
	case Elastic: return elastic(t);
	case Exponential: return exponential(t);
	case Linear: return linear(t);
	case Quadratic: return quadratic(t);
	case Quartic: return quartic(t);
	case Quintic: return quintic(t);
	case Sine: return sine(t);
	case Callback:
		if (callback)
			return (*callback)(t);
		break;
	default: break;
	}
	return t;
}

float Tween::in(float t) const
{
	return tween(type_in, t);
}

float Tween::out(float t) const
{
	return 1.0f - tween(type_out, 1.0f - t);
}

float Tween::in_out(float t) const
{
	if (t < 0.5f)
		return tween(type_in, 2.0f * t) * 0.5f;
	else
		return 0.5f + out(2.0f * t - 1.0f) * 0.5f;
}

} // namespace Rml
