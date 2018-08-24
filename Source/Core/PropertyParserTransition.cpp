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
#include "PropertyParserTransition.h"
#include "../../Include/Rocket/Core/StringUtilities.h"
#include "PropertyShorthandDefinition.h"


namespace Rocket {
namespace Core {


struct TransitionSpec {
	enum Type { KEYWORD_NONE, KEYWORD_ALL, TWEEN } type;
	Tween tween;
	TransitionSpec(Tween tween) : type(TWEEN), tween(tween) {}
	TransitionSpec(Type type) : type(type) {}
};


static const std::unordered_map<String, TransitionSpec> transition_spec = {
		{"none", {TransitionSpec::KEYWORD_NONE} },
		{"all", {TransitionSpec::KEYWORD_ALL}},

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




PropertyParserTransition::PropertyParserTransition()
{
}


bool PropertyParserTransition::ParseValue(Property & property, const String & value, const ParameterMap & parameters) const
{
	StringList list_of_properties;
	{
		auto lowercase_value = value.ToLower();
		StringUtilities::ExpandString(list_of_properties, lowercase_value, ',');
	}

	TransitionList transition_list{ false, false, {} };

	for (const String& single_property : list_of_properties)
	{
		Transition transition;
		StringList target_property_names;

		StringList arguments;
		StringUtilities::ExpandString(arguments, single_property, ' ');


		bool duration_found = false;
		bool delay_found = false;
		bool reverse_adjustment_factor_found = false;

		for (auto& argument : arguments)
		{
			if (argument.Empty())
				continue;

			// See if we have a <keyword> or <tween> specifier as defined in transition_spec
			if (auto it = transition_spec.find(argument); it != transition_spec.end())
			{
				if (it->second.type == TransitionSpec::KEYWORD_NONE)
				{
					if (transition_list.transitions.size() > 0) // The none keyword can not be part of multiple definitions
						return false;
					property = Property{ TransitionList{true, false, {}}, Property::TRANSITION };
					return true;
				}
				else if (it->second.type == TransitionSpec::KEYWORD_ALL)
				{
					if (transition_list.transitions.size() > 0) // The all keyword can not be part of multiple definitions
						return false;
					transition_list.all = true;
					target_property_names.push_back("all");
				}
				else if (it->second.type == TransitionSpec::TWEEN)
				{
					transition.tween = it->second.tween;
				}
			}
			else
			{
				// Either <duration>, <delay> or a <property name>
				float number = 0.0f;
				int count = 0;

				if (sscanf(argument.CString(), "%fs%n", &number, &count) == 1)
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
						// For shorthands, add each underlying property separately
						for (const auto& property : shorthand->properties)
							target_property_names.push_back(property.first);
					}
					else if (auto definition = StyleSheetSpecification::GetProperty(argument))
					{
						// Single property
						target_property_names.push_back(argument);
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
		if (target_property_names.empty() || transition.duration <= 0.0f || transition.reverse_adjustment_factor < 0.0f || transition.reverse_adjustment_factor > 1.0f
			|| (transition_list.all && target_property_names.size() != 1))
		{
			return false;
		}

		for (const auto& property_name : target_property_names)
		{
			transition.name = property_name;
			transition_list.transitions.push_back(transition);
		}
	}

	property = Property{ transition_list, Property::TRANSITION };

	return true;
}

void PropertyParserTransition::Release()
{
	delete this;
}

}
}