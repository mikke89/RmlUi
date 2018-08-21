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
	Property property;
	Tween tween;  // Tweening between the previous and this key. Ignored for the first animation key.
};


class ElementAnimation
{
private:
	String property_name;

	float duration;           // for a single iteration
	int num_iterations;       // -1 for infinity
	bool alternate_direction; // between iterations

	std::vector<AnimationKey> keys;

	float last_update_world_time;
	float time_since_iteration_start;
	int current_iteration;
	bool reverse_direction;

	bool animation_complete;

public:

	ElementAnimation(const String& property_name, const Property& current_value, float start_world_time, float duration, int num_iterations, bool alternate_direction);

	bool AddKey(float time, const Property& property, Element& element, Tween tween);

	Property UpdateAndGetProperty(float time, Element& element);

	const String& GetPropertyName() const { return property_name; }
	float GetDuration() const { return duration; }
	void SetDuration(float duration) { this->duration = duration; }
	bool IsComplete() const { return animation_complete; }
};





}
}

#endif