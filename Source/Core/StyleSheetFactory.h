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

#ifndef RMLUI_CORE_STYLESHEETFACTORY_H
#define RMLUI_CORE_STYLESHEETFACTORY_H

#include "../../Include/RmlUi/Core/Types.h"

namespace Rml {

class StyleSheetContainer;
enum class StructuralSelectorType;
struct StructuralSelector;

/**
    Creates stylesheets on the fly as needed. The factory keeps a cache of built sheets for optimisation.

    @author Lloyd Weehuizen
 */

class StyleSheetFactory {
public:
	~StyleSheetFactory();

	/// Initialise the style factory
	static bool Initialise();
	/// Shutdown style manager
	static void Shutdown();

	/// Gets the named sheet, retrieving it from the cache if its already been loaded.
	/// @param sheet name of sheet to load
	/// @lifetime Returned pointer is valid until the next call to ClearStyleSheetCache or Shutdown, it should not be stored around.
	static const StyleSheetContainer* GetStyleSheetContainer(const String& sheet);

	/// Clear the style sheet cache.
	static void ClearStyleSheetCache();

	/// Returns one of the available node selectors.
	/// @param name[in] The name of the desired selector.
	/// @return The selector registered with the given name, or nullptr if none exists.
	static StructuralSelector GetSelector(const String& name);

private:
	StyleSheetFactory();

	// Loads an individual style sheet
	UniquePtr<const StyleSheetContainer> LoadStyleSheetContainer(const String& sheet);

	// Individual loaded stylesheets
	using StyleSheets = UnorderedMap<String, UniquePtr<const StyleSheetContainer>>;
	StyleSheets stylesheets;

	// Custom complex selectors available for style sheets.
	using SelectorMap = UnorderedMap<String, StructuralSelectorType>;
	SelectorMap selectors;
};

} // namespace Rml
#endif
