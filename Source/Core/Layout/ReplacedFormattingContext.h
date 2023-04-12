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

#ifndef RMLUI_CORE_LAYOUT_REPLACEDFORMATTINGCONTEXT_H
#define RMLUI_CORE_LAYOUT_REPLACEDFORMATTINGCONTEXT_H

#include "../../../Include/RmlUi/Core/Types.h"
#include "FormattingContext.h"
#include "LayoutBox.h"

namespace Rml {

/*
    A formatting context that handles replaced elements.

    Replaced elements normally take care of their own layouting, so this is only responsible for setting thei box
    dimensions and notifying the element.
*/
class ReplacedFormattingContext final : public FormattingContext {
public:
	static UniquePtr<LayoutBox> Format(ContainerBox* parent_container, Element* element, const Box* override_initial_box);
};

class ReplacedBox : public LayoutBox {
public:
	ReplacedBox(Element* element) : LayoutBox(Type::Replaced), element(element) {}

	void Close();
	Box& GetBox() { return box; }

	const Box* GetIfBox() const override { return &box; }
	String DebugDumpTree(int depth) const override;

private:
	Element* element;
	Box box;
};

} // namespace Rml
#endif
