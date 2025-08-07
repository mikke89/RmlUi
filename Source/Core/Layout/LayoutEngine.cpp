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

#include "LayoutEngine.h"
#include "../../../Include/RmlUi/Core/Element.h"
#include "../../../Include/RmlUi/Core/ElementUtilities.h"
#include "../../../Include/RmlUi/Core/Log.h"
#include "../../../Include/RmlUi/Core/Profiling.h"
#include "ContainerBox.h"
#include "FormattingContext.h"
#include "LayoutDetails.h"
#include "LayoutNode.h"

namespace Rml {

static void FormatElementImpl(Element* element, Vector2f containing_block, Vector2f absolutely_positioning_containing_block,
	const FormattingMode& formatting_mode)
{
	RMLUI_ZoneScoped;

	RootBox absolute_root(Box(absolutely_positioning_containing_block), formatting_mode);
	RootBox root(Box(containing_block), &absolute_root);

	auto layout_box = FormattingContext::FormatIndependent(&root, element, nullptr, FormattingContextType::Block);
	if (!layout_box)
	{
		Log::Message(Log::LT_ERROR, "Error while formatting element: %s", element->GetAddress().c_str());
	}

	if (int num_absolute_boxes = absolute_root.CountAbsolutelyPositionedBoxes(); num_absolute_boxes != 0)
	{
		// In the end, this might be fine, but needs further investigation.
		Log::Message(Log::LT_ERROR, "%d absolutely positioned box(es) not closed, while formatting: %s", num_absolute_boxes,
			element->GetAddress().c_str());
	}
}

void LayoutEngine::FormatElement(Element* layout_root, Vector2f containing_block, bool allow_cache)
{
	RMLUI_ASSERT(layout_root && containing_block.x >= 0 && containing_block.y >= 0);

	const FormattingMode formatting_mode{FormattingMode::Constraint::None, allow_cache, allow_cache};

	constexpr bool debug_logging = false;
	if (debug_logging)
		Log::Message(Log::LT_INFO, "UpdateLayout start: %s", layout_root->GetAddress().c_str());

	struct ElementToFormat {
		Element* element;
		Vector2f containing_block_size;
		Vector2f absolutely_positioning_containing_block_size;
	};
	Vector<ElementToFormat> elements;

	bool force_full_document_layout = !allow_cache;
	if (!force_full_document_layout)
	{
		ElementUtilities::BreadthFirstSearch(layout_root, [&](Element* candidate) {
			if (candidate->GetDisplay() == Style::Display::None)
				return ElementUtilities::CallbackControlFlow::SkipChildren;

			LayoutNode* layout_node = candidate->GetLayoutNode();
			if (!layout_node->IsDirty())
				return ElementUtilities::CallbackControlFlow::Continue;

			if (!layout_node->IsLayoutBoundary())
			{
				// Normally the dirty layout should have propagated to the closest layout boundary during
				// Element::Update(), which then invokes formatting on that boundary element, and which again clears the
				// dirty layout on this element. This didn't happen here, which may be caused by modifying the layout
				// during layouting itself. For now, we simply skip this element and keep going. The element should
				// normally be properly layed-out on the next context update.
				// TODO: Might want to investigate this further.
				if (debug_logging)
					Log::Message(Log::LT_INFO, "Dirty layout on non-layout boundary element: %s", candidate->GetAddress().c_str());
				return ElementUtilities::CallbackControlFlow::Continue;
			}

			const Optional<CommittedLayout>& committed_layout = layout_node->GetCommittedLayout();
			if (!committed_layout)
			{
				if (candidate != layout_root && debug_logging)
					Log::Message(Log::LT_INFO, "Forcing full layout update due to missing committed layout on element: %s",
						candidate->GetAddress().c_str());
				force_full_document_layout = true;
				return ElementUtilities::CallbackControlFlow::Break;
			}

			elements.push_back(ElementToFormat{
				candidate,
				committed_layout->containing_block_size,
				committed_layout->absolutely_positioning_containing_block_size,
			});

			return ElementUtilities::CallbackControlFlow::SkipChildren;
		});
	}

	if (force_full_document_layout)
	{
		elements = {
			ElementToFormat{
				layout_root,
				containing_block,
				containing_block,
			},
		};
	}

	for (const ElementToFormat& element_to_format : elements)
	{
		if (element_to_format.element != layout_root && debug_logging)
			Log::Message(Log::LT_INFO, "Doing partial layout update on element: %s", element_to_format.element->GetAddress().c_str());

		// TODO: In some cases, we need to check if size changed, such that we need to do a layout update in its parent.
		FormatElementImpl(element_to_format.element, element_to_format.containing_block_size,
			element_to_format.absolutely_positioning_containing_block_size, formatting_mode);

		// TODO: A bit ugly
		element_to_format.element->UpdateRelativeOffsetFromInsetConstraints();
	}

	if (elements.empty() && debug_logging)
		Log::Message(Log::LT_INFO, "Didn't make any layout updates on dirty document: %s", layout_root->GetAddress().c_str());

	if (debug_logging)
		Log::Message(Log::LT_INFO, "Document layout: Clear dirty on %s", layout_root->GetAddress().c_str());

	{
		RMLUI_ZoneScopedN("CommitLayoutRecursive");
		layout_root->CommitLayoutRecursive();
	}
}

} // namespace Rml
