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

#ifndef RMLUI_CORE_LAYOUT_LAYOUTBOX_H
#define RMLUI_CORE_LAYOUT_LAYOUTBOX_H

#include "../../../Include/RmlUi/Core/Box.h"
#include "../../../Include/RmlUi/Core/Types.h"

namespace Rml {

/*
    A box used to represent the formatting structure of the document, taking part in the box tree.
*/
class LayoutBox {
public:
	enum class Type { Root, BlockContainer, InlineContainer, FlexContainer, TableWrapper, Replaced };

	virtual ~LayoutBox() = default;

	Type GetType() const { return type; }
	Vector2f GetVisibleOverflowSize() const { return visible_overflow_size; }

	// Returns a pointer to the dimensions box if this layout box has one.
	virtual const Box* GetIfBox() const;
	// Returns the baseline of the last line of this box, if any. Returns true if a baseline was found, otherwise false.
	virtual bool GetBaselineOfLastLine(float& out_baseline) const;
	// Calculates the box's inner content width, i.e. the size used to calculate the shrink-to-fit width.
	virtual float GetShrinkToFitWidth() const;

	// Debug dump layout tree.
	String DumpLayoutTree(int depth = 0) const { return DebugDumpTree(depth); }

	void* operator new(size_t size);
	void operator delete(void* chunk, size_t size);

protected:
	LayoutBox(Type type) : type(type) {}

	void SetVisibleOverflowSize(Vector2f size) { visible_overflow_size = size; }

	virtual String DebugDumpTree(int depth) const = 0;

private:
	Type type;

	// Visible overflow size is the border size of this box, plus any overflowing content. If this is a scroll
	// container, then any overflow should be caught here, and not contribute to our visible overflow size.
	Vector2f visible_overflow_size;
};

} // namespace Rml
#endif
