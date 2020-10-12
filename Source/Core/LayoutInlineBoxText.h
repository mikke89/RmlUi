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

#ifndef RMLUI_CORE_LAYOUTINLINEBOXTEXT_H
#define RMLUI_CORE_LAYOUTINLINEBOXTEXT_H

#include "LayoutInlineBox.h"

namespace Rml {

/**
	@author Peter Curry
 */

class LayoutInlineBoxText : public LayoutInlineBox
{
public:
	/// Constructs a new inline box for a text element.
	/// @param[in] element The text element this inline box is flowing.
	/// @param[in] line_begin The index of the first character of the element's string this text box will render.
	LayoutInlineBoxText(ElementText* element, int line_begin = 0);
	virtual ~LayoutInlineBoxText();

	/// Returns true if this box is capable of overflowing, or if it must be rendered on a single line.
	/// @return True if this box can overflow, false otherwise.
	bool CanOverflow() const override;

	/// Flows the inline box's content into its parent line.
	/// @param[in] first_box True if this box is the first box containing content to be flowed into this line.
	/// @param available_width[in] The width available for flowing this box's content. This is measured from the left side of this box's content area.
	/// @param right_spacing_width[in] The width of the spacing that must be left on the right of the element if no overflow occurs. If overflow occurs, then the entire width can be used.
	/// @return The overflow box containing any content that spilled over from the flow. This must be nullptr if no overflow occured.
	UniquePtr<LayoutInlineBox> FlowContent(bool first_box, float available_width, float right_spacing_width) override;

	/// Computes and sets the vertical position of this element, relative to its parent inline box (or block box,
	/// for an un-nested inline box).
	/// @param ascender[out] The maximum ascender of this inline box and all of its children.
	/// @param descender[out] The maximum descender of this inline box and all of its children.
	void CalculateBaseline(float& ascender, float& descender) override;
	/// Offsets the baseline of this box, and all of its children, by the ascender of the parent line box.
	/// @param ascender[in] The ascender of the line box.
	void OffsetBaseline(float ascender) override;

	/// Positions the inline box's element.
	void PositionElement() override;
	/// Sizes the inline box's element.
	void SizeElement(bool split) override;

	void* operator new(size_t size);
	void operator delete(void* chunk, size_t size);

private:
	/// Returns the box's element as a text element.
	/// @return The box's element cast to a text element.
	ElementText* GetTextElement();

	/// Builds a box for the first word of the element.
	void BuildWordBox();

	// The index of the first character of this line.
	int line_begin;
	// The contents on this line.
	String line_contents;

	// True if this line can be segmented into parts, false if it consists of only a single word.
	bool line_segmented;
};

} // namespace Rml
#endif
