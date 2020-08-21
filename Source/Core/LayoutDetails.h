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

#ifndef RMLUI_CORE_LAYOUTDETAILS_H
#define RMLUI_CORE_LAYOUTDETAILS_H

#include "LayoutBlockBox.h"

namespace Rml {

class Box;

/**
	Layout functions for sizing elements.
	
	Corresponds to the CSS 2.1 specification, 'Section 10. Visual formatting model details'.
 */

class LayoutDetails
{
public:
	/// Generates the box for an element.
	/// @param[out] box The box to be built.
	/// @param[in] containing_block The dimensions of the content area of the block containing the element.
	/// @param[in] element The element to build the box for.
	/// @param[in] inline_element True if the element is placed in an inline context, false if not.
	/// @param[in] override_shrink_to_fit_width Provide a fixed shrink-to-fit width instead of formatting the element when its properties allow shrinking.
	static void BuildBox(Box& box, Vector2f containing_block, Element* element, bool inline_element = false, float override_shrink_to_fit_width = -1);
	/// Generates the box for an element placed in a block box.
	/// @param[out] box The box to be built.
	/// @param[out] min_height The minimum height of the element's box.
	/// @param[out] max_height The maximum height of the element's box.
	/// @param[in] containing_box The block box containing the element.
	/// @param[in] element The element to build the box for.
	/// @param[in] inline_element True if the element is placed in an inline context, false if not.
	/// @param[in] override_shrink_to_fit_width Provide a fixed shrink-to-fit width instead of formatting the element when its properties allow shrinking.
	static void BuildBox(Box& box, float& min_height, float& max_height, LayoutBlockBox* containing_box, Element* element, bool inline_element, float override_shrink_to_fit_width = -1);

	/// Clamps the width of an element based from its min-width and max-width properties.
	/// @param[in] width The width to clamp.
	/// @param[in] element The element to read the properties from.
	/// @param[in] containing_block_width The width of the element's containing block.
	/// @return The clamped width.
	static float ClampWidth(float width, const ComputedValues& computed, const Box& box, float containing_block_width);
	/// Clamps the height of an element based from its min-height and max-height properties.
	/// @param[in] height The height to clamp.
	/// @param[in] element The element to read the properties from.
	/// @param[in] containing_block_height The height of the element's containing block.
	/// @return The clamped height.
	static float ClampHeight(float height, const ComputedValues& computed, const Box& box, float containing_block_height);

	/// Returns the fully-resolved, fixed-width and -height containing block from a block box.
	/// @param[in] containing_box The leaf box.
	/// @return The dimensions of the content area, using the latest fixed dimensions for width and height in the hierarchy.
	static Vector2f GetContainingBlock(const LayoutBlockBox* containing_box);

private:
	/// Formats the element and returns the width of its contents.
	static float GetShrinkToFitWidth(Element* element, Vector2f containing_block);

	/// Builds the block-specific width and horizontal margins of a Box.
	/// @param[in,out] box The box to generate. The padding and borders must be set on the box already. If the content area is sized, then it will be used instead of the width property.
	/// @param[in] element The element the box is being generated for.
	/// @param[in] containing_block_width The width of the containing block.
	/// @param[in] replaced_element True when the element is a replaced element.
	/// @param[in] override_shrink_to_fit_width Provide a fixed shrink-to-fit width instead of formatting the element when its properties allow shrinking.
	static void BuildBoxWidth(Box& box, const ComputedValues& computed, Vector2f containing_block_width, Element* element, bool replaced_element, float override_shrink_to_fit_width = -1);
	/// Builds the block-specific height and vertical margins of a Box.
	/// @param[in,out] box The box to generate. The padding and borders must be set on the box already. If the content area is sized, then it will be used instead of the height property.
	/// @param[in] element The element the box is being generated for.
	/// @param[in] containing_block_height The height of the containing block.
	static void BuildBoxHeight(Box& box, const ComputedValues& computed, float containing_block_height);
};

} // namespace Rml
#endif
