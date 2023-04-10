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

#ifndef RMLUI_CORE_LAYOUT_BLOCKFORMATTINGCONTEXT_H
#define RMLUI_CORE_LAYOUT_BLOCKFORMATTINGCONTEXT_H

#include "../../../Include/RmlUi/Core/Types.h"
#include "FormattingContext.h"

namespace Rml {

class Box;
class BlockContainer;
class ContainerBox;
class LayoutBox;

/*
    Places boxes according to normal flow, while handling floated boxes.

    A block formatting context (BFC) starts with a root BlockContainer, where child block-level boxes are placed
    vertically. Descendant elements take part in the same BFC, unless the element has properties causing it to establish
    an independent formatting context.

    Floated boxes are taken out-of-flow and placed inside the current BFC. Floats are only affected by, and affect only,
    other boxes within the same BFC.
*/
class BlockFormattingContext final : public FormattingContext {
public:
	static UniquePtr<LayoutBox> Format(ContainerBox* parent_container, Element* element, const Box* override_initial_box);

private:
	// Format the element as a block box, including its children.
	// @return False if the box caused an automatic vertical scrollbar to appear in the block formatting context root, forcing it to be reformatted.
	static bool FormatBlockBox(BlockContainer* parent_container, Element* element);

	// Format the element as an inline box, including its children.
	// @return False if the box caused an automatic vertical scrollbar to appear in the block formatting context root, forcing it to be reformatted.
	static bool FormatInlineBox(BlockContainer* parent_container, Element* element);

	// Determine how to format a child element of a block container, and format it accordingly, possibly including any children.
	// @return False if the box caused an automatic vertical scrollbar to appear in the block formatting context root, forcing it to be reformatted.
	static bool FormatBlockContainerChild(BlockContainer* parent_container, Element* element);
};

} // namespace Rml
#endif
