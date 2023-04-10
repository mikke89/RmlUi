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

#ifndef RMLUI_CORE_LAYOUT_INLINETYPES_H
#define RMLUI_CORE_LAYOUT_INLINETYPES_H

#include "../../../Include/RmlUi/Core/Types.h"

namespace Rml {

using LayoutOverflowHandle = int;
using LayoutFragmentHandle = int;

enum class InlineLayoutMode {
	WrapAny,          // Allow wrapping to avoid overflow, even if nothing is placed.
	WrapAfterContent, // Allow wrapping to avoid overflow, but first place at least *some* content on this line.
	Nowrap,           // Place all content on this line, regardless of overflow.
};

enum class FragmentType : byte {
	Invalid,   // Could not be placed.
	InlineBox, // An inline box.
	SizedBox,  // Sized inline-level boxes that are not inline-boxes.
	TextRun,   // Text runs.
};

struct FragmentConstructor {
	FragmentType type = FragmentType::Invalid;
	float layout_width = 0.f;
	LayoutFragmentHandle fragment_handle = {}; // Handle to enable the inline-level box to reference any fragment-specific data.
	LayoutOverflowHandle overflow_handle = {}; // Overflow handle is non-zero when there is another fragment to be layed out.
};

struct PlacedFragment {
	Element* offset_parent;
	LayoutFragmentHandle handle;
	Vector2f position;
	float layout_width;
	bool split_left;
	bool split_right;
};

} // namespace Rml
#endif
