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
#include "ElementStyle.h"
#include "../../Include/Rocket/Core/TransformPrimitive.h"
#include "../../Include/Rocket/Core/StyleSheetSpecification.h"

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

// Merges all the primitives to a single DecomposedMatrix4 primitive
static bool CombineAndDecompose(Transform& t, Element& e)
{
	Matrix4f m = Matrix4f::Identity();

	for (auto& primitive : t.GetPrimitives())
	{
		Matrix4f m_primitive;
		if (primitive.ResolveTransform(m_primitive, e))
			m *= m_primitive;
	}

	Transforms::DecomposedMatrix4 decomposed;

	if (!decomposed.Decompose(m))
		return false;

	t.ClearPrimitives();
	t.AddPrimitive(decomposed);

	return true;
}


static Property InterpolateProperties(const Property & p0, const Property& p1, float alpha, Element& element, const PropertyDefinition* definition)
{
	if ((p0.unit & Property::NUMBER_LENGTH_PERCENT) && (p1.unit & Property::NUMBER_LENGTH_PERCENT))
	{
		if (p0.unit == p1.unit || !definition)
		{
			// If we have the same units, we can just interpolate regardless of what the value represents.
			// Or if we have distinct units but no definition, all bets are off. This shouldn't occur, just interpolate values.
			float f0 = p0.value.Get<float>();
			float f1 = p1.value.Get<float>();
			float f = (1.0f - alpha) * f0 + alpha * f1;
			return Property{ f, p0.unit };
		}
		else
		{
			// Otherwise, convert units to pixels.
			float f0 = element.GetStyle()->ResolveNumericProperty(&p0, definition->GetRelativeTarget());
			float f1 = element.GetStyle()->ResolveNumericProperty(&p1, definition->GetRelativeTarget());
			float f = (1.0f - alpha) * f0 + alpha * f1;
			return Property{ f, Property::PX };
		}
	}

	if (p0.unit == Property::COLOUR && p1.unit == Property::COLOUR)
	{
		Colourf c0 = ColourToLinearSpace(p0.value.Get<Colourb>());
		Colourf c1 = ColourToLinearSpace(p1.value.Get<Colourb>());

		Colourf c = c0 * (1.0f - alpha) + c1 * alpha;

		return Property{ ColourFromLinearSpace(c), Property::COLOUR };
	}

	if (p0.unit == Property::TRANSFORM && p1.unit == Property::TRANSFORM)
	{
		using namespace Rocket::Core::Transforms;

		// Build the new, interpolating transform
		std::unique_ptr<Transform> t(new Transform);

		auto t0 = p0.value.Get<TransformRef>();
		auto t1 = p1.value.Get<TransformRef>();

		const auto& prim0 = t0->GetPrimitives();
		const auto& prim1 = t1->GetPrimitives();

		if (prim0.size() != prim1.size())
		{
			ROCKET_ERRORMSG("Transform primitives not of same size during interpolation. Were the transforms properly prepared for interpolation?");
			return Property{ t0, Property::TRANSFORM };
		}

		for (size_t i = 0; i < prim0.size(); i++)
		{
			Primitive p = prim0[i];
			if (!p.InterpolateWith(prim1[i], alpha))
			{
				ROCKET_ERRORMSG("Transform primitives can not be interpolated. Were the transforms properly prepared for interpolation?");
				return Property{ t0, Property::TRANSFORM };
			}
			t->AddPrimitive(p);
		}

		return Property{ TransformRef(std::move(t)), Property::TRANSFORM };
	}

	return alpha < 0.5f ? p0 : p1;
}




enum class PrepareTransformResult { Unchanged = 0, ChangedT0 = 1, ChangedT1 = 2, ChangedT0andT1 = 3, Invalid = 4 };

