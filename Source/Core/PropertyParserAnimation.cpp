/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2018 Michael R. P. Ragazzon
 * Copyright (c) 2019 The RmlUi Team, and contributors
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


#include "PropertyParserAnimation.h"
#include "PropertyShorthandDefinition.h"
#include "../../Include/RmlUi/Core/PropertyDefinition.h"
#include "../../Include/RmlUi/Core/PropertyIdSet.h"
#include "../../Include/RmlUi/Core/StringUtilities.h"
#include "../../Include/RmlUi/Core/StyleSheetSpecification.h"


namespace Rml {

struct Keyword {
	enum Type { NONE, TWEEN, ALL, ALTERNATE, INFINITE, PAUSED } type;
	Tween tween;
	Keyword(Tween tween) : type(TWEEN), tween(tween) {}
	Keyword(Type type) : type(type) {}

	bool ValidTransition() const {
		return type == NONE || type == TWEEN || type == ALL;
	}
	bool ValidAnimation() const {
		return type == NONE || type == TWEEN || type == ALTERNATE || type == INFINITE || type == PAUSED;
	}
};


static const UnorderedMap<String, Keyword> keywords = {
		{"none", {Keyword::NONE} },
		{"all", {Keyword::ALL}},
		{"alternate", {Keyword::ALTERNATE}},
		{"infinite", {Keyword::INFINITE}},
		{"paused", {Keyword::PAUSED}},

		{"back-in", {Tween{Tween::Back, Tween::In}}},
		{"back-out", {Tween{Tween::Back, Tween::Out}}},
		{"back-in-out", {Tween{Tween::Back, Tween::InOut}}},

		{"bounce-in", {Tween{Tween::Bounce, Tween::In}}},
		{"bounce-out", {Tween{Tween::Bounce, Tween::Out}}},
		{"bounce-in-out", {Tween{Tween::Bounce, Tween::InOut}}},

		{"circular-in", {Tween{Tween::Circular, Tween::In}}},
		{"circular-out", {Tween{Tween::Circular, Tween::Out}}},
		{"circular-in-out", {Tween{Tween::Circular, Tween::InOut}}},

		{"cubic-in", {Tween{Tween::Cubic, Tween::In}}},
		{"cubic-out", {Tween{Tween::Cubic, Tween::Out}}},
		{"cubic-in-out", {Tween{Tween::Cubic, Tween::InOut}}},

		{"elastic-in", {Tween{Tween::Elastic, Tween::In}}},
		{"elastic-out", {Tween{Tween::Elastic, Tween::Out}}},
		{"elastic-in-out", {Tween{Tween::Elastic, Tween::InOut}}},

		{"exponential-in", {Tween{Tween::Exponential, Tween::In}}},
		{"exponential-out", {Tween{Tween::Exponential, Tween::Out}}},
		{"exponential-in-out", {Tween{Tween::Exponential, Tween::InOut}}},

		{"linear-in", {Tween{Tween::Linear, Tween::In}}},
		{"linear-out", {Tween{Tween::Linear, Tween::Out}}},
		{"linear-in-out", {Tween{Tween::Linear, Tween::InOut}}},

		{"quadratic-in", {Tween{Tween::Quadratic, Tween::In}}},
		{"quadratic-out", {Tween{Tween::Quadratic, Tween::Out}}},
		{"quadratic-in-out", {Tween{Tween::Quadratic, Tween::InOut}}},

		{"quartic-in", {Tween{Tween::Quartic, Tween::In}}},
		{"quartic-out", {Tween{Tween::Quartic, Tween::Out}}},
		{"quartic-in-out", {Tween{Tween::Quartic, Tween::InOut}}},

		{"quintic-in", {Tween{Tween::Quintic, Tween::In}}},
		{"quintic-out", {Tween{Tween::Quintic, Tween::Out}}},
		{"quintic-in-out", {Tween{Tween::Quintic, Tween::InOut}}},

		{"sine-in", {Tween{Tween::Sine, Tween::In}}},
		{"sine-out", {Tween{Tween::Sine, Tween::Out}}},
		{"sine-in-out", {Tween{Tween::Sine, Tween::InOut}}},
};




PropertyParserAnimation::PropertyParserAnimation(Type type) : type(type)
{
}


static bool ParseAnimation(Property & property, const StringList& animation_values)
{
	AnimationList animation_list;

	for (const String& single_animation_value : animation_values)
	{
		Animation animation;

		StringList arguments;
		StringUtilities::ExpandString(arguments, single_animation_value, ' ');

		bool duration_found = false;
		bool delay_found = false;
		bool num_iterations_found = false;

		for (auto& argument : arguments)
		{
			if (argument.empty())
				continue;

			// See if we have a <keyword> or <tween> specifier as defined in keywords
			auto it = keywords.find(argument); 
			if (it != keywords.end() && it->second.ValidAnimation())
			{
				switch (it->second.type)
				{
				case Keyword::NONE:
				{
					if (animation_list.size() > 0) // The none keyword can not be part of multiple definitions
						return false;
					property = Property{ AnimationList{}, Property::ANIMATION };
					return true;
				}
				break;
				case Keyword::TWEEN:
					animation.tween = it->second.tween;
					break;
				case Keyword::ALTERNATE:
					animation.alternate = true;
					break;
				case Keyword::INFINITE:
					if (num_iterations_found)
						return false;
					animation.num_iterations = -1;
					num_iterations_found = true;
					break;
				case Keyword::PAUSED:
					animation.paused = true;
					break;
				default:
					break;
				}
			}
			else
			{
				// Either <duration>, <delay>, <num_iterations> or a <keyframes-name>
				float number = 0.0f;
				int count = 0;

				if (sscanf(argument.c_str(), "%fs%n", &number, &count) == 1)
				{
					// Found a number, if there was an 's' unit, count will be positive
					if (count > 0)
					{
						// Duration or delay was assigned
						if (!duration_found)
						{
							duration_found = true;
							animation.duration = number;
						}
						else if (!delay_found)
						{
							delay_found = true;
							animation.delay = number;
						}
						else
							return false;
					}
					else
					{
						// No 's' unit means num_iterations was found
						if (!num_iterations_found)
						{
							animation.num_iterations = Math::RoundToInteger(number);
							num_iterations_found = true;
						}
						else
							return false;
					}
				}
				else
				{
					// Must be an animation name
					animation.name = argument;
				}
			}
		}

		// Validate the parsed transition
		if (animation.name.empty() || animation.duration <= 0.0f || (animation.num_iterations < -1 || animation.num_iterations == 0))
		{
			return false;
		}

		animation_list.push_back(std::move(animation));
	}

	property.value = std::move(animation_list);
	property.unit = Property::ANIMATION;

	return true;
}


static bool ParseTransition(Property & property, const StringList& transition_values)
{
	TransitionList transition_list{ false, false, {} };

	for (const String& single_transition_value : transition_values)
	{

		Transition transition;
		PropertyIdSet target_property_names;

		StringList arguments;
		StringUtilities::ExpandString(arguments, single_transition_value, ' ');


		bool duration_found = false;
		bool delay_found = false;
		bool reverse_adjustment_factor_found = false;

		for (auto& argument : arguments)
		{
			if (argument.empty())
				continue;

			// See if we have a <keyword> or <tween> specifier as defined in keywords
			auto it = keywords.find(argument);
			if (it != keywords.end() && it->second.ValidTransition())
			{
				if (it->second.type == Keyword::NONE)
				{
					if (transition_list.transitions.size() > 0) // The none keyword can not be part of multiple definitions
						return false;
					property = Property{ TransitionList{true, false, {}}, Property::TRANSITION };
					return true;
				}
				else if (it->second.type == Keyword::ALL)
				{
					if (transition_list.transitions.size() > 0) // The all keyword can not be part of multiple definitions
						return false;
					transition_list.all = true;
				}
				else if (it->second.type == Keyword::TWEEN)
				{
					transition.tween = it->second.tween;
				}
			}
			else
			{
				// Either <duration>, <delay> or a <property name>
				float number = 0.0f;
				int count = 0;

				if (sscanf(argument.c_str(), "%fs%n", &number, &count) == 1)
				{
					// Found a number, if there was an 's' unit, count will be positive
					if (count > 0)
					{
						// Duration or delay was assigned
						if (!duration_found)
						{
							duration_found = true;
							transition.duration = number;
						}
						else if (!delay_found)
						{
							delay_found = true;
							transition.delay = number;
						}
						else
							return false;
					}
					else
					{
						// No 's' unit means reverse adjustment factor was found
						if (!reverse_adjustment_factor_found)
						{
							reverse_adjustment_factor_found = true;
							transition.reverse_adjustment_factor = number;
						}
						else
							return false;
					}
				}
				else
				{
					// Must be a property name or shorthand, expand now
					if (auto shorthand = StyleSheetSpecification::GetShorthand(argument))
					{
						PropertyIdSet underlying_properties = StyleSheetSpecification::GetShorthandUnderlyingProperties(shorthand->id);
						target_property_names |= underlying_properties;
					}
					else if (auto definition = StyleSheetSpecification::GetProperty(argument))
					{
						// Single property
						target_property_names.Insert(definition->GetId());
					}
					else
					{
						// Unknown property name
						return false;
					}
				}
			}
		}

		// Validate the parsed transition
		if (target_property_names.Empty() || transition.duration <= 0.0f || transition.reverse_adjustment_factor < 0.0f || transition.reverse_adjustment_factor > 1.0f
			|| (transition_list.all && target_property_names.Size() != 1))
		{
			return false;
		}

		for (const auto& property_name : target_property_names)
		{
			transition.id = property_name;
			transition_list.transitions.push_back(transition);
		}
	}

	property.value = std::move(transition_list);
	property.unit = Property::TRANSITION;

	return true;
}


bool PropertyParserAnimation::ParseValue(Property & property, const String & value, const ParameterMap & /*parameters*/) const
{
	StringList list_of_values;
	{
		auto lowercase_value = StringUtilities::ToLower(value);
		StringUtilities::ExpandString(list_of_values, lowercase_value, ',');
	}

	bool result = false;

	if (type == ANIMATION_PARSER)
	{
		result = ParseAnimation(property, list_of_values);
	}
	else if (type == TRANSITION_PARSER)
	{
		result = ParseTransition(property, list_of_values);
	}

	return result;
}

} // namespace Rml
