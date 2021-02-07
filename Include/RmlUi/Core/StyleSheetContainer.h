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
#include "StyleSheetTypes.h"

namespace Rml {

class Stream;
class StyleSheet;

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

	/// Compiles a single style sheet by combining all contained style sheets whose media queries match the provided parameters.
	/// @param[in] dp_ratio The current context ratio of 'dp' units to 'px' units
	/// @param[in] vp_dimensions The current context viewport dimensions
	/// @returns True when the compiled style sheet was changed, otherwise false.
	/// @warning This operation invalidates all references to the previously compiled style sheet.
	bool UpdateCompiledStyleSheet(float dp_ratio, Vector2f vp_dimensions);

	/// Returns the previously compiled style sheet. 
	StyleSheet* GetCompiledStyleSheet();

	/// Combines this style sheet container with another one, producing a new sheet container.
	SharedPtr<StyleSheetContainer> CombineStyleSheetContainer(const StyleSheetContainer& container) const;

	/// Merge another style sheet container into this.
	void MergeStyleSheetContainer(const StyleSheetContainer& container);

	/// Optimizes properties of the underlying style sheet(s) for faster retrieval.
	void OptimizeNodeProperties();

private:
	Vector<MediaBlock> media_blocks;

	StyleSheet* compiled_style_sheet = nullptr;
	UniquePtr<StyleSheet> combined_compiled_style_sheet;
	Vector<int> active_media_block_indices;
};

} // namespace Rml
#endif
