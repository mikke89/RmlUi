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

#include "FormattingContext.h"
#include "../../../Include/RmlUi/Core/ComputedValues.h"
#include "../../../Include/RmlUi/Core/Element.h"
#include "../../../Include/RmlUi/Core/Profiling.h"
#include "BlockFormattingContext.h"
#include "ContainerBox.h"
#include "FlexFormattingContext.h"
#include "FormattingContextDebug.h"
#include "LayoutBox.h"
#include "LayoutDetails.h"
#include "LayoutNode.h"
#include "ReplacedFormattingContext.h"
#include "TableFormattingContext.h"

namespace Rml {

FormattingContextType FormattingContext::GetFormattingContextType(Element* element)
{
	if (element->IsReplaced())
		return FormattingContextType::Replaced;

	using namespace Style;
	auto& computed = element->GetComputedValues();
	const Display display = computed.display();

	if (display == Display::Flex || display == Display::InlineFlex)
		return FormattingContextType::Flex;

	if (display == Display::Table || display == Display::InlineTable)
		return FormattingContextType::Table;

	if (display == Display::InlineBlock || display == Display::FlowRoot || display == Display::TableCell || computed.float_() != Float::None ||
		computed.position() == Position::Absolute || computed.position() == Position::Fixed || computed.overflow_x() != Overflow::Visible ||
		computed.overflow_y() != Overflow::Visible || !element->GetParentNode() || element->GetParentNode()->GetDisplay() == Display::Flex)
	{
		return FormattingContextType::Block;
	}

	return FormattingContextType::None;
}

UniquePtr<LayoutBox> FormattingContext::FormatIndependent(ContainerBox* parent_container, Element* element, const Box* override_initial_box,
	FormattingContextType default_context)
{
	RMLUI_ASSERT(parent_container && element);
	RMLUI_ZoneScopedC(0xAFAFAF);

	FormattingContextType type = GetFormattingContextType(element);
	if (type == FormattingContextType::None)
		type = default_context;

#ifdef RMLUI_DEBUG
	auto* debug_tracker = FormatIndependentDebugTracker::GetIf();
	FormatIndependentDebugTracker::Entry* tracker_entry = nullptr;
	if (debug_tracker && type != FormattingContextType::None)
	{
		tracker_entry = &debug_tracker->entries.emplace_back(FormatIndependentDebugTracker::Entry{
			debug_tracker->current_stack_level,
			parent_container->GetElement() ? parent_container->GetElement()->GetAddress() : "",
			parent_container->GetAbsolutePositioningContainingBlockElementForDebug()
				? parent_container->GetAbsolutePositioningContainingBlockElementForDebug()->GetAddress()
				: "",
			parent_container->GetContainingBlockSize(Style::Position::Static),
			parent_container->GetContainingBlockSize(Style::Position::Absolute),
			element->GetAddress(),
			override_initial_box ? Optional<Box>{*override_initial_box} : std::nullopt,
			type,
		});
		debug_tracker->current_stack_level += 1;
	}
#endif

	UniquePtr<LayoutBox> layout_box;

	const bool is_under_max_content_constraint = parent_container->IsUnderMaxContentConstraint();
	if (type != FormattingContextType::None && !is_under_max_content_constraint)
	{
		LayoutNode* layout_node = element->GetLayoutNode();
		if (layout_node->CommittedLayoutMatches(parent_container->GetContainingBlockSize(Style::Position::Static),
				parent_container->GetContainingBlockSize(Style::Position::Static), override_initial_box))
		{
			Log::Message(Log::LT_INFO, "Layout cache match on element: %s", element->GetAddress().c_str());
			// TODO: How to deal with ShrinkToFitWidth, in particular for the returned box? Store it in the LayoutNode?
			//   Maybe best not to use this committed layout at all during max-content layouting. Instead, skip this here,
			//   return zero in the CacheContainerBox, and make a separate LayoutNode cache for shrink-to-fit width that
			//   is fetched in LayoutDetails::ShrinkToFitWidth().
			layout_box = MakeUnique<CachedContainer>(element, parent_container, element->GetBox(),
				layout_node->GetCommittedLayout()->visible_overflow_size, layout_node->GetCommittedLayout()->baseline_of_last_line);
		}
	}

	if (!layout_box)
	{
		switch (type)
		{
		case FormattingContextType::Block: layout_box = BlockFormattingContext::Format(parent_container, element, override_initial_box); break;
		case FormattingContextType::Table: layout_box = TableFormattingContext::Format(parent_container, element, override_initial_box); break;
		case FormattingContextType::Flex: layout_box = FlexFormattingContext::Format(parent_container, element, override_initial_box); break;
		case FormattingContextType::Replaced: layout_box = ReplacedFormattingContext::Format(parent_container, element, override_initial_box); break;
		case FormattingContextType::None: break;
		}
	}

	if (layout_box && !is_under_max_content_constraint)
	{
		Optional<float> baseline_of_last_line;
		float baseline_of_last_line_value = 0.f;
		if (layout_box->GetBaselineOfLastLine(baseline_of_last_line_value))
			baseline_of_last_line = baseline_of_last_line_value;

		LayoutNode* layout_node = element->GetLayoutNode();
		layout_node->CommitLayout(parent_container->GetContainingBlockSize(Style::Position::Static),
			parent_container->GetContainingBlockSize(Style::Position::Absolute), override_initial_box, layout_box->GetVisibleOverflowSize(),
			baseline_of_last_line);
	}

#ifdef RMLUI_DEBUG
	if (tracker_entry)
	{
		debug_tracker->current_stack_level -= 1;
		if (layout_box)
		{
			const bool from_cache = (layout_box->GetType() == LayoutBox::Type::CachedContainer);
			tracker_entry->layout = Optional<FormatIndependentDebugTracker::Entry::LayoutResults>({
				from_cache,
				layout_box->GetVisibleOverflowSize(),
				layout_box->GetIfBox() ? Optional<Box>{*layout_box->GetIfBox()} : std::nullopt,
			});
		}
	}
#endif

	return layout_box;
}

} // namespace Rml
