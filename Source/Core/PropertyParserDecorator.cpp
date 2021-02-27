/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
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

#include "PropertyParserDecorator.h"
#include "../../Include/RmlUi/Core/DecoratorInstancer.h"
#include "../../Include/RmlUi/Core/Factory.h"
#include "../../Include/RmlUi/Core/PropertySpecification.h"
#include "../../Include/RmlUi/Core/Profiling.h"
#include "../../Include/RmlUi/Core/StyleSheetTypes.h"

namespace Rml {

PropertyParserDecorator::PropertyParserDecorator()
{}

PropertyParserDecorator::~PropertyParserDecorator()
{}

bool PropertyParserDecorator::ParseValue(Property& property, const String& decorator_string_value, const ParameterMap& /*parameters*/) const
{
	// Decorators are declared as
	//   decorator: <decorator-value>[, <decorator-value> ...];
	// Where <decorator-value> is either a @decorator name:
	//   decorator: invader-theme-background, ...;
	// or is an anonymous decorator with inline properties
	//   decorator: tiled-box( <shorthand properties> ), ...;

	if (decorator_string_value.empty() || decorator_string_value == "none")
	{
		property.value = Variant();
		property.unit = Property::UNKNOWN;
		return true;
	}

	RMLUI_ZoneScoped;

	DecoratorDeclarationList decorators;

	// Make sure we don't split inside the parenthesis since they may appear in decorator shorthands.
	StringList decorator_string_list;
	StringUtilities::ExpandString(decorator_string_list, decorator_string_value, ',', '(', ')');

	decorators.value = decorator_string_value;
	decorators.list.reserve(decorator_string_list.size());

	// Get or instance each decorator in the comma-separated string list
	for (const String& decorator_string : decorator_string_list)
	{
		const size_t shorthand_open = decorator_string.find('(');
		const size_t shorthand_close = decorator_string.rfind(')');
		const bool invalid_parenthesis = (shorthand_open == String::npos || shorthand_close == String::npos || shorthand_open >= shorthand_close);

		if (invalid_parenthesis)
		{
			// We found no parenthesis, that means the value must be a name of a @decorator rule.
			decorators.list.emplace_back(DecoratorDeclaration{ decorator_string, nullptr, {} });
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
				return false;
			}

			// Set unspecified values to their defaults
			specification.SetPropertyDefaults(properties);

			decorators.list.emplace_back(DecoratorDeclaration{ type, instancer, std::move(properties) });
		}
	}

	if (decorators.list.empty())
		return false;

	property.value = Variant(MakeShared<DecoratorDeclarationList>(std::move(decorators)));
	property.unit = Property::DECORATOR;

	return true;
}

} // namespace Rml
