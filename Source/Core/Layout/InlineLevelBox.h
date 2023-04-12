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

#ifndef RMLUI_CORE_LAYOUT_INLINELEVELBOX_H
#define RMLUI_CORE_LAYOUT_INLINELEVELBOX_H

#include "../../../Include/RmlUi/Core/Box.h"
#include "../../../Include/RmlUi/Core/StyleTypes.h"
#include "InlineTypes.h"

namespace Rml {

class ElementText;
struct FontMetrics;

/**
    A box that takes part in inline layout.

    The inline-level box is used to generate fragments that are placed within line boxes.
 */
class InlineLevelBox {
public:
	virtual ~InlineLevelBox();

	// Create a fragment from this box, if it can fit within the available width.
	virtual FragmentConstructor CreateFragment(InlineLayoutMode mode, float available_width, float right_spacing_width, bool first_box,
		LayoutOverflowHandle overflow_handle) = 0;

	// Submit a fragment's position and size to be displayed on the underlying element.
	virtual void Submit(const PlacedFragment& placed_fragment) = 0;

	float GetHeightAboveBaseline() const { return height_above_baseline; }
	float GetDepthBelowBaseline() const { return depth_below_baseline; }
	Style::VerticalAlign::Type GetVerticalAlign() const { return vertical_align_type; }
	float GetVerticalOffsetFromParent() const { return vertical_offset_from_parent; }
	float GetSpacingLeft() const { return spacing_left; }
	float GetSpacingRight() const { return spacing_right; }

	virtual String DebugDumpNameValue() const = 0;
	virtual String DebugDumpTree(int depth) const;

	void* operator new(size_t size);
	void operator delete(void* chunk, size_t size);

protected:
	InlineLevelBox(Element* element) : element(element) { RMLUI_ASSERT(element); }

	Element* GetElement() const { return element; }
	const FontMetrics& GetFontMetrics() const;

	// Set the height used for inline layout, and the vertical offset relative to our parent box.
	void SetHeightAndVerticalAlignment(float height_above_baseline, float depth_below_baseline, const InlineLevelBox* parent);

	// Set the height used for inline layout.
	void SetHeight(float height_above_baseline, float depth_below_baseline);

	// Set the inner-to-outer spacing (margin + border + padding) for inline boxes.
	void SetInlineBoxSpacing(float spacing_left, float spacing_right);

	// Calls Element::OnLayout (proxy for private access to Element).
	void SubmitElementOnLayout();

private:
	float height_above_baseline = 0.f;
	float depth_below_baseline = 0.f;

	Style::VerticalAlign::Type vertical_align_type = {};
	float vertical_offset_from_parent = 0.f;

	float spacing_left = 0.f;  // Left margin-border-padding for inline boxes.
	float spacing_right = 0.f; // Right margin-border-padding for inline boxes.

	Element* element;
};

/**
    Atomic inline-level boxes are sized boxes that cannot be split.

    This includes inline-block elements, replaced inline-level elements, inline tables, and inline flex containers.
 */
class InlineLevelBox_Atomic final : public InlineLevelBox {
public:
	InlineLevelBox_Atomic(const InlineLevelBox* parent, Element* element, const Box& box);

	FragmentConstructor CreateFragment(InlineLayoutMode mode, float available_width, float right_spacing_width, bool first_box,
		LayoutOverflowHandle overflow_handle) override;

	void Submit(const PlacedFragment& placed_fragment) override;

	String DebugDumpNameValue() const override { return "InlineLevelBox_Atomic"; }

private:
	Box box;
};

/**
    Inline-level text boxes represent text nodes.

    Generates fragments to display its text, splitting it up as necessary to fit in the available space.
 */
class InlineLevelBox_Text final : public InlineLevelBox {
public:
	InlineLevelBox_Text(ElementText* element);

	FragmentConstructor CreateFragment(InlineLayoutMode mode, float available_width, float right_spacing_width, bool first_box,
		LayoutOverflowHandle overflow_handle) override;

	void Submit(const PlacedFragment& placed_fragment) override;

	String DebugDumpNameValue() const override;

private:
	ElementText* GetTextElement();

	Vector2f element_offset;
	StringList fragments;
};

} // namespace Rml
#endif
