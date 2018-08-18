/*
 * This source file is part of libRocket, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://www.librocket.com
 *
 * Copyright (c) 2018 Michael Ragazzon
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
 
#ifndef ROCKETCORETWEEN_H
#define ROCKETCORETWEEN_H

#include <functional>

namespace Rocket {
namespace Core {


class Tween {
public:
	enum Type { None, Back, Bounce, Circular, Cubic, Elastic, Exponential, Linear, Quadratic, Quartic, Quintic, Sine, Callback };
	enum Direction { In, Out, InOut };

	Tween(Type type = Linear, Direction direction = In) : type(type), direction(direction) {}
	Tween(std::function<float(float)> callback, Direction direction = In) : type(Callback), direction(direction), callback(callback) {}

	float operator()(float t) const
	{
		if (type == None)
			return t;

		switch (direction)
		{
		case In:
			return in(t);
		case Out:
			return out(t);
		case InOut:
			return in_out(t);
		}
		return t;
	}

	// Tweening functions below.
	// Partly based on http://libclaw.sourceforge.net/tweeners.html

	static float back(float t)
	{
		return t * t*(2.70158f*t - 1.70158f);
	}
	static float bounce(float t)
	{
		if (t > 1.f - 1.f / 2.75f)
			return 1.f - 7.5625f*square(1.f - t);
		else if (t > 1.f - 2.f / 2.75f)
			return 1.0f - (7.5625f*square(1.f - t - 1.5f / 2.75f) + 0.75f);
		else if (t > 1.f - 2.5f / 2.75f)
			return 1.0f - (7.5625f*square(1.f - t - 2.25f / 2.75f) + 0.9375f);
		return 1.0f - (7.5625f*square(1.f - t - 2.625f / 2.75f) + 0.984375f);
	}
	static float circular(float t)
	{
		return 1.f - sqrt(1.f - t * t);
	}
	static float cubic(float t)
	{
		return t * t*t;
	}
	static float elastic(float t)
	{
		if (t == 0) return t;
		if (t == 1) return t;
		return -exp(7.24f*(t-1.f))*sin((t - 1.1f)*2.f*Math::ROCKET_PI / 0.4f);
	}
	static float exponential(float t)
	{
		if (t == 0) return t;
		if (t == 1) return t;
		return exp(7.24f*(t - 1.f));
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
		return t*t*t*t;
	}
	static float quintic(float t)
	{
		return t*t*t*t*t;
	}
	static float sine(float t)
	{
		return 1.f - cos(t*Math::ROCKET_PI*0.5f);
	}


private:
	float in(float t) const
	{
		switch (type)
		{
		case Back:
			return back(t);
		case Bounce:
			return bounce(t);
		case Circular:
			return circular(t);
		case Cubic:
			return cubic(t);
		case Elastic:
			return elastic(t);
		case Exponential:
			return exponential(t);
		case Linear:
			return linear(t);
		case Quadratic:
			return quadratic(t);
		case Quartic:
			return quartic(t);
		case Quintic:
			return quintic(t);
		case Sine:
			return sine(t);
		case Callback:
			if(callback(t))
				return callback(t);
			break;
		}
		return t;
	}
	float out(float t) const
	{
		return 1.0f - in(1.0f - t);
	}
	float in_out(float t) const
	{
		if (t < 0.5f)
			return in(2.0f*t)*0.5f;
		else
			return 0.5f + out(2.0f*t - 1.0f)*0.5f;
	}

	inline static float square(float t) { return t * t; }

	Type type;
	Direction direction;
	std::function<float(float)> callback;
};


}
}

#endif
