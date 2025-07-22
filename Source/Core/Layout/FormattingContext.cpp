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

static bool UsesFitContentSizeForAutoWidth(Element* element)
{
	// Whether to apply the fit-content sizing (shrink-to-fit width) algorithm to find the width of the element.
	// See CSS 2.1 section 10.3.7 for when this should be applied.
	const auto& computed = element->GetComputedValues();
	const bool inset_auto = (computed.left().type == Style::Left::Auto || computed.right().type == Style::Right::Auto);
	const bool absolutely_positioned = (computed.position() == Style::Position::Absolute || computed.position() == Style::Position::Fixed);

	return (computed.float_() != Style::Float::None) || (absolutely_positioned && inset_auto) ||
		(computed.display() == Style::Display::InlineBlock || computed.display() == Style::Display::InlineFlex);
}

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

	if (type == FormattingContextType::None)
		return nullptr;

	if (element->GetId() == "special")
		int x = 0;

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

	const FormattingMode& formatting_mode = parent_container->GetFormattingMode();

	{
		const Vector2f containing_block = parent_container->GetContainingBlockSize(Style::Position::Static);
		const Vector2f absolute_containing_block = parent_container->GetContainingBlockSize(Style::Position::Absolute);

		LayoutNode* layout_node = element->GetLayoutNode();
		if (layout_node->CommittedLayoutMatches(containing_block, absolute_containing_block, override_initial_box,
				formatting_mode.constraint != FormattingMode::Constraint::None))
		{
#ifdef RMLUI_DEBUG
			Log::Message(Log::LT_INFO, "Layout cache match on element%s: %s",
				(formatting_mode.allow_format_independent_cache ? "" : " (skipping cache due to formatting mode)"), element->GetAddress().c_str());
#endif
			if (formatting_mode.allow_format_independent_cache)
			{
				layout_box = MakeUnique<CachedContainer>(element, parent_container, element->GetBox(),
					layout_node->GetCommittedLayout()->visible_overflow_size, layout_node->GetCommittedLayout()->baseline_of_last_line);
			}
		}
	}

	if (!layout_box)
	{
		const Vector2f containing_block = parent_container->GetContainingBlockSize(element->GetPosition());
		Box box;
		if (override_initial_box)
		{
			box = *override_initial_box;
		}
		else
		{
			const BuildBoxMode build_box_mode = (UsesFitContentSizeForAutoWidth(element) ? BuildBoxMode::FitContent : BuildBoxMode::StretchFit);
			LayoutDetails::BuildBox(box, containing_block, element, build_box_mode);

			if (box.GetSize().x < 0.f)
			{
				FormatFitContentWidth(box, element, type, formatting_mode, containing_block);
				LayoutDetails::ClampSizeAndBuildAutoMarginsForBlockWidth(box, containing_block, element);
			}
		}

		switch (type)
		{
		case FormattingContextType::Block: layout_box = BlockFormattingContext::Format(parent_container, element, containing_block, box); break;
		case FormattingContextType::Table: layout_box = TableFormattingContext::Format(parent_container, element, containing_block, box); break;
		case FormattingContextType::Flex: layout_box = FlexFormattingContext::Format(parent_container, element, containing_block, box); break;
		case FormattingContextType::Replaced: layout_box = ReplacedFormattingContext::Format(parent_container, element, box); break;
		case FormattingContextType::None: RMLUI_ERROR; break;
		}

		// TODO: If MaxContent constraint, and containing block = -1, -1, store resulting size as max-content size.

		if (layout_box)
		{
			Optional<float> baseline_of_last_line;
			float baseline_of_last_line_value = 0.f;
			if (layout_box->GetBaselineOfLastLine(baseline_of_last_line_value))
				baseline_of_last_line = baseline_of_last_line_value;

			LayoutNode* layout_node = element->GetLayoutNode();
			layout_node->CommitLayout(parent_container->GetContainingBlockSize(Style::Position::Static),
				parent_container->GetContainingBlockSize(Style::Position::Absolute), override_initial_box,
				formatting_mode.constraint != FormattingMode::Constraint::None, layout_box->GetVisibleOverflowSize(), baseline_of_last_line);
		}
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

float FormattingContext::FormatFitContentWidth(ContainerBox* parent_container, Element* element, Vector2f containing_block)
{
	Box box;
	LayoutDetails::BuildBox(box, containing_block, element, BuildBoxMode::Unaligned);

	if (box.GetSize().x < 0.f)
	{
		FormattingContextType type = GetFormattingContextType(element);
		if (type == FormattingContextType::None)
			type = FormattingContextType::Block;

		const FormattingMode& formatting_mode = parent_container->GetFormattingMode();
		FormatFitContentWidth(box, element, type, formatting_mode, containing_block);
	}

	return box.GetSize().x;
}

float FormattingContext::FormatFitContentHeight(ContainerBox* parent_container, Element* element, const Box& box)
{
	if (box.GetSize().y >= 0.f)
		return box.GetSize().y;

	LayoutNode* layout_node = element->GetLayoutNode();

	float max_content_height = box.GetSize().y;
	if (Optional<float> cached_height = layout_node->GetMaxContentHeightIfCached())
	{
		max_content_height = *cached_height;
	}
	else
	{
		FormattingContextType type = GetFormattingContextType(element);
		if (type == FormattingContextType::None)
			type = FormattingContextType::Block;

		FormattingMode formatting_mode = parent_container->GetFormattingMode();
		formatting_mode.constraint = FormattingMode::Constraint::MaxContent;
		const Vector2f root_containing_block(-1.f);
		RootBox root(Box(root_containing_block), formatting_mode);

		auto layout_box = FormattingContext::FormatIndependent(&root, element, &box, FormattingContextType::Block);

		const Box* formatted_box = layout_box->GetIfBox();
		if (!formatted_box)
		{
			Log::Message(Log::LT_WARNING, "Could not retrieve fit-content height from formatted result in element %s", element->GetAddress().c_str());
			return -1.f;
		}

		max_content_height = formatted_box->GetSize().y;
		layout_node->CommitMaxContentHeight(max_content_height);
	}

	// In the block axis, min-content, max-content, and fit-content are all equivalent for container boxes. Thus, we
	// should not clamp to the available height, like we do for the fit-content width.
	return max_content_height;
}

void FormattingContext::FormatFitContentWidth(Box& box, Element* element, FormattingContextType type, const FormattingMode& parent_formatting_mode,
	const Vector2f containing_block)
{
	RMLUI_ASSERT(element);
#ifdef RMLUI_TRACY_PROFILING
	RMLUI_ZoneScoped;
	const String zone_text = CreateString("%s    %x    Containing block: %g x %g", element->GetAddress(false, false).c_str(), element,
		containing_block.x, containing_block.y);
	RMLUI_ZoneText(zone_text.c_str(), zone_text.size());
#endif

	const float box_height = box.GetSize().y;
	LayoutNode* layout_node = element->GetLayoutNode();
	float max_content_width = -1.f;

	if (Optional<float> cached_width = layout_node->GetMaxContentWidthIfCached())
	{
		max_content_width = *cached_width;
	}
	else
	{
		// Max-content width should be calculated without any vertical constraint.
		box.SetContent(Vector2f(-1.f));

		FormattingMode formatting_mode = parent_formatting_mode;
		formatting_mode.constraint = FormattingMode::Constraint::MaxContent;

		switch (type)
		{
		case FormattingContextType::Block: max_content_width = BlockFormattingContext::DetermineMaxContentWidth(element, box, formatting_mode); break;
		case FormattingContextType::Table:
			// Currently we don't support shrink-to-fit width for tables, just use a zero-sized width.
			max_content_width = 0.f;
			break;
		case FormattingContextType::Flex: max_content_width = FlexFormattingContext::DetermineMaxContentWidth(element, box, formatting_mode); break;
		case FormattingContextType::Replaced:
			RMLUI_ERRORMSG("Replaced elements are expected to have a positive intrinsice size and do not perform shrink-to-fit layout.");
			break;
		case FormattingContextType::None: RMLUI_ERROR; break;
		}

		layout_node->CommitMaxContentWidth(max_content_width);
	}

	RMLUI_ASSERTMSG(max_content_width >= 0.f, "Max-content width should evaluate to a positive size.")

	float fit_content_width = max_content_width;
	if (containing_block.x >= 0.f)
	{
		const float available_width =
			Math::Max(0.f, containing_block.x - box.GetSizeAcross(BoxDirection::Horizontal, BoxArea::Margin, BoxArea::Padding));
		fit_content_width = Math::Min(max_content_width, available_width);
	}

	box.SetContent(Vector2f(fit_content_width, box_height));
}

} // namespace Rml
