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

#include "precompiled.h"
#include "ElementAnimation.h"
#include "../../Include/Rocket/Core/Element.h"

namespace Rocket {
namespace Core {


static Colourf ColourToLinearSpace(Colourb c)
{
	Colourf result;
	// Approximate inverse sRGB function
	result.red = Math::SquareRoot((float)c.red / 255.f);
	result.green = Math::SquareRoot((float)c.green / 255.f);
	result.blue = Math::SquareRoot((float)c.blue / 255.f);
	result.alpha = (float)c.alpha / 255.f;
	return result;
}

static Colourb ColourFromLinearSpace(Colourf c)
{
	Colourb result;
	result.red = (Rocket::Core::byte)Math::Clamp(c.red*c.red*255.f, 0.0f, 255.f);
	result.green = (Rocket::Core::byte)Math::Clamp(c.green*c.green*255.f, 0.0f, 255.f);
	result.blue = (Rocket::Core::byte)Math::Clamp(c.blue*c.blue*255.f, 0.0f, 255.f);
	result.alpha = (Rocket::Core::byte)Math::Clamp(c.alpha*255.f, 0.0f, 255.f);
	return result;
}

static Variant InterpolateValues(const Variant & from, const Variant & to, float alpha)
{
	auto type = from.GetType();
	auto type_to = to.GetType();
	if (type != type_to)
	{
		Log::Message(Log::LT_WARNING, "Interpolating properties must be of same unit. Got types: '%c' and '%c'.", type, type_to);
		return from;
	}

	switch (type)
	{
	case Variant::FLOAT:
	{
		float f0 = from.Get<float>();
		float f1 = to.Get<float>();
		float f = (1.0f - alpha) * f0 + alpha * f1;
		return Variant(f);
	}
	case Variant::COLOURB:
	{
		Colourf c0 = ColourToLinearSpace(from.Get<Colourb>());
		Colourf c1 = ColourToLinearSpace(to.Get<Colourb>());
		Colourf c = c0 * (1.0f - alpha) + c1 * alpha;
		return Variant(ColourFromLinearSpace(c));
	}
	}

	Log::Message(Log::LT_WARNING, "Currently, only float and color values can be interpolated. Got types of: '%c'.", type);

	return from;
}



bool ElementAnimation::AddKey(float time, const Property & property)
{
	if (property.unit != property_unit)
		return false;

	keys.push_back({ time, property.value });

	return true;
}

Property ElementAnimation::UpdateAndGetProperty(float time)
{
	Property result;


	//Log::Message(Log::LT_INFO, "Animation it = %d,  t_it = %f, rev = %d,  dt = %f", current_iteration, time_since_iteration_start, (int)reverse_direction, time - last_update_time);

	if (animation_complete || time - last_update_time <= 0.0f)
		return result;

	const float dt = time - last_update_time;

	last_update_time = time;
	time_since_iteration_start += dt;

	if (time_since_iteration_start >= duration)
	{
		// Next iteration
		current_iteration += 1;

		if (current_iteration < num_iterations || num_iterations == -1)
		{
			time_since_iteration_start = 0.0f;

			if (alternate_direction)
				reverse_direction = !reverse_direction;
		}
		else
		{
			animation_complete = true;
			time_since_iteration_start = duration;
		}
	}

	float t = time_since_iteration_start;

	if (reverse_direction)
		t = duration - t;

	int key0 = -1;
	int key1 = -1;

	{
		for (int i = 0; i < (int)keys.size(); i++)
		{
			if (keys[i].time >= t)
			{
				key1 = i;
				break;
			}
		}

		if (key1 < 0) key1 = (int)keys.size() - 1;
		key0 = (key1 == 0 ? 0 : key1 - 1 );
	}

	ROCKET_ASSERT(key0 >= 0 && key0 < (int)keys.size() && key1 >= 0 && key1 < (int)keys.size());

	float alpha = 0.0f;

	{
		const float t0 = keys[key0].time;
		const float t1 = keys[key1].time;

		const float eps = 1e-3f;

		if (t1 - t0 > eps)
			alpha = (t - t0) / (t1 - t0);
		

		alpha = Math::Clamp(alpha, 0.0f, 1.0f);
	}

	result.unit = property_unit;
	result.specificity = property_specificity;
	result.value = InterpolateValues(keys[key0].value, keys[key1].value, alpha);
	
	return result;
}


}
}