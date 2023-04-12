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

#ifndef RMLUI_CORE_SCROLLTYPES_H
#define RMLUI_CORE_SCROLLTYPES_H

namespace Rml {

enum class ScrollBehavior {
	Auto,    // Scroll using the context's configured setting.
	Smooth,  // Scroll using a smooth animation.
	Instant, // Scroll instantly.
};

enum class ScrollAlignment {
	Start,   // Align to the top or left edge of the parent element.
	Center,  // Align to the center of the parent element.
	End,     // Align to the bottom or right edge of the parent element.
	Nearest, // Align with minimal scroll change.
};

/**
    Defines behavior of Element::ScrollIntoView.
 */
struct ScrollIntoViewOptions {
	ScrollIntoViewOptions(ScrollAlignment vertical = ScrollAlignment::Start, ScrollAlignment horizontal = ScrollAlignment::Nearest,
		ScrollBehavior behavior = ScrollBehavior::Instant) :
		vertical(vertical),
		horizontal(horizontal), behavior(behavior)
	{}
	ScrollAlignment vertical;
	ScrollAlignment horizontal;
	ScrollBehavior behavior;
};

} // namespace Rml

#endif
