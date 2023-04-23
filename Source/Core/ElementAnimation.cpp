/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2018 Michael R. P. Ragazzon
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

#include "ElementAnimation.h"
#include "../../Include/RmlUi/Core/DecoratorInstancer.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/Factory.h"
#include "../../Include/RmlUi/Core/PropertyDefinition.h"
#include "../../Include/RmlUi/Core/PropertySpecification.h"
#include "../../Include/RmlUi/Core/StyleSheet.h"
#include "../../Include/RmlUi/Core/StyleSheetSpecification.h"
#include "../../Include/RmlUi/Core/StyleSheetTypes.h"
#include "../../Include/RmlUi/Core/Transform.h"
#include "../../Include/RmlUi/Core/TransformPrimitive.h"
#include "ElementStyle.h"
#include "TransformUtilities.h"

namespace Rml {

static Colourf ColourToLinearSpace(Colourb c)
{
	Colourf result;
	// Approximate inverse sRGB function
	result.red = c.red / 255.f;
	result.red *= result.red;
	result.green = c.green / 255.f;
	result.green *= result.green;
	result.blue = c.blue / 255.f;
	result.blue *= result.blue;
	result.alpha = c.alpha / 255.f;
	return result;
}

static Colourb ColourFromLinearSpace(Colourf c)
{
	Colourb result;
	result.red = (byte)Math::Clamp(Math::SquareRoot(c.red) * 255.f, 0.0f, 255.f);
	result.green = (byte)Math::Clamp(Math::SquareRoot(c.green) * 255.f, 0.0f, 255.f);
	result.blue = (byte)Math::Clamp(Math::SquareRoot(c.blue) * 255.f, 0.0f, 255.f);
	result.alpha = (byte)Math::Clamp(c.alpha * 255.f, 0.0f, 255.f);
	return result;
}

// Merges all the primitives to a single DecomposedMatrix4 primitive
static bool CombineAndDecompose(Transform& t, Element& e)
{
	Matrix4f m = Matrix4f::Identity();

	for (TransformPrimitive& primitive : t.GetPrimitives())
	{
		Matrix4f m_primitive = TransformUtilities::ResolveTransform(primitive, e);
		m *= m_primitive;
	}

	Transforms::DecomposedMatrix4 decomposed;

	if (!TransformUtilities::Decompose(decomposed, m))
		return false;

	t.ClearPrimitives();
	t.AddPrimitive(decomposed);

	return true;
}

static Property InterpolateProperties(const Property& p0, const Property& p1, float alpha, Element& element, const PropertyDefinition* definition)
{
	if (Any(p0.unit & Unit::NUMBER_LENGTH_PERCENT) && Any(p1.unit & Unit::NUMBER_LENGTH_PERCENT))
	{
		if (p0.unit == p1.unit || !definition)
		{
			// If we have the same units, we can just interpolate regardless of what the value represents.
			// Or if we have distinct units but no definition, all bets are off. This shouldn't occur, just interpolate values.
			float f0 = p0.value.Get<float>();
			float f1 = p1.value.Get<float>();
			float f = (1.0f - alpha) * f0 + alpha * f1;
			return Property{f, p0.unit};
		}
		else
		{
			// Otherwise, convert units to pixels.
			float f0 = element.GetStyle()->ResolveRelativeLength(p0.GetNumericValue(), definition->GetRelativeTarget());
			float f1 = element.GetStyle()->ResolveRelativeLength(p1.GetNumericValue(), definition->GetRelativeTarget());
			float f = (1.0f - alpha) * f0 + alpha * f1;
			return Property{f, Unit::PX};
		}
	}

	if (p0.unit == Unit::KEYWORD && p1.unit == Unit::KEYWORD)
	{
		// Discrete interpolation, swap at alpha = 0.5.
		// Special case for the 'visibility' property as in the CSS specs:
		//   Apply the visible property if present during the entire transition period, ie. alpha (0,1).
		if (definition && definition->GetId() == PropertyId::Visibility)
		{
			if (p0.Get<int>() == (int)Style::Visibility::Visible)
				return alpha < 1.f ? p0 : p1;
			else if (p1.Get<int>() == (int)Style::Visibility::Visible)
				return alpha <= 0.f ? p0 : p1;
		}

		return alpha < 0.5f ? p0 : p1;
	}

	if (p0.unit == Unit::COLOUR && p1.unit == Unit::COLOUR)
	{
		Colourf c0 = ColourToLinearSpace(p0.value.Get<Colourb>());
		Colourf c1 = ColourToLinearSpace(p1.value.Get<Colourb>());

		Colourf c = c0 * (1.0f - alpha) + c1 * alpha;

		return Property{ColourFromLinearSpace(c), Unit::COLOUR};
	}

	if (p0.unit == Unit::TRANSFORM && p1.unit == Unit::TRANSFORM)
	{
		auto& t0 = p0.value.GetReference<TransformPtr>();
		auto& t1 = p1.value.GetReference<TransformPtr>();

		const auto& prim0 = t0->GetPrimitives();
		const auto& prim1 = t1->GetPrimitives();

		if (prim0.size() != prim1.size())
		{
			RMLUI_ERRORMSG("Transform primitives not of same size during interpolation. Were the transforms properly prepared for interpolation?");
			return Property{t0, Unit::TRANSFORM};
		}

		// Build the new, interpolating transform
		UniquePtr<Transform> t(new Transform);
		t->GetPrimitives().reserve(t0->GetPrimitives().size());

		for (size_t i = 0; i < prim0.size(); i++)
		{
			TransformPrimitive p = prim0[i];
			if (!TransformUtilities::InterpolateWith(p, prim1[i], alpha))
			{
				RMLUI_ERRORMSG("Transform primitives can not be interpolated. Were the transforms properly prepared for interpolation?");
				return Property{t0, Unit::TRANSFORM};
			}
			t->AddPrimitive(p);
		}

		return Property{TransformPtr(std::move(t)), Unit::TRANSFORM};
	}

	if (p0.unit == Unit::DECORATOR && p1.unit == Unit::DECORATOR)
	{
		auto DiscreteInterpolation = [&]() { return alpha < 0.5f ? p0 : p1; };

		// We construct DecoratorDeclarationView from declaration if it has instancer, otherwise we look for DecoratorSpecification and return
		// DecoratorDeclarationView from it
		auto GetDecoratorDeclarationView = [&](const DecoratorDeclaration& declaration) -> DecoratorDeclarationView {
			if (declaration.instancer)
				return DecoratorDeclarationView{declaration};

			const StyleSheet* style_sheet = element.GetStyleSheet();
			if (!style_sheet)
			{
				Log::Message(Log::LT_WARNING, "Failed to get element stylesheet for '%s' decorator rule.", declaration.type.c_str());
				return DecoratorDeclarationView{declaration};
			}

			const DecoratorSpecification* specification = style_sheet->GetDecoratorSpecification(declaration.type);
			if (!specification)
			{
				Log::Message(Log::LT_WARNING, "Could not find DecoratorSpecification for '%s' decorator rule.", declaration.type.c_str());
				return DecoratorDeclarationView{declaration};
			}

			return DecoratorDeclarationView{specification};
		};

		auto& ptr0 = p0.value.GetReference<DecoratorsPtr>();
		auto& ptr1 = p1.value.GetReference<DecoratorsPtr>();
		if (!ptr0 || !ptr1)
		{
			RMLUI_ERRORMSG("Invalid decorator pointer, were the decorator keys properly prepared?");
			return DiscreteInterpolation();
		}

		const bool p0_smaller = (ptr0->list.size() < ptr1->list.size());
		auto& small = (p0_smaller ? ptr0->list : ptr1->list);
		auto& big = (p0_smaller ? ptr1->list : ptr0->list);

		// Build the new, interpolated decorator.
		UniquePtr<DecoratorDeclarationList> decorator(new DecoratorDeclarationList);
		decorator->list.reserve(ptr0->list.size());

		// Interpolate decorators that have common types.
		for (size_t i = 0; i < small.size(); i++)
		{
			DecoratorDeclarationView d0_view{GetDecoratorDeclarationView(ptr0->list[i])};
			DecoratorDeclarationView d1_view{GetDecoratorDeclarationView(ptr1->list[i])};

			if (!d0_view.instancer || !d1_view.instancer)
				return DiscreteInterpolation();

			if (d0_view.instancer != d1_view.instancer || d0_view.type != d1_view.type ||
				d0_view.properties.GetNumProperties() != d1_view.properties.GetNumProperties())
			{
				// Incompatible decorators, fall back to discrete interpolation.
				return DiscreteInterpolation();
			}

			decorator->list.push_back(DecoratorDeclaration{d0_view.type, d0_view.instancer, PropertyDictionary()});
			PropertyDictionary& props = decorator->list.back().properties;

			const auto& props0 = d0_view.properties.GetProperties();
			const auto& props1 = d1_view.properties.GetProperties();

			for (const auto& pair0 : props0)
			{
				const PropertyId id = pair0.first;
				const Property& prop0 = pair0.second;

				auto it = props1.find(id);
				if (it == props1.end())
				{
					RMLUI_ERRORMSG("Incompatible decorator properties.");
					return DiscreteInterpolation();
				}
				const Property& prop1 = it->second;

				Property p = InterpolateProperties(prop0, prop1, alpha, element, prop0.definition);
				p.definition = prop0.definition;
				props.SetProperty(id, p);
			}
		}

		// Append any trailing decorators from the largest list and interpolate against the default values of its type.
		for (size_t i = small.size(); i < big.size(); i++)
		{
			DecoratorDeclarationView dbig_view{GetDecoratorDeclarationView(big[i])};

			if (!dbig_view.instancer)
				return DiscreteInterpolation();

			decorator->list.push_back(DecoratorDeclaration{dbig_view.type, dbig_view.instancer, PropertyDictionary()});
			DecoratorDeclaration& d_new = decorator->list.back();

			const PropertySpecification& specification = d_new.instancer->GetPropertySpecification();

			const PropertyMap& props_big = dbig_view.properties.GetProperties();
			for (const auto& pair_big : props_big)
			{
				const PropertyId id = pair_big.first;
				const PropertyDefinition* underlying_definition = specification.GetProperty(id);
				if (!underlying_definition)
					return DiscreteInterpolation();

				const Property& p_big = pair_big.second;
				const Property& p_small = *underlying_definition->GetDefaultValue();
				const Property& p_interp0 = (p0_smaller ? p_small : p_big);
				const Property& p_interp1 = (p0_smaller ? p_big : p_small);

				Property p = InterpolateProperties(p_interp0, p_interp1, alpha, element, p_big.definition);
				p.definition = p_big.definition;
				d_new.properties.SetProperty(id, p);
			}
		}

		return Property{DecoratorsPtr(std::move(decorator)), Unit::DECORATOR};
	}

	// Fall back to discrete interpolation for incompatible units.
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
			auto p0_type = prims0[i].type;
			auto p1_type = prims1[i].type;

			// See if they are the same or can be converted to a matching generic type.
			if (TransformUtilities::TryConvertToMatchingGenericType(prims0[i], prims1[i]))
			{
				if (prims0[i].type != p0_type)
					result = PrepareTransformResult((int)result | (int)PrepareTransformResult::ChangedT0);
				if (prims1[i].type != p1_type)
					result = PrepareTransformResult((int)result | (int)PrepareTransformResult::ChangedT1);
			}
			else
			{
				same_primitives = false;
				break;
			}
		}
		if (same_primitives)
			return result;
	}

	if (prims0.size() != prims1.size())
	{
		// Try to match the smallest set of primitives to the larger set, set missing keys in the small set to identity.
		// Requirement: The small set must match types in the same order they appear in the big set.
		// Example: (letter indicates type, number represents values)
		// big:       a0 b0 c0 b1
		//               ^     ^
		// small:     b2 b3
		//            ^  ^
		// new small: a1 b2 c1 b3
		bool prims0_smallest = (prims0.size() < prims1.size());

		auto& small = (prims0_smallest ? prims0 : prims1);
		auto& big = (prims0_smallest ? prims1 : prims0);

		Vector<size_t> matching_indices; // Indices into 'big' for matching types
		matching_indices.reserve(small.size() + 1);

		size_t i_big = 0;
		bool match_success = true;
		bool changed_big = false;

		// Iterate through the small set to see if its types fit into the big set
		for (size_t i_small = 0; i_small < small.size(); i_small++)
		{
			match_success = false;

			for (; i_big < big.size(); i_big++)
			{
				auto big_type = big[i_big].type;

				if (TransformUtilities::TryConvertToMatchingGenericType(small[i_small], big[i_big]))
				{
					// They matched exactly or in their more generic form. One or both primitives may have been converted.
					match_success = true;
					if (big[i_big].type != big_type)
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
					TransformPrimitive p = big[i];
					TransformUtilities::SetIdentity(p);
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

static bool PrepareTransforms(Vector<AnimationKey>& keys, Element& element, int start_index)
{
	bool result = true;

	// Prepare each transform individually.
	for (int i = start_index; i < (int)keys.size(); i++)
	{
		Property& property = keys[i].property;
		RMLUI_ASSERT(property.value.GetType() == Variant::TRANSFORMPTR);

		if (!property.value.GetReference<TransformPtr>())
			property.value = MakeShared<Transform>();

		bool must_decompose = false;
		Transform& transform = *property.value.GetReference<TransformPtr>();

		for (TransformPrimitive& primitive : transform.GetPrimitives())
		{
			if (!TransformUtilities::PrepareForInterpolation(primitive, element))
			{
				must_decompose = true;
				break;
			}
		}

		if (must_decompose)
			result &= CombineAndDecompose(transform, element);
	}

	if (!result)
		return false;

	// We don't need to prepare the transforms pairwise if we only have a single key added so far.
	if (keys.size() < 2 || start_index < 1)
		return true;

	// Now, prepare the transforms pair-wise so they can be interpolated.
	const int N = (int)keys.size();

	int count_iterations = -1;
	const int max_iterations = 3 * N;

	Vector<bool> dirty_list(N + 1, false);
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

		if (prop0.unit != Unit::TRANSFORM || prop1.unit != Unit::TRANSFORM)
			return false;

		auto& t0 = prop0.value.GetReference<TransformPtr>();
		auto& t1 = prop1.value.GetReference<TransformPtr>();

		auto prepare_result = PrepareTransformPair(*t0, *t1, element);

		if (prepare_result == PrepareTransformResult::Invalid)
			return false;

		bool changed_t0 = ((int)prepare_result & (int)PrepareTransformResult::ChangedT0);
		bool changed_t1 = ((int)prepare_result & (int)PrepareTransformResult::ChangedT1);

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

static void PrepareDecorator(AnimationKey& key)
{
	Property& property = key.property;
	RMLUI_ASSERT(property.value.GetType() == Variant::DECORATORSPTR);

	if (!property.value.GetReference<DecoratorsPtr>())
		property.value = MakeShared<DecoratorDeclarationList>();
}

ElementAnimation::ElementAnimation(PropertyId property_id, ElementAnimationOrigin origin, const Property& current_value, Element& element,
	double start_world_time, float duration, int num_iterations, bool alternate_direction) :
	property_id(property_id),
	duration(duration), num_iterations(num_iterations), alternate_direction(alternate_direction), last_update_world_time(start_world_time),
	origin(origin)
{
	if (!current_value.definition)
	{
		Log::Message(Log::LT_WARNING, "Property in animation key did not have a definition (while adding key '%s').",
			current_value.ToString().c_str());
	}
	InternalAddKey(0.0f, current_value, element, Tween{});
}

bool ElementAnimation::InternalAddKey(float time, const Property& in_property, Element& element, Tween tween)
{
	const Units valid_units = (Unit::NUMBER_LENGTH_PERCENT | Unit::ANGLE | Unit::COLOUR | Unit::TRANSFORM | Unit::KEYWORD | Unit::DECORATOR);

	if (!Any(in_property.unit & valid_units))
	{
		Log::Message(Log::LT_WARNING, "Property value '%s' is not a valid target for interpolation.", in_property.ToString().c_str());
		return false;
	}

	keys.emplace_back(time, in_property, tween);
	bool result = true;

	if (keys.back().property.unit == Unit::TRANSFORM)
	{
		result = PrepareTransforms(keys, element, (int)keys.size() - 1);
	}
	else if (keys.back().property.unit == Unit::DECORATOR)
	{
		PrepareDecorator(keys.back());
	}

	if (!result)
	{
		Log::Message(Log::LT_WARNING, "Could not add animation key with property '%s'.", in_property.ToString().c_str());
		keys.pop_back();
	}

	return result;
}

bool ElementAnimation::AddKey(float target_time, const Property& in_property, Element& element, Tween tween, bool extend_duration)
{
	if (!IsInitalized())
	{
		Log::Message(Log::LT_WARNING, "Element animation was not initialized properly, can't add key.");
		return false;
	}
	if (!InternalAddKey(target_time, in_property, element, tween))
	{
		return false;
	}

	if (extend_duration)
		duration = target_time;

	return true;
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

		if (key1 < 0)
			key1 = (int)keys.size() - 1;
		key0 = (key1 == 0 ? 0 : key1 - 1);
	}

	RMLUI_ASSERT(key0 >= 0 && key0 < (int)keys.size() && key1 >= 0 && key1 < (int)keys.size());

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

	if (out_key0)
		*out_key0 = key0;
	if (out_key1)
		*out_key1 = key1;

	return alpha;
}

Property ElementAnimation::UpdateAndGetProperty(double world_time, Element& element)
{
	float dt = float(world_time - last_update_world_time);
	if (keys.size() < 2 || animation_complete || dt <= 0.0f)
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

} // namespace Rml
