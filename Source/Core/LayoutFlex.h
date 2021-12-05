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

#ifndef RMLUI_CORE_LAYOUTFLEX_H
#define RMLUI_CORE_LAYOUTFLEX_H

#include "../../Include/RmlUi/Core/Types.h"

namespace Rml {

class Box;

class LayoutFlex {
public:
	/// Formats a flexible box, including all elements contained within.
	/// @param[in] box The box used for dimensioning the flex container.
	/// @param[in] min_size Minimum width and height of the flexbox.
	/// @param[in] max_size Maximum width and height of the flexbox.
	/// @param[in] containing_block Flexbox's containing block size.
	/// @param[in] element_flex The flex container element.
	/// @param[out] out_formatted_content_size The flex container element's used size.
	/// @param[out] out_content_overflow_size  The content size of the flexbox's overflowing content.
	/// @param[out] out_absolutely_positioned_elements List of absolutely positioned elements within the flexbox.
	static void Format(const Box& box, Vector2f min_size, Vector2f max_size, Vector2f containing_block, Element* element_flex,
		Vector2f& out_formatted_content_size, Vector2f& out_content_overflow_size, ElementList& out_absolutely_positioned_elements);

private:
	LayoutFlex(Element* element_flex, Vector2f flex_available_content_size, Vector2f flex_content_containing_block, Vector2f flex_content_offset,
		Vector2f flex_min_size, Vector2f flex_max_size, ElementList& absolutely_positioned_elements);

	// Format the flexbox.
	void Format();

	Element* const element_flex;

	const Vector2f flex_available_content_size;
	const Vector2f flex_content_containing_block;
	const Vector2f flex_content_offset;
	const Vector2f flex_min_size;
	const Vector2f flex_max_size;

	// The final size of the table which will be determined by the size of its columns, rows, and spacing.
	Vector2f flex_resulting_content_size;
	// Overflow size in case flex items overflow the container or contents of any flex items overflow their box (without being caught by the item).
	Vector2f flex_content_overflow_size;

	ElementList& absolutely_positioned_elements;
};

} // namespace Rml
#endif
