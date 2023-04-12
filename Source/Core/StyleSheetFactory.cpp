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

#include "StyleSheetFactory.h"
#include "../../Include/RmlUi/Core/Log.h"
#include "../../Include/RmlUi/Core/StyleSheetContainer.h"
#include "StreamFile.h"
#include "StyleSheetNode.h"
#include "StyleSheetParser.h"
#include "StyleSheetSelector.h"

namespace Rml {

static UniquePtr<StyleSheetFactory> instance;

StyleSheetFactory::StyleSheetFactory() :
	selectors{
		{"nth-child", StructuralSelectorType::Nth_Child},
		{"nth-last-child", StructuralSelectorType::Nth_Last_Child},
		{"nth-of-type", StructuralSelectorType::Nth_Of_Type},
		{"nth-last-of-type", StructuralSelectorType::Nth_Last_Of_Type},
		{"first-child", StructuralSelectorType::First_Child},
		{"last-child", StructuralSelectorType::Last_Child},
		{"first-of-type", StructuralSelectorType::First_Of_Type},
		{"last-of-type", StructuralSelectorType::Last_Of_Type},
		{"only-child", StructuralSelectorType::Only_Child},
		{"only-of-type", StructuralSelectorType::Only_Of_Type},
		{"empty", StructuralSelectorType::Empty},
		{"not", StructuralSelectorType::Not},
	}
{}

StyleSheetFactory::~StyleSheetFactory() {}

bool StyleSheetFactory::Initialise()
{
	RMLUI_ASSERT(instance == nullptr);
	instance = UniquePtr<StyleSheetFactory>(new StyleSheetFactory);
	return true;
}

void StyleSheetFactory::Shutdown()
{
	instance.reset();
}

const StyleSheetContainer* StyleSheetFactory::GetStyleSheetContainer(const String& sheet_name)
{
	// Look up the sheet definition in the cache
	auto it = instance->stylesheets.find(sheet_name);
	if (it != instance->stylesheets.end())
		return it->second.get();

	// Don't currently have the sheet, attempt to load it
	UniquePtr<const StyleSheetContainer> sheet = instance->LoadStyleSheetContainer(sheet_name);
	if (!sheet)
		return nullptr;

	const StyleSheetContainer* result = sheet.get();

	// Add it to the cache.
	instance->stylesheets[sheet_name] = std::move(sheet);

	return result;
}

void StyleSheetFactory::ClearStyleSheetCache()
{
	instance->stylesheets.clear();
}

StructuralSelector StyleSheetFactory::GetSelector(const String& name)
{
	SelectorMap::const_iterator it;
	const size_t parameter_start = name.find('(');

	if (parameter_start == String::npos)
		it = instance->selectors.find(name);
	else
		it = instance->selectors.find(name.substr(0, parameter_start));

	if (it == instance->selectors.end())
		return StructuralSelector(StructuralSelectorType::Invalid, 0, 0);

	const StructuralSelectorType selector_type = it->second;

	bool requires_parameter = false;
	switch (selector_type)
	{
	case StructuralSelectorType::Nth_Child:
	case StructuralSelectorType::Nth_Last_Child:
	case StructuralSelectorType::Nth_Of_Type:
	case StructuralSelectorType::Nth_Last_Of_Type:
	case StructuralSelectorType::Not: requires_parameter = true; break;
	default: break;
	}

	const size_t parameter_end = name.rfind(')');
	const bool has_parameter = (parameter_start != String::npos && parameter_end != String::npos && parameter_start < parameter_end);

	if (requires_parameter != has_parameter)
	{
		Log::Message(Log::LT_WARNING, "Invalid selector ':%s' encountered, expected %s parameters", name.c_str(),
			requires_parameter ? "parenthesized" : "no");
		return StructuralSelector(StructuralSelectorType::Invalid, 0, 0);
	}

	// Parse the 'a' and 'b' values.
	int a = 1;
	int b = 0;

	if (has_parameter)
	{
		const String parameters = StringUtilities::StripWhitespace(StringView(name, parameter_start + 1, parameter_end - (parameter_start + 1)));

		if (selector_type == StructuralSelectorType::Not)
		{
			auto list = MakeShared<SelectorTree>();
			list->root = MakeUnique<StyleSheetNode>();
			list->leafs = StyleSheetParser::ConstructNodes(*list->root, parameters);

			int specificity = 0;
			for (const StyleSheetNode* node : list->leafs)
				specificity = Math::Max(specificity, node->GetSpecificity());

			return StructuralSelector(selector_type, std::move(list), specificity);
		}

		// Check for 'even' or 'odd' first.
		if (parameters == "even")
		{
			a = 2;
			b = 0;
		}
		else if (parameters == "odd")
		{
			a = 2;
			b = 1;
		}
		else
		{
			// Alrighty; we've got an equation in the form of [[+/-]an][(+/-)b]. So, foist up, we split on 'n'.
			const size_t n_index = parameters.find('n');
			if (n_index == String::npos)
			{
				// The equation is 0n + b. So a = 0, and we only have to parse b.
				a = 0;
				b = atoi(parameters.c_str());
			}
			else
			{
				if (n_index == 0)
					a = 1;
				else
				{
					const String a_parameter = parameters.substr(0, n_index);
					if (StringUtilities::StripWhitespace(a_parameter) == "-")
						a = -1;
					else
						a = atoi(a_parameter.c_str());
				}

				size_t pm_index = parameters.find('+', n_index + 1);
				if (pm_index != String::npos)
					b = 1;
				else
				{
					pm_index = parameters.find('-', n_index + 1);
					if (pm_index != String::npos)
						b = -1;
				}

				if (n_index == parameters.size() - 1 || pm_index == String::npos)
					b = 0;
				else
					b = b * atoi(parameters.data() + pm_index + 1);
			}
		}
	}

	return StructuralSelector(selector_type, a, b);
}

UniquePtr<const StyleSheetContainer> StyleSheetFactory::LoadStyleSheetContainer(const String& sheet)
{
	UniquePtr<StyleSheetContainer> new_style_sheet;

	// Open stream, construct new sheet and pass the stream into the sheet
	auto stream = MakeUnique<StreamFile>();
	if (stream->Open(sheet))
	{
		new_style_sheet = MakeUnique<StyleSheetContainer>();
		if (!new_style_sheet->LoadStyleSheetContainer(stream.get()))
		{
			new_style_sheet.reset();
		}
	}

	return new_style_sheet;
}

} // namespace Rml
