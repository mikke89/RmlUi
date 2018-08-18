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

#ifndef ROCKETCOREELEMENTANIMATION_H
#define ROCKETCOREELEMENTANIMATION_H

#include "../../Include/Rocket/Core/Header.h"
#include "../../Include/Rocket/Core/Property.h"
#include "../../Include/Rocket/Core/Tween.h"

namespace Rocket {
namespace Core {


struct AnimationKey {
	float time;
	Variant value;
};


class ElementAnimation
{
private:
	String property_name;
	Property::Unit property_unit;
	int property_specificity;

	float duration;           // for a single iteration
	int num_iterations;       // -1 for infinity
	bool alternate_direction; // between iterations
	Tween tween;              // tweening for a single iteration

	std::vector<AnimationKey> keys;

	float last_update_time;
	float time_since_iteration_start;
	int current_iteration;
	bool reverse_direction;  // if true, run time backwards

	bool animation_complete;

public:

	ElementAnimation(const String& property_name, const Property& current_value, float time, float duration, int num_iterations, bool alternate_direction, Tween tween) 
		: property_name(property_name), property_unit(current_value.unit), property_specificity(current_value.specificity), tween(tween),
		duration(duration), num_iterations(num_iterations), alternate_direction(alternate_direction), 
		keys({ AnimationKey{0.0f, current_value.value} }),
		last_update_time(time), time_since_iteration_start(0.0f), current_iteration(0), reverse_direction(false), animation_complete(false) 
	{}

	bool AddKey(float time, const Property& property, Element& element);

	Property UpdateAndGetProperty(float time);

	const String& GetPropertyName() const { return property_name; }
	float GetDuration() const { return duration; }
	void SetDuration(float duration) { this->duration = duration; }
	bool IsComplete() const { return animation_complete; }
};





}
}

#endif