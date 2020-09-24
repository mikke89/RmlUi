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

	// Retrieves the minimum and maximum width from an element's computed values.
	static void GetMinMaxWidth(float& min_width, float& max_width, const ComputedValues& computed, const Box& box, float containing_block_width);

	// Retrieves the minimum and maximum height from an element's computed values.
	static void GetMinMaxHeight(float& min_height, float& max_height, const ComputedValues& computed, const Box& box, float containing_block_height);

	// Retrieves the minimum and maximum height, set to the box's content height if it is definite (>= 0), otherwise retrieves the minimum and maximum heights from an element's computed values.
	static void GetDefiniteMinMaxHeight(float& min_height, float& max_height, const ComputedValues& computed, const Box& box, float containing_block_height);

	/// Returns the fully-resolved, fixed-width and -height containing block from a block box.
	/// @param[in] containing_box The leaf box.
	/// @return The dimensions of the content area, using the latest fixed dimensions for width and height in the hierarchy.
	static Vector2f GetContainingBlock(const LayoutBlockBox* containing_box);

	/// Builds margins of a Box, and resolves any auto width or height for non-inline elements. The height may be left unresolved if it depends on the element's children.
	/// @param[in,out] box The box to generate. The padding and borders must be set on the box already. The content area is used instead of the width and height properties, and -1 means auto width/height.
	/// @param[in] min_size The element's minimum width and height.
	/// @param[in] max_size The element's maximum width and height.
	/// @param[in] containing_block The size of the containing block.
	/// @param[in] element The element the box is being generated for.
	/// @param[in] inline_element True when the element is an inline element.
	/// @param[in] replaced_element True when the element is a replaced element.
	/// @param[in] override_shrink_to_fit_width Provide a fixed shrink-to-fit width instead of formatting the element when its properties allow shrinking.
	static void BuildBoxSizeAndMargins(Box& box, Vector2f min_size, Vector2f max_size, Vector2f containing_block, Element* element, bool inline_element, bool replaced_element, float override_shrink_to_fit_width = -1);

private:
	/// Formats the element and returns the width of its contents.
	static float GetShrinkToFitWidth(Element* element, Vector2f containing_block);

	/// Calculates and returns the content size for replaced elements.
	static Vector2f CalculateSizeForReplacedElement(Vector2f specified_content_size, Vector2f min_size, Vector2f max_size, Vector2f intrinsic_size, float intrinsic_ratio);

	/// Builds the block-specific width and horizontal margins of a Box.
	/// @param[in,out] box The box to generate. The padding and borders must be set on the box already. The content area is used instead of the width property, and -1 means auto width.
	/// @param[in] computed The computed values of the element the box is being generated for.
	/// @param[in] min_width The minimum content width of the element.
	/// @param[in] max_width The maximum content width of the element.
	/// @param[in] containing_block The size of the containing block.
	/// @param[in] element The element the box is being generated for.
	/// @param[in] replaced_element True when the element is a replaced element.
	/// @param[in] override_shrink_to_fit_width Provide a fixed shrink-to-fit width instead of formatting the element when its properties allow shrinking.
	static void BuildBoxWidth(Box& box, const ComputedValues& computed, float min_width, float max_width, Vector2f containing_block, Element* element, bool replaced_element, float override_shrink_to_fit_width = -1);
	/// Builds the block-specific height and vertical margins of a Box.
	/// @param[in,out] box The box to generate. The padding and borders must be set on the box already. The content area is used instead of the height property, and -1 means auto height.
	/// @param[in] computed The computed values of the element the box is being generated for.
	/// @param[in] min_height The minimum content height of the element.
	/// @param[in] max_height The maximum content height of the element.
	/// @param[in] containing_block_height The height of the containing block.
	static void BuildBoxHeight(Box& box, const ComputedValues& computed, float min_height, float max_height, float containing_block_height);
};

} // namespace Rml
#endif
