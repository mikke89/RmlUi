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

#include "precompiled.h"
#include "StyleSheetFactory.h"
#include "../../Include/RmlUi/Core/StyleSheet.h"
#include "StreamFile.h"
#include "StyleSheetNodeSelectorNthChild.h"
#include "StyleSheetNodeSelectorNthLastChild.h"
#include "StyleSheetNodeSelectorNthOfType.h"
#include "StyleSheetNodeSelectorNthLastOfType.h"
#include "StyleSheetNodeSelectorFirstChild.h"
#include "StyleSheetNodeSelectorLastChild.h"
#include "StyleSheetNodeSelectorFirstOfType.h"
#include "StyleSheetNodeSelectorLastOfType.h"
#include "StyleSheetNodeSelectorOnlyChild.h"
#include "StyleSheetNodeSelectorOnlyOfType.h"
#include "StyleSheetNodeSelectorEmpty.h"
#include "../../Include/RmlUi/Core/Log.h"

namespace Rml {
namespace Core {

static StyleSheetFactory* instance = NULL;

StyleSheetFactory::StyleSheetFactory()
{
	RMLUI_ASSERT(instance == NULL);
	instance = this;
}

StyleSheetFactory::~StyleSheetFactory()
{
	instance = NULL;
}

bool StyleSheetFactory::Initialise()
{
	new StyleSheetFactory();

	instance->selectors["nth-child"] = new StyleSheetNodeSelectorNthChild();
	instance->selectors["nth-last-child"] = new StyleSheetNodeSelectorNthLastChild();
	instance->selectors["nth-of-type"] = new StyleSheetNodeSelectorNthOfType();
	instance->selectors["nth-last-of-type"] = new StyleSheetNodeSelectorNthLastOfType();
	instance->selectors["first-child"] = new StyleSheetNodeSelectorFirstChild();
	instance->selectors["last-child"] = new StyleSheetNodeSelectorLastChild();
	instance->selectors["first-of-type"] = new StyleSheetNodeSelectorFirstOfType();
	instance->selectors["last-of-type"] = new StyleSheetNodeSelectorLastOfType();
	instance->selectors["only-child"] = new StyleSheetNodeSelectorOnlyChild();
	instance->selectors["only-of-type"] = new StyleSheetNodeSelectorOnlyOfType();
	instance->selectors["empty"] = new StyleSheetNodeSelectorEmpty();

	return true;
}

void StyleSheetFactory::Shutdown()
{
	if (instance != NULL)
	{
		ClearStyleSheetCache();

		for (SelectorMap::iterator i = instance->selectors.begin(); i != instance->selectors.end(); ++i)
			delete (*i).second;

		delete instance;
	}
}

SharedPtr<StyleSheet> StyleSheetFactory::GetStyleSheet(const String& sheet_name)
{
	// Look up the sheet definition in the cache
	StyleSheets::iterator itr = instance->stylesheets.find(sheet_name);
	if (itr != instance->stylesheets.end())
	{
		return (*itr).second;
	}

	// Don't currently have the sheet, attempt to load it
	SharedPtr<StyleSheet> sheet = instance->LoadStyleSheet(sheet_name);
	if (!sheet)
		return nullptr;

	// Add it to the cache, and add a reference count so the cache will keep hold of it.
	instance->stylesheets[sheet_name] = sheet;

	return sheet;
}

SharedPtr<StyleSheet> StyleSheetFactory::GetStyleSheet(const StringList& sheets)
{
	// Generate a unique key for these sheets
	String combined_key;
	for (size_t i = 0; i < sheets.size(); i++)
	{		
		URL path(sheets[i]);
		combined_key += path.GetFileName();
	}

	// Look up the sheet definition in the cache.
	StyleSheets::iterator itr = instance->stylesheet_cache.find(combined_key);
	if (itr != instance->stylesheet_cache.end())
	{
		return (*itr).second;
	}

	// Load and combine the sheets.
	SharedPtr<StyleSheet> sheet;
	for (size_t i = 0; i < sheets.size(); i++)
	{
		SharedPtr<StyleSheet> sub_sheet = GetStyleSheet(sheets[i]);
		if (sub_sheet)
		{
			if (sheet)
			{
				SharedPtr<StyleSheet> new_sheet = sheet->CombineStyleSheet(*sub_sheet);
				sheet = new_sheet;
			}
			else
				sheet = sub_sheet;
		}
		else
			Log::Message(Log::LT_ERROR, "Failed to load style sheet %s.", sheets[i].c_str());
	}

	if (!sheet)
		return nullptr;

	// Add to cache, and a reference to the sheet to hold it in the cache.
	instance->stylesheet_cache[combined_key] = sheet;
	return sheet;
}

// Clear the style sheet cache.
void StyleSheetFactory::ClearStyleSheetCache()
{
	instance->stylesheets.clear();
	instance->stylesheet_cache.clear();
}

// Returns one of the available node selectors.
StyleSheetNodeSelector* StyleSheetFactory::GetSelector(const String& name)
{
	size_t index = name.find("(");
	SelectorMap::iterator i = instance->selectors.find(name.substr(0, index));
	if (i == instance->selectors.end())
		return NULL;
	return (*i).second;
}

SharedPtr<StyleSheet> StyleSheetFactory::LoadStyleSheet(const String& sheet)
{
	SharedPtr<StyleSheet> new_style_sheet;

	// Open stream, construct new sheet and pass the stream into the sheet
	// TODO: Make this support ASYNC
	auto stream = std::make_unique<StreamFile>();
	if (stream->Open(sheet))
	{
		new_style_sheet = std::make_shared<StyleSheet>();
		if (!new_style_sheet->LoadStyleSheet(stream.get()))
		{
			new_style_sheet = nullptr;
		}
	}
	return new_style_sheet;
}

}
}
