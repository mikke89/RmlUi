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

#include "PropertyParserDecorator.h"
#include "../../Include/RmlUi/Core/Decorator.h"
#include "../../Include/RmlUi/Core/Factory.h"
#include "../../Include/RmlUi/Core/Profiling.h"
#include "../../Include/RmlUi/Core/PropertySpecification.h"
#include "../../Include/RmlUi/Core/StyleSheetTypes.h"

namespace Rml {

const SmallUnorderedMap<String, BoxArea> PropertyParserDecorator::area_keywords = {
	{"border-box", BoxArea::Border},
	{"padding-box", BoxArea::Padding},
	{"content-box", BoxArea::Content},
};

PropertyParserDecorator::PropertyParserDecorator() {}

PropertyParserDecorator::~PropertyParserDecorator() {}

bool PropertyParserDecorator::ParseValue(Property& property, const String& decorator_string_value, const ParameterMap& /*parameters*/) const
{
	// Decorators are declared as
	//   decorator: <decorator-value>[, <decorator-value> ...];
	// Where <decorator-value> is either a @decorator name:
	//   decorator: invader-theme-background <paint-area>?, ...;
	// or is an anonymous decorator with inline properties
	//   decorator: tiled-box( <shorthand properties> ) <paint-area>?, ...;
	// where <paint-area> is one of
	//   border-box, padding-box, content-box

	if (decorator_string_value.empty() || decorator_string_value == "none")
	{
		property.value = Variant(DecoratorsPtr());
		property.unit = Unit::DECORATOR;
		return true;
	}

	RMLUI_ZoneScoped;

	// Make sure we don't split inside the parenthesis since they may appear in decorator shorthands.
	StringList decorator_string_list;
	StringUtilities::ExpandString(decorator_string_list, decorator_string_value, ',', '(', ')');

	DecoratorDeclarationList decorators;
	decorators.value = decorator_string_value;
	decorators.list.reserve(decorator_string_list.size());

	// Get or instance each decorator in the comma-separated string list
	for (const String& decorator_string : decorator_string_list)
	{
		const size_t shorthand_open = decorator_string.find('(');
		const size_t shorthand_close = decorator_string.rfind(')');
		const bool invalid_parenthesis = (shorthand_open == String::npos || shorthand_close == String::npos || shorthand_open >= shorthand_close);
		const size_t keywords_begin = (invalid_parenthesis ? decorator_string.find(' ') : shorthand_close + 1);

		// Look-up keywords for customized paint area.
		BoxArea paint_area = BoxArea::Auto;

		{
			StringList keywords;
			if (keywords_begin < decorator_string.size())
				StringUtilities::ExpandString(keywords, decorator_string.substr(keywords_begin), ' ');

			for (const String& keyword : keywords)
			{
				if (keyword.empty())
					continue;

				auto it = area_keywords.find(StringUtilities::ToLower(keyword));
				if (it == area_keywords.end())
					return false; // Bail out if we have an invalid keyword.

				paint_area = it->second;
			}
		}

		if (invalid_parenthesis)
		{
			// We found no parenthesis, that means the value must be a name of a @decorator rule.
			decorators.list.emplace_back(DecoratorDeclaration{decorator_string.substr(0, keywords_begin), nullptr, {}, paint_area});
		}
		else
		{
			// Since we have parentheses it must be an anonymous decorator with inline properties
			const String type = StringUtilities::StripWhitespace(decorator_string.substr(0, shorthand_open));

			// Check for valid decorator type
			DecoratorInstancer* instancer = Factory::GetDecoratorInstancer(type);
			if (!instancer)
			{
				Log::Message(Log::LT_WARNING, "Decorator type '%s' not found.", type.c_str());
				return false;
			}

			const String shorthand = decorator_string.substr(shorthand_open + 1, shorthand_close - shorthand_open - 1);
			const PropertySpecification& specification = instancer->GetPropertySpecification();

			// Parse the shorthand properties given by the 'decorator' shorthand property
			PropertyDictionary properties;
			if (!specification.ParsePropertyDeclaration(properties, "decorator", shorthand))
			{
				// Empty values are allowed in decorators, if the value is not empty we must have encountered a parser error.
				if (!StringUtilities::StripWhitespace(shorthand).empty())
					return false;
			}

			// Set unspecified values to their defaults
			specification.SetPropertyDefaults(properties);

			decorators.list.emplace_back(DecoratorDeclaration{type, instancer, std::move(properties), paint_area});
		}
	}

	if (decorators.list.empty())
		return false;

	property.value = Variant(MakeShared<DecoratorDeclarationList>(std::move(decorators)));
	property.unit = Unit::DECORATOR;

	return true;
}

String PropertyParserDecorator::ConvertAreaToString(BoxArea area)
{
	for (const auto& it : area_keywords)
	{
		if (it.second == area)
			return it.first;
	}
	return String();
}

} // namespace Rml