static PrepareTransformResult PrepareTransformPair(Transform& t0, Transform& t1, Element& element)
{
	using namespace Transforms;

	// Insert or modify primitives such that the two transforms match exactly in both number of and types of primitives.
	// Based largely on https://drafts.csswg.org/css-transforms-1/#interpolation-of-transforms

	auto& prims0 = t0.GetPrimitives();
	auto& prims1 = t1.GetPrimitives();

	// Check for trivial case where they contain the same primitives
	if (prims0.size() == prims1.size())
	{
		PrepareTransformResult result = PrepareTransformResult::Unchanged;
		bool same_primitives = true;
		for (size_t i = 0; i < prims0.size(); i++)
		{
			auto p0_type = prims0[i].primitive.type;
			auto p1_type = prims1[i].primitive.type;
			if (p0_type != p1_type)
			{
				// They are not the same, but see if we can convert them to their more generic form
				if (!Primitive::TryConvertToMatchingGenericType(prims0[i], prims1[i]))
				{
					same_primitives = false;
					break;
				}
				if (prims0[i].primitive.type != p0_type)
					(int&)result |= (int)PrepareTransformResult::ChangedT0;
				if (prims1[i].primitive.type != p1_type)
					(int&)result |= (int)PrepareTransformResult::ChangedT1;
			}
		}
		if (same_primitives)
			return result;
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

		size_t i_big = 0;
		bool match_success = true;
		bool changed_big = false;

		// Iterate through the small set to see if its types fit into the big set
		for (size_t i_small = 0; i_small < small.size(); i_small++)
		{
			match_success = false;
			auto small_type = small[i_small].primitive.type;

			for (; i_big < big.size(); i_big++)
			{
				auto big_type = big[i_big].primitive.type;

				if (small_type == big_type)
				{
					// Exact match
					match_success = true;
				}
				else if (Primitive::TryConvertToMatchingGenericType(small[i_small], big[i_big]))
				{
					// They matched in their more generic form, one or both primitives converted
					match_success = true;
					if (big[i_big].primitive.type != big_type)
						changed_big = true;
				}

				if (match_success)
				{
					matching_indices.push_back(i_big);
					match_success = true;
					i_big += 1;
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

			// The small set has always been changed if we get here, but the big set is only changed
			// if one or more of its primitives were converted to a general form.
			if (changed_big)
				return PrepareTransformResult::ChangedT0andT1;

			return (prims0_smallest ? PrepareTransformResult::ChangedT0 : PrepareTransformResult::ChangedT1);
		}
	}


	// If we get here, things get tricky. Need to do full matrix interpolation.
	// In short, we decompose the Transforms into translation, rotation, scale, skew and perspective components. 
	// Then, during update, interpolate these components and combine into a new transform matrix.
	if (!CombineAndDecompose(t0, element))
		return PrepareTransformResult::Invalid;
	if (!CombineAndDecompose(t1, element))
		return PrepareTransformResult::Invalid;

	return PrepareTransformResult::ChangedT0andT1;
}


static bool PrepareTransforms(std::vector<AnimationKey>& keys, Element& element, int start_index)
{
	if (keys.size() < 2 || start_index < 1)
		return false;

	const int N = (int)keys.size();

	int count_iterations = -1;
	const int max_iterations = 3 * N;
	if (start_index < 1) start_index = 1;

	std::vector<bool> dirty_list(N + 1, false);
	dirty_list[start_index] = true;

	// For each pair of keys, match the transform primitives such that they can be interpolated during animation update
	for (int i = start_index; i < N && count_iterations < max_iterations; count_iterations++)
	{
		if (!dirty_list[i])
		{
			++i;
			continue;
		}

		auto& prop0 = keys[i - 1].property;
		auto& prop1 = keys[i].property;

		if(prop0.unit != Property::TRANSFORM || prop1.unit != Property::TRANSFORM)
			return false;

		auto t0 = prop0.value.Get<TransformRef>();
		auto t1 = prop1.value.Get<TransformRef>();

		auto result = PrepareTransformPair(*t0, *t1, element);

		if (result == PrepareTransformResult::Invalid)
			return false;

		bool changed_t0 = ((int)result & (int)PrepareTransformResult::ChangedT0);
		bool changed_t1 = ((int)result & (int)PrepareTransformResult::ChangedT1);

		dirty_list[i] = false;
		dirty_list[i - 1] = dirty_list[i - 1] || changed_t0;
		dirty_list[i + 1] = dirty_list[i + 1] || changed_t1;

		if (changed_t0 && i > 1)
			--i;
		else
			++i;
	}

	// Something has probably gone wrong if we exceeded max_iterations, possibly a bug in PrepareTransformPair()
	return (count_iterations < max_iterations);
}


ElementAnimation::ElementAnimation(const String& property_name, const Property& current_value, double start_world_time, float duration, int num_iterations, bool alternate_direction, bool is_transition)
	: property_name(property_name), duration(duration), num_iterations(num_iterations), alternate_direction(alternate_direction),
	last_update_world_time(start_world_time), time_since_iteration_start(0.0f), current_iteration(0), reverse_direction(false), animation_complete(false), is_transition(is_transition)
{
	if (!current_value.definition)
	{
		Log::Message(Log::LT_WARNING, "Property in animation key did not have a definition (while adding key '%s').", current_value.ToString().CString());
	}
	InternalAddKey(AnimationKey{ 0.0f, current_value, Tween{} });
}

bool ElementAnimation::InternalAddKey(AnimationKey key)
{
	int valid_properties = (Property::NUMBER_LENGTH_PERCENT | Property::ANGLE | Property::COLOUR | Property::TRANSFORM);

	if (!(key.property.unit & valid_properties))
	{
		Log::Message(Log::LT_WARNING, "Property '%s' is not a valid target for interpolation.", key.property.ToString().CString());
		return false;
	}

	if (key.property.unit == Property::TRANSFORM)
	{
		if (!key.property.value.Get<TransformRef>())
			key.property.value.Reset(TransformRef(new Transform));
	}

	keys.push_back(key);

	return true;
}


bool ElementAnimation::AddKey(float target_time, const Property & in_property, Element& element, Tween tween, bool extend_duration)
{
	if (keys.empty())
	{
		Log::Message(Log::LT_WARNING, "Element animation was not initialized properly, can't add key.");
		return false;
	}
	if (!InternalAddKey(AnimationKey{ target_time, in_property, tween }))
	{
		return false;
	}

	bool result = true;
	auto& property = keys.back().property;

	if (property.unit == Property::TRANSFORM)
	{
		bool must_decompose = false;
		auto& transform = *property.value.Get<TransformRef>();

		for (auto& primitive : transform.GetPrimitives())
		{
			if (!primitive.PrepareForInterpolation(element))
			{
				must_decompose = true;
				break;
			}
		}

		if(must_decompose)
			result = CombineAndDecompose(transform, element);

		if (result)
			result = PrepareTransforms(keys, element, (int)keys.size() - 1);
	}

	if(result)
	{
		if(extend_duration) 
			duration = target_time;
	}
	else
	{
		keys.pop_back();
	}

	return result;
}

float ElementAnimation::GetInterpolationFactorAndKeys(int* out_key0, int* out_key1) const
{
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
		key0 = (key1 == 0 ? 0 : key1 - 1);
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

	alpha = keys[key1].tween(alpha);

	if (out_key0) *out_key0 = key0;
	if (out_key1) *out_key1 = key1;

	return alpha;
}



Property ElementAnimation::UpdateAndGetProperty(double world_time, Element& element)
{
	ROCKET_ASSERT(keys.size() >= 2);
	float dt = float(world_time - last_update_world_time);
	if (animation_complete || dt <= 0.0f)
		return Property{};

	dt = Math::Min(dt, 0.1f);

	last_update_world_time = world_time;
	time_since_iteration_start += dt;

	if (time_since_iteration_start >= duration)
	{
		// Next iteration
		current_iteration += 1;

		if (num_iterations == -1 || (current_iteration >= 0 && current_iteration < num_iterations))
		{
			time_since_iteration_start -= duration;

			if (alternate_direction)
				reverse_direction = !reverse_direction;
		}
		else
		{
			animation_complete = true;
			time_since_iteration_start = duration;
		}
	}

	int key0 = -1;
	int key1 = -1;

	float alpha = GetInterpolationFactorAndKeys(&key0, &key1);

	Property result = InterpolateProperties(keys[key0].property, keys[key1].property, alpha, element, keys[0].property.definition);
	
	return result;
}


}
}