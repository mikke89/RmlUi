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
#include "../../Include/Rocket/Core/TransformPrimitive.h"

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


static Variant InterpolateValues(const Variant & v0, const Variant & v1, float alpha)
{
	auto type0 = v0.GetType();
	auto type1 = v1.GetType();
	if (type0 != type1)
	{
		Log::Message(Log::LT_WARNING, "Interpolating properties must be of same unit. Got types: '%c' and '%c'.", type0, type1);
		return v0;
	}

	switch (type0)
	{
	case Variant::FLOAT:
	{
		float f0 = v0.Get<float>();
		float f1 = v1.Get<float>();
		float f = (1.0f - alpha) * f0 + alpha * f1;
		return Variant(f);
	}
	case Variant::COLOURB:
	{
		Colourf c0 = ColourToLinearSpace(v0.Get<Colourb>());
		Colourf c1 = ColourToLinearSpace(v1.Get<Colourb>());
		Colourf c = c0 * (1.0f - alpha) + c1 * alpha;
		return Variant(ColourFromLinearSpace(c));
	}
	case Variant::TRANSFORMREF:
	{
		using namespace Rocket::Core::Transforms;

		// Build the new, interpolating transform
		auto t = TransformRef{ new Transform };

		auto t0 = v0.Get<TransformRef>();
		auto t1 = v1.Get<TransformRef>();

		const auto& p0 = t0->GetPrimitives();
		const auto& p1 = t1->GetPrimitives();

		if (p0.size() != p1.size())
		{
			Log::Message(Log::LT_WARNING, "Transform primitives not of same size during interpolation.");
			return Variant{ t0 };
		}

		for (size_t i = 0; i < p0.size(); i++)
		{
			Primitive p = p0[i];
			if (!p.InterpolateWith(p1[i], alpha))
			{
				Log::Message(Log::LT_WARNING, "Transform primitives not of same type during interpolation.");
				return Variant{ t0 };
			}
			t->AddPrimitive(p);
		}

		return Variant(t);

		Log::Message(Log::LT_WARNING, "Could not decode transform for interpolation.");
	}
	}

	Log::Message(Log::LT_WARNING, "Currently, only float and color values can be interpolated. Got types of: '%c'.", type0);

	return v0;
}



enum class PrepareTransformResult { Unchanged = 0, ChangedT0 = 1, ChangedT1 = 2, ChangedT0andT1 = 3, Invalid = 4 };

static PrepareTransformResult PrepareTransformPair(Transform& t0, Transform& t1, Element& element)
{
	using namespace Transforms;

	// Insert missing primitives into transform
	// See e.g. https://drafts.csswg.org/css-transforms-1/#interpolation-of-transforms for inspiration


	auto& prims0 = t0.GetPrimitives();
	auto& prims1 = t1.GetPrimitives();

	// Check for trivial case where they contain the same primitives
	if (prims0.size() == prims1.size())
	{
		bool same_primitives = true;
		for (size_t i = 0; i < prims0.size(); i++)
		{
			if (prims0[i].primitive.index() != prims1[i].primitive.index())
			{
				same_primitives = false;
				break;
			}
		}
		if (same_primitives)
			return PrepareTransformResult::Unchanged;
	}

	if (prims0.size() != prims1.size())
	{
		// Try to match the smallest set of primitives to the larger set, set missing keys in the small set to identity.
		// Requirement: The small set must match types in the same order they appear in the big set.
		// Example: (letter indicates type, number represent values)
		// big:       a0 b0 c0 b1
		//               ^     ^ 
		// small:     b2 b3   
		//            ^  ^
		// new small: a1 b2 c1 b3   
		bool prims0_smallest = (prims0.size() < prims1.size());

		auto& small = (prims0_smallest ? prims0 : prims1);
		auto& big = (prims0_smallest ? prims1 : prims0);

		std::vector<size_t> matching_indices; // Indices into 'big' for matching types
		matching_indices.reserve(small.size() + 1);

		size_t big_index = 0;
		bool match_success = true;

		// Iterate through the small set to see if its types fit into the big set
		for (size_t i = 0; i < small.size(); i++)
		{
			auto small_type = small[i].primitive.index();
			match_success = false;

			for (; big_index < big.size(); big_index++)
			{
				auto big_type = big[big_index].primitive.index();

				if (small_type == big_type)
				{
					matching_indices.push_back(big_index);
					match_success = true;
					big_index += 1;
					break;
				}
			}

			if (!match_success)
				break;
		}


		if (match_success)
		{
			// Success, insert the missing primitives into the small set
			matching_indices.push_back(big.size()); // Needed to copy elements behind the last matching primitive
			small.reserve(big.size());
			size_t i0 = 0;
			for (size_t match_index : matching_indices)
			{
				for (size_t i = i0; i < match_index; i++)
				{
					Primitive p = big[i];
					p.SetIdentity();
					small.insert(small.begin() + i, p);
				}

				// Next value to copy is one-past the matching primitive
				i0 = match_index + 1;
			}

			return (prims0_smallest ? PrepareTransformResult::ChangedT0 : PrepareTransformResult::ChangedT1);
		}
	}


	// If we get here, things get tricky. Need to do full matrix interpolation.
	// We resolve the full transform here. This is not entirely correct if the elements box size changes
	// during the animation. Ideally, we would resolve it during each iteration.
	// For performance: We could also consider breaking up the transforms into their interpolating primitives (translate, rotate, skew, scale) here,
	// instead of doing this every animation tick.
	for(Transform* t : {&t0, &t1})
	{
		Matrix4f transform_value = Matrix4f::Identity();
		for (const auto& primitive : t->GetPrimitives())
		{
			Matrix4f m;
			if (primitive.ResolveTransform(m, element))
				transform_value *= m;
		}
		t->ClearPrimitives();
		t->AddPrimitive({ Matrix3D{transform_value} });
	}

	return PrepareTransformResult::ChangedT0andT1;
}


static bool PrepareTransforms(std::vector<AnimationKey>& keys, Element& element)
{
	for (int i = 1; i < (int)keys.size();)
	{
		auto& ref0 = keys[i - 1].value.Get<TransformRef>();
		auto& ref1 = keys[i].value.Get<TransformRef>();

		auto result = PrepareTransformPair(*ref0, *ref1, element);

		bool changed_t0 = (result == PrepareTransformResult::ChangedT0 || result == PrepareTransformResult::ChangedT0andT1);
		if (changed_t0 && i > 1)
			--i;
		else
			++i;
	}
	return true;
}



bool ElementAnimation::AddKey(float time, const Property & property, Element& element)
{
	if (property.unit != property_unit)
		return false;

	keys.push_back({ time, property.value });

	if (property.unit == Property::TRANSFORM)
	{
		PrepareTransforms(keys, element);
	}

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