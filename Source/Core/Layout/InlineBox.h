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

#ifndef RMLUI_CORE_LAYOUT_INLINEBOX_H
#define RMLUI_CORE_LAYOUT_INLINEBOX_H

#include "../../../Include/RmlUi/Core/Box.h"
#include "InlineLevelBox.h"

namespace Rml {

class InlineBoxBase : public InlineLevelBox {
public:
	InlineLevelBox* AddChild(UniquePtr<InlineLevelBox> child);

	String DebugDumpTree(int depth) const override;

protected:
	InlineBoxBase(Element* element);

	// Get the total height above and depth below the baseline based on this element's line-height and font.
	void GetStrut(float& out_total_height_above, float& out_total_depth_below) const;

private:
	using InlineLevelBoxList = Vector<UniquePtr<InlineLevelBox>>;

	// @performance Use first_child, next_sibling instead to build the tree?
	InlineLevelBoxList children;
};

/**
    Inline boxes are inline-level boxes whose contents (child boxes) participate in the same inline formatting context
    as the box itself.

    An inline box initially creates an unsized open fragment, since its width depends on its children. The fragment is
    sized and placed later on, either when its line needs to be split or when its element is closed, which happens after
    all its children in the element tree have already been placed.
 */
class InlineBox final : public InlineBoxBase {
public:
	InlineBox(const InlineLevelBox* parent, Element* element, const Box& box);

	FragmentConstructor CreateFragment(InlineLayoutMode mode, float available_width, float right_spacing_width, bool first_box,
		LayoutOverflowHandle overflow_handle) override;

	void Submit(const PlacedFragment& placed_fragment) override;

	String DebugDumpNameValue() const override { return "InlineBox"; }

private:
	float baseline_to_border_height = 0;
	Vector2f element_offset;
	Box box;
};

/**
    The root inline box is contained directly within its inline container.

    All content in the current inline formatting context is contained either within the root or one of its decendants.
 */
class InlineBoxRoot final : public InlineBoxBase {
public:
	InlineBoxRoot(Element* element);

	FragmentConstructor CreateFragment(InlineLayoutMode mode, float available_width, float right_spacing_width, bool first_box,
		LayoutOverflowHandle overflow_handle) override;

	void Submit(const PlacedFragment& placed_fragment) override;

	String DebugDumpNameValue() const override { return "InlineBoxRoot"; }
};

} // namespace Rml
#endif
