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

#include "PropertyParserFontEffect.h"
#include "../../Include/RmlUi/Core/Factory.h"
#include "../../Include/RmlUi/Core/FontEffect.h"
#include "../../Include/RmlUi/Core/FontEffectInstancer.h"
#include "../../Include/RmlUi/Core/Profiling.h"
#include "../../Include/RmlUi/Core/PropertySpecification.h"
#include "../../Include/RmlUi/Core/Utilities.h"
#include <algorithm>

namespace Rml {

PropertyParserFontEffect::PropertyParserFontEffect() {}

PropertyParserFontEffect::~PropertyParserFontEffect() {}

bool PropertyParserFontEffect::ParseValue(Property& property, const String& font_effect_string_value, const ParameterMap& /*parameters*/) const
{
	// Font-effects are declared as
	//   font-effect: <font-effect-value>[, <font-effect-value> ...];
	// Where <font-effect-value> is declared with inline properties, e.g.
	//   font-effect: outline( 1px black ), ...;

	if (font_effect_string_value.empty() || font_effect_string_value == "none")
	{
		property.value = Variant();
		property.unit = Unit::UNKNOWN;
		return true;
	}

	RMLUI_ZoneScoped;

	FontEffects font_effects;

	// Make sure we don't split inside the parenthesis since they may appear in decorator shorthands.
	StringList font_effect_string_list;
	StringUtilities::ExpandString(font_effect_string_list, font_effect_string_value, ',', '(', ')');

	font_effects.value = font_effect_string_value;
	font_effects.list.reserve(font_effect_string_list.size());

	// Get or instance each decorator in the comma-separated string list
	for (const String& font_effect_string : font_effect_string_list)
	{
		const size_t shorthand_open = font_effect_string.find('(');
		const size_t shorthand_close = font_effect_string.rfind(')');
		const bool invalid_parenthesis = (shorthand_open == String::npos || shorthand_close == String::npos || shorthand_open >= shorthand_close);

		if (invalid_parenthesis)
		{
			// We found no parenthesis, font-effects can only be declared anonymously for now.
			Log::Message(Log::LT_WARNING, "Invalid syntax for font-effect '%s'.", font_effect_string.c_str());
			return false;
		}
		else
		{
			// Since we have parentheses it must be an anonymous decorator with inline properties
			const String type = StringUtilities::StripWhitespace(font_effect_string.substr(0, shorthand_open));

			// Check for valid font-effect type
			FontEffectInstancer* instancer = Factory::GetFontEffectInstancer(type);
			if (!instancer)
			{
				Log::Message(Log::LT_WARNING, "Font-effect type '%s' not found.", type.c_str());
				return false;
			}

			const String shorthand = font_effect_string.substr(shorthand_open + 1, shorthand_close - shorthand_open - 1);
			const PropertySpecification& specification = instancer->GetPropertySpecification();

			// Parse the shorthand properties given by the 'font-effect' shorthand property
			PropertyDictionary properties;
			if (!specification.ParsePropertyDeclaration(properties, "font-effect", shorthand))
			{
				// Empty values are allowed in font-effects, if the value is not empty we must have encountered a parser error.
				if (!StringUtilities::StripWhitespace(shorthand).empty())
				{
					Log::Message(Log::LT_WARNING, "Could not parse font-effect value '%s'.", font_effect_string.c_str());
					return false;
				}
			}

			// Set unspecified values to their defaults
			specification.SetPropertyDefaults(properties);

			RMLUI_ZoneScopedN("InstanceFontEffect");
			SharedPtr<FontEffect> font_effect = instancer->InstanceFontEffect(type, properties);
			if (font_effect)
			{
				// Create a unique hash value for the given type and values
				size_t fingerprint = Hash<String>{}(type);
				for (const auto& id_value : properties.GetProperties())
					Utilities::HashCombine(fingerprint, id_value.second.Get<String>());

				font_effect->SetFingerprint(fingerprint);

				font_effects.list.emplace_back(std::move(font_effect));
			}
			else
			{
				Log::Message(Log::LT_WARNING, "Font-effect '%s' could not be instanced.", font_effect_string.c_str());
				return false;
			}
		}
	}

	if (font_effects.list.empty())
		return false;

	// Partition the list such that the back layer effects appear before the front layer effects
	std::stable_partition(font_effects.list.begin(), font_effects.list.end(),
		[](const SharedPtr<const FontEffect>& effect) { return effect->GetLayer() == FontEffect::Layer::Back; });

	property.value = Variant(MakeShared<FontEffects>(std::move(font_effects)));
	property.unit = Unit::FONTEFFECT;

	return true;
}

} // namespace Rml
