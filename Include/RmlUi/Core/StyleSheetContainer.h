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

#ifndef RMLUI_CORE_STYLESHEETCONTAINER_H
#define RMLUI_CORE_STYLESHEETCONTAINER_H

#include "Traits.h"
#include "PropertyDictionary.h"

namespace Rml {

class Stream;
class StyleSheet;

struct MediaBlock
{
	PropertyDictionary properties;
	UniquePtr<StyleSheet> stylesheet;

	MediaBlock() {}
	MediaBlock(PropertyDictionary _properties, UniquePtr<StyleSheet> _stylesheet) : properties(_properties), stylesheet(std::move(_stylesheet)) {}
};

/**
	StyleSheetContainer contains a list of media blocks and creates a combined style sheet when getting
	properties of the current context regarding the available media features.

	@author Maximilian Stark
 */

class RMLUICORE_API StyleSheetContainer : public NonCopyMoveable
{
public:
	StyleSheetContainer();
	virtual ~StyleSheetContainer();

	/// Loads a style from a CSS definition.
	bool LoadStyleSheetContainer(Stream* stream, int begin_line_number = 1);

	/// Returns the currently compiled style sheet that has been merged from incorporating all matching media blocks
	/// or creates it ad-hoc if the given properties differ from the currently stored values.
	/// @param[in] dimensions The current context viewport dimensions
	/// @param[in] density_ratio The current context ratio of 'dp' units to 'px' units
	StyleSheet* GetCompiledStyleSheet(Vector2i dimensions, float density_ratio);

	/// Combines this style sheet container with another one, producing a new sheet container.
	SharedPtr<StyleSheetContainer> CombineStyleSheetContainer(const StyleSheetContainer& container) const;

private:
	Vector<MediaBlock> media_blocks;

	UniquePtr<StyleSheet> compiled_style_sheet;

	Vector2i current_dimensions;
	float current_density_ratio;
};

} // namespace Rml
#endif
