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

#include "LayoutNode.h"
#include "../../../Include/RmlUi/Core/ComputedValues.h"
#include "../../../Include/RmlUi/Core/Element.h"
#include "../../../Include/RmlUi/Core/Log.h"
#include "FormattingContext.h"

namespace Rml {

void LayoutNode::PropagateDirtyToParent()
{
	auto DirtyParentNode = [](Element* element) {
		if (Element* parent = element->GetParentNode())
			parent->GetLayoutNode()->SetDirty(DirtyLayoutType::Child);
	};

	if (IsSelfDirty())
	{
		// We may be able to skip formatting in ancestor elements if this is a layout boundary, in some scenarios. Consider
		// some scenarios for illustration:
		//
		// 1. Absolute element. `display: block` to `display: none`. This does not need a new layout. Same with margin and
		//    size, only the current element need to be reformatted, not ancestors.
		// 2. Absolute element. `display: none` to `display: block`. This *does* need to be layed out, since we don't know
		//    our static position or containing block. We could in principle ignore static position in some situations where
		//    it is not used, and could in principle find our containing block. But it is tricky.
		// 3. Flex container contents changed. If (and only if) it results in a new layed out size, its parent needs to be
		//    reformatted again. If so, it should be able to reuse the flex container's layout cache.
		//
		// Currently, we don't have all of this information here. So skip this for now.
		// - TODO: This information could be provided as part of DirtyLayout.
		// - TODO: Consider if some of this logic should be moved to the layout engine.
		//
		// ```
		//     if (IsLayoutBoundary()) return;
		// ```

		DirtyParentNode(element);
		return;
	}

	if (IsChildDirty() && !IsLayoutBoundary())
	{
		DirtyParentNode(element);
		return;
	}
}

void LayoutNode::ClearDirty()
{
	// Log::Message(Log::LT_INFO, "ClearDirty (was Self %d  Child %d)  Element: %s", (dirty_flag & DirtyLayoutType::DOM) != DirtyLayoutType::None,
	//	(dirty_flag & DirtyLayoutType::Child) != DirtyLayoutType::None, element->GetAddress().c_str());
	dirty_flag = DirtyLayoutType::None;
}

void LayoutNode::SetDirty(DirtyLayoutType dirty_type)
{
	// Log::Message(Log::LT_INFO, "SetDirty. Self %d  Child %d  Element: %s", (dirty_type & DirtyLayoutType::DOM) != DirtyLayoutType::None,
	//	(dirty_type & DirtyLayoutType::Child) != DirtyLayoutType::None, element->GetAddress().c_str());
	dirty_flag = dirty_flag | dirty_type;
	committed_max_content_width.reset();
	committed_max_content_height.reset();
}

void LayoutNode::CommitLayout(Vector2f containing_block_size, Vector2f absolutely_positioning_containing_block_size, const Box* override_box,
	bool layout_constraint, Vector2f visible_overflow_size, float max_content_width, Optional<float> baseline_of_last_line)
{
	// TODO: This is mixing slightly different concepts. Rather, it might be advantageous to separate what is the input
	// to the layout of the current element, and what is the output. That way we can e.g. set the containing block size
	// even if there is nothing to format (for example due to `display: none`), or if the element itself can be cached
	// despite ancestor changes. This way we can resume the layout here, without formatting its ancestors, if it is
	// dirtied in a non-parent-mutable way.
	// - E.g. consider scenario 2 above with Absolute element `display: none` to `display: block`.
	//
	// Conversely, the output of the layout passed in here can later be used by when formatting ancestors, when the
	// current element does not need a new layout by itself.
	committed_layout.emplace(CommittedLayout{
		containing_block_size,
		absolutely_positioning_containing_block_size,
		override_box ? Optional<Box>(*override_box) : Optional<Box>(),
		layout_constraint,
		visible_overflow_size,
		max_content_width,
		baseline_of_last_line,
	});
	ClearDirty();
}

bool LayoutNode::IsLayoutBoundary() const
{
	using namespace Style;
	auto& computed = element->GetComputedValues();

	// TODO: Should this be moved into PropagateDirtyToParent() instead? It's not really a layout boundary, or
	// maybe it's okay?
	if (computed.display() == Display::None)
		return true;

	const FormattingContextType formatting_context = FormattingContext::GetFormattingContextType(element);
	if (formatting_context == FormattingContextType::None)
		return false;

	if (computed.position() == Position::Absolute || computed.position() == Position::Fixed)
		return true;

	return false;
}

} // namespace Rml
