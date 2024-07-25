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

#include "FlexFormattingContext.h"
#include "../../../Include/RmlUi/Core/ComputedValues.h"
#include "../../../Include/RmlUi/Core/Element.h"
#include "../../../Include/RmlUi/Core/ElementScroll.h"
#include "../../../Include/RmlUi/Core/Profiling.h"
#include "../../../Include/RmlUi/Core/Types.h"
#include "ContainerBox.h"
#include "LayoutDetails.h"
#include "LayoutEngine.h"
#include <algorithm>
#include <float.h>
#include <numeric>

namespace Rml {

UniquePtr<LayoutBox> FlexFormattingContext::Format(ContainerBox* parent_container, Element* element, const Box* override_initial_box)
{
	RMLUI_ZoneScopedC(0xAFAF4F);
	auto flex_container_box = MakeUnique<FlexContainer>(element, parent_container);

	ElementScroll* element_scroll = element->GetElementScroll();
	const ComputedValues& computed = element->GetComputedValues();

	const Vector2f containing_block = LayoutDetails::GetContainingBlock(parent_container, element->GetPosition()).size;
	RMLUI_ASSERT(containing_block.x >= 0.f);

	// Build the initial box as specified by the flex's style, as if it was a normal block element.
	Box& box = flex_container_box->GetBox();
	if (override_initial_box)
		box = *override_initial_box;
	else
		LayoutDetails::BuildBox(box, containing_block, element, BuildBoxMode::Block);

	// Start with any auto-scrollbars off.
	flex_container_box->ResetScrollbars(box);

	FlexFormattingContext context;
	context.flex_container_box = flex_container_box.get();
	context.element_flex = element;

	LayoutDetails::GetMinMaxWidth(context.flex_min_size.x, context.flex_max_size.x, computed, box, containing_block.x);
	LayoutDetails::GetMinMaxHeight(context.flex_min_size.y, context.flex_max_size.y, computed, box, containing_block.y);

	const Vector2f box_content_size = box.GetSize();
	const bool auto_height = (box_content_size.y < 0.0f);

	context.flex_content_offset = box.GetPosition();

	for (int layout_iteration = 0; layout_iteration < 3; layout_iteration++)
	{
		// One or both scrollbars can be enabled between iterations.
		const Vector2f scrollbar_size = {
			element_scroll->GetScrollbarSize(ElementScroll::VERTICAL),
			element_scroll->GetScrollbarSize(ElementScroll::HORIZONTAL),
		};

		context.flex_available_content_size = Math::Max(box_content_size - scrollbar_size, Vector2f(0.f));
		context.flex_content_containing_block = context.flex_available_content_size;

		if (auto_height)
		{
			context.flex_available_content_size.y = -1.f; // Negative means infinite space
			context.flex_content_containing_block.y = containing_block.y;
		}

		Math::SnapToPixelGrid(context.flex_content_offset, context.flex_available_content_size);

		// Format the flexbox and all its children.
		Vector2f flex_resulting_content_size, content_overflow_size;
		float flex_baseline = 0.f;
		context.Format(flex_resulting_content_size, content_overflow_size, flex_baseline);

		// Output the size of the formatted flexbox. The width is determined as a normal block box so we don't need to change that.
		Vector2f formatted_content_size = box_content_size;
		if (auto_height)
			formatted_content_size.y = flex_resulting_content_size.y + scrollbar_size.y;

		Box sized_box = box;
		sized_box.SetContent(formatted_content_size);

		// Change the flex baseline coordinates to the element baseline, which is defined as the distance from the element's bottom margin edge.
		const float element_baseline =
			sized_box.GetSizeAcross(BoxDirection::Vertical, BoxArea::Border) + sized_box.GetEdge(BoxArea::Margin, BoxEdge::Bottom) - flex_baseline;

		// Close the box, and break out of the loop if it did not produce any new scrollbars, otherwise continue to format the flexbox again.
		if (flex_container_box->Close(content_overflow_size, sized_box, element_baseline))
			break;
	}

	return flex_container_box;
}

Vector2f FlexFormattingContext::GetMaxContentSize(Element* element)
{
	// A large but finite number is used here, because the flexbox formatting algorithm
	// needs to round numbers, and it doesn't support infinities.
	const Vector2f infinity(10000.0f, 10000.0f);
	RootBox root(infinity);
	auto flex_container_box = MakeUnique<FlexContainer>(element, &root);

	FlexFormattingContext context;
	context.flex_container_box = flex_container_box.get();
	context.element_flex = element;
	context.flex_available_content_size = Vector2f(-1, -1);
	context.flex_content_containing_block = infinity;
	context.flex_max_size = Vector2f(FLT_MAX, FLT_MAX);

	// Format the flexbox and all its children.
	Vector2f flex_resulting_content_size, content_overflow_size;
	float flex_baseline = 0.f;
	context.Format(flex_resulting_content_size, content_overflow_size, flex_baseline);
	return flex_resulting_content_size;
}

struct FlexItem {
	// In the following, suffix '_a' means flex start edge while '_b' means flex end edge.
	struct Size {
		bool auto_margin_a, auto_margin_b;
		bool auto_size;
		float margin_a, margin_b;
		float sum_edges_a;        // Start edge: margin (non-auto) + border + padding
		float sum_edges;          // Inner->outer size
		float min_size, max_size; // Inner size
	};

	Element* element;
	Box box;

	// Filled during the build step.
	Size main;
	Size cross;
	float flex_shrink_factor;
	float flex_grow_factor;
	Style::AlignSelf align_self;  // 'Auto' is replaced by container's 'align-items' value

	float inner_flex_base_size;   // Inner size
	float flex_base_size;         // Outer size
	float hypothetical_main_size; // Outer size

	// Used for resolving flexible length
	enum class Violation : uint8_t { None = 0, Min, Max };
	bool frozen;
	Violation violation;
	float target_main_size; // Outer size
	float used_main_size;   // Outer size (without auto margins)
	float main_auto_margin_size_a, main_auto_margin_size_b;
	float main_offset;

	// Used for resolving cross size
	float hypothetical_cross_size; // Outer size
	float used_cross_size;         // Outer size
	float cross_offset;            // Offset within line
	float cross_baseline_top;      // Only used for baseline cross alignment
};

struct FlexLine {
	FlexLine(Vector<FlexItem>&& items) : items(std::move(items)) {}
	Vector<FlexItem> items;
	float accumulated_hypothetical_main_size = 0;
	float cross_size = 0; // Excludes line spacing
	float cross_spacing_a = 0, cross_spacing_b = 0;
	float cross_offset = 0;
};

struct FlexLineContainer {
	Vector<FlexLine> lines;
};

static void GetItemSizing(FlexItem::Size& destination, const ComputedAxisSize& computed_size, const float base_value, const bool direction_reverse)
{
	float margin_a, margin_b, padding_border_a, padding_border_b;
	LayoutDetails::GetEdgeSizes(margin_a, margin_b, padding_border_a, padding_border_b, computed_size, base_value);

	const float padding_border = padding_border_a + padding_border_b;
	const float margin = margin_a + margin_b;

	destination.auto_margin_a = (computed_size.margin_a.type == Style::Margin::Auto);
	destination.auto_margin_b = (computed_size.margin_b.type == Style::Margin::Auto);

	destination.auto_size = (computed_size.size.type == Style::LengthPercentageAuto::Auto);

	destination.margin_a = margin_a;
	destination.margin_b = margin_b;
	destination.sum_edges = padding_border + margin;
	destination.sum_edges_a = (direction_reverse ? padding_border_b + margin_b : padding_border_a + margin_a);

	destination.min_size = ResolveValue(computed_size.min_size, base_value);
	destination.max_size = ResolveValue(computed_size.max_size, base_value);

	if (computed_size.box_sizing == Style::BoxSizing::BorderBox)
	{
		destination.min_size = Math::Max(0.0f, destination.min_size - padding_border);
		if (destination.max_size < FLT_MAX)
			destination.max_size = Math::Max(0.0f, destination.max_size - padding_border);
	}

	if (direction_reverse)
	{
		std::swap(destination.auto_margin_a, destination.auto_margin_b);
		std::swap(destination.margin_a, destination.margin_b);
	}
}

void FlexFormattingContext::Format(Vector2f& flex_resulting_content_size, Vector2f& flex_content_overflow_size, float& flex_baseline) const
{
	// The following procedure is based on the CSS flexible box layout algorithm.
	// For details, see https://drafts.csswg.org/css-flexbox/#layout-algorithm

	const ComputedValues& computed_flex = element_flex->GetComputedValues();
	const Style::FlexDirection direction = computed_flex.flex_direction();
	const Style::LengthPercentage row_gap = computed_flex.row_gap();
	const Style::LengthPercentage column_gap = computed_flex.column_gap();

	const bool main_axis_horizontal = (direction == Style::FlexDirection::Row || direction == Style::FlexDirection::RowReverse);
	const bool direction_reverse = (direction == Style::FlexDirection::RowReverse || direction == Style::FlexDirection::ColumnReverse);
	const bool flex_single_line = (computed_flex.flex_wrap() == Style::FlexWrap::Nowrap);
	const bool wrap_reverse = (computed_flex.flex_wrap() == Style::FlexWrap::WrapReverse);

	const float main_available_size = (main_axis_horizontal ? flex_available_content_size.x : flex_available_content_size.y);
	const float cross_available_size = (!main_axis_horizontal ? flex_available_content_size.x : flex_available_content_size.y);

	const float main_min_size = (main_axis_horizontal ? flex_min_size.x : flex_min_size.y);
	const float main_max_size = (main_axis_horizontal ? flex_max_size.x : flex_max_size.y);
	const float cross_min_size = (main_axis_horizontal ? flex_min_size.y : flex_min_size.x);
	const float cross_max_size = (main_axis_horizontal ? flex_max_size.y : flex_max_size.x);

	// For the purpose of placing items we make infinite size a big value.
	const float main_wrap_size = Math::Clamp(main_available_size < 0.0f ? FLT_MAX : main_available_size, main_min_size, main_max_size);

	// For the purpose of resolving lengths, infinite main size becomes zero.
	const float main_size_base_value = (main_available_size < 0.0f ? 0.0f : main_available_size);
	const float cross_size_base_value = (cross_available_size < 0.0f ? 0.0f : cross_available_size);

	const float main_gap_size = ResolveValue(main_axis_horizontal ? column_gap : row_gap, main_size_base_value);
	const float cross_gap_size = ResolveValue(main_axis_horizontal ? row_gap : column_gap, cross_size_base_value);

	// -- Build a list of all flex items with base size information --
	const int num_flex_children = element_flex->GetNumChildren();
	Vector<FlexItem> items;
	items.reserve(num_flex_children);

	for (int i = 0; i < num_flex_children; i++)
	{
		Element* element = element_flex->GetChild(i);
		const ComputedValues& computed = element->GetComputedValues();

		if (computed.display() == Style::Display::None)
		{
			continue;
		}
		else if (computed.position() == Style::Position::Absolute || computed.position() == Style::Position::Fixed)
		{
			ContainerBox* absolute_containing_block = LayoutDetails::GetContainingBlock(flex_container_box, computed.position()).container;
			absolute_containing_block->AddAbsoluteElement(element, {}, element_flex);
			continue;
		}
		else if (computed.position() == Style::Position::Relative)
		{
			flex_container_box->AddRelativeElement(element);
		}

		FlexItem item = {};
		item.element = element;
		LayoutDetails::BuildBox(item.box, flex_content_containing_block, element, BuildBoxMode::UnalignedBlock);

		Style::LengthPercentageAuto item_main_size;

		{
			const ComputedAxisSize computed_main_size =
				main_axis_horizontal ? LayoutDetails::BuildComputedHorizontalSize(computed) : LayoutDetails::BuildComputedVerticalSize(computed);
			const ComputedAxisSize computed_cross_size =
				!main_axis_horizontal ? LayoutDetails::BuildComputedHorizontalSize(computed) : LayoutDetails::BuildComputedVerticalSize(computed);

			GetItemSizing(item.main, computed_main_size, main_size_base_value, direction_reverse);
			GetItemSizing(item.cross, computed_cross_size, cross_size_base_value, wrap_reverse);

			item_main_size = computed_main_size.size;
		}

		item.flex_shrink_factor = computed.flex_shrink();
		item.flex_grow_factor = computed.flex_grow();
		item.align_self = computed.align_self();

		static_assert(int(Style::AlignSelf::FlexStart) == int(Style::AlignItems::FlexStart) + 1 &&
				int(Style::AlignSelf::Stretch) == int(Style::AlignItems::Stretch) + 1,
			"It is assumed below that align items is a shifted version (no auto value) of align self.");

		// Use the container's align-items property if align-self is auto.
		if (item.align_self == Style::AlignSelf::Auto)
			item.align_self = static_cast<Style::AlignSelf>(static_cast<int>(computed_flex.align_items()) + 1);

		auto GetMainSize = [&](const Box& box) { return box.GetSize()[main_axis_horizontal ? 0 : 1]; };

		const float sum_padding_border = item.main.sum_edges - (item.main.margin_a + item.main.margin_b);

		// Find the flex base size (possibly negative when using border box sizing)
		if (computed.flex_basis().type != Style::FlexBasis::Auto)
		{
			item.inner_flex_base_size = ResolveValue(computed.flex_basis(), main_size_base_value);
			if (computed.box_sizing() == Style::BoxSizing::BorderBox)
				item.inner_flex_base_size -= sum_padding_border;
		}
		else if (!item.main.auto_size)
		{
			item.inner_flex_base_size = ResolveValue(item_main_size, main_size_base_value);
			if (computed.box_sizing() == Style::BoxSizing::BorderBox)
				item.inner_flex_base_size -= sum_padding_border;
		}
		else if (GetMainSize(item.box) >= 0.f)
		{
			// The element is auto-sized, and yet its box was given a definite size. This can happen e.g. due to intrinsic sizing or aspect ratios.
			item.inner_flex_base_size = GetMainSize(item.box);
		}
		else if (main_axis_horizontal)
		{
			item.inner_flex_base_size = LayoutDetails::GetShrinkToFitWidth(element, flex_content_containing_block);
		}
		else
		{
			const Vector2f initial_box_size = item.box.GetSize();
			RMLUI_ASSERT(initial_box_size.y < 0.f);

			Box format_box = item.box;
			if (initial_box_size.x < 0.f && flex_available_content_size.x >= 0.f)
				format_box.SetContent(Vector2f(flex_available_content_size.x - item.cross.sum_edges, initial_box_size.y));

			FormattingContext::FormatIndependent(flex_container_box, element, (format_box.GetSize().x >= 0 ? &format_box : nullptr),
				FormattingContextType::Block);
			item.inner_flex_base_size = element->GetBox().GetSize().y;
		}

		// Calculate the hypothetical main size (clamped flex base size).
		item.hypothetical_main_size = Math::Clamp(item.inner_flex_base_size, item.main.min_size, item.main.max_size) + item.main.sum_edges;
		item.flex_base_size = item.inner_flex_base_size + item.main.sum_edges;

		items.push_back(std::move(item));
	}

	if (items.empty())
	{
		return;
	}

	// -- Collect the items into lines --
	FlexLineContainer container;

	if (flex_single_line)
	{
		container.lines.emplace_back(std::move(items));
	}
	else
	{
		float cursor = 0;

		Vector<FlexItem> line_items;

		for (FlexItem& item : items)
		{
			cursor += item.hypothetical_main_size;

			if (!line_items.empty() && cursor > main_wrap_size)
			{
				// Break into new line.
				container.lines.emplace_back(std::move(line_items));
				cursor = item.hypothetical_main_size;
				line_items = {std::move(item)};
			}
			else
			{
				// Add item to current line.
				line_items.push_back(std::move(item));
			}

			cursor += main_gap_size;
		}

		if (!line_items.empty())
			container.lines.emplace_back(std::move(line_items));

		items.clear();
		items.shrink_to_fit();
	}

	for (FlexLine& line : container.lines)
	{
		// now that items are in lines, we can add the main gap size to all but the last item
		if (main_gap_size > 0.f)
		{
			for (size_t i = 0; i < line.items.size() - 1; i++)
			{
				line.items[i].hypothetical_main_size += main_gap_size;
				line.items[i].flex_base_size += main_gap_size;
				line.items[i].main.margin_b += main_gap_size;
				line.items[i].main.sum_edges += main_gap_size;
			}
		}

		line.accumulated_hypothetical_main_size = std::accumulate(line.items.begin(), line.items.end(), 0.0f,
			[](float value, const FlexItem& item) { return value + item.hypothetical_main_size; });
	}

	// If the available main size is infinite, the used main size becomes the accumulated outer size of all items of the widest line.
	const float used_main_size_unconstrained = main_available_size >= 0.f
		? main_available_size
		: std::max_element(container.lines.begin(), container.lines.end(), [](const FlexLine& a, const FlexLine& b) {
			  return a.accumulated_hypothetical_main_size < b.accumulated_hypothetical_main_size;
		  })->accumulated_hypothetical_main_size;

	const float used_main_size = Math::Clamp(used_main_size_unconstrained, main_min_size, main_max_size);

	// -- Determine main size --
	// Resolve flexible lengths to find the used main size of all items.
	for (FlexLine& line : container.lines)
	{
		const float available_flex_space = used_main_size - line.accumulated_hypothetical_main_size; // Possibly negative

		const bool flex_mode_grow = (available_flex_space > 0.f);

		auto FlexFactor = [flex_mode_grow](const FlexItem& item) { return (flex_mode_grow ? item.flex_grow_factor : item.flex_shrink_factor); };

		// Initialize items and freeze inflexible items.
		for (FlexItem& item : line.items)
		{
			item.target_main_size = item.flex_base_size;

			if (FlexFactor(item) == 0.f || (flex_mode_grow && item.flex_base_size > item.hypothetical_main_size) ||
				(!flex_mode_grow && item.flex_base_size < item.hypothetical_main_size))
			{
				item.frozen = true;
				item.target_main_size = item.hypothetical_main_size;
			}
		}

		auto RemainingFreeSpace = [used_main_size, &line]() {
			return used_main_size - std::accumulate(line.items.begin(), line.items.end(), 0.f, [](float value, const FlexItem& item) {
				return value + (item.frozen ? item.target_main_size : item.flex_base_size);
			});
		};

		const float initial_free_space = RemainingFreeSpace();

		// Now iteratively distribute or shrink the size of all the items, until all the items are frozen.
		while (!std::all_of(line.items.begin(), line.items.end(), [](const FlexItem& item) { return item.frozen; }))
		{
			float remaining_free_space = RemainingFreeSpace();

			const float flex_factor_sum = std::accumulate(line.items.begin(), line.items.end(), 0.f,
				[&FlexFactor](float value, const FlexItem& item) { return value + (item.frozen ? 0.0f : FlexFactor(item)); });

			if (flex_factor_sum < 1.f)
			{
				const float scaled_initial_free_space = initial_free_space * flex_factor_sum;
				if (Math::Absolute(scaled_initial_free_space) < Math::Absolute(remaining_free_space))
					remaining_free_space = scaled_initial_free_space;
			}

			if (remaining_free_space != 0.f)
			{
				// Distribute free space proportionally to flex factors
				if (flex_mode_grow)
				{
					for (FlexItem& item : line.items)
					{
						if (!item.frozen)
						{
							const float distribute_ratio = item.flex_grow_factor / flex_factor_sum;
							item.target_main_size = item.flex_base_size + distribute_ratio * remaining_free_space;
						}
					}
				}
				else
				{
					const float scaled_flex_shrink_factor_sum =
						std::accumulate(line.items.begin(), line.items.end(), 0.f, [](float value, const FlexItem& item) {
							return value + (item.frozen ? 0.0f : item.flex_shrink_factor * item.inner_flex_base_size);
						});
					const float scaled_flex_shrink_factor_sum_nonzero = (scaled_flex_shrink_factor_sum == 0 ? 1 : scaled_flex_shrink_factor_sum);

					for (FlexItem& item : line.items)
					{
						if (!item.frozen)
						{
							const float scaled_flex_shrink_factor = item.flex_shrink_factor * item.inner_flex_base_size;
							const float distribute_ratio = scaled_flex_shrink_factor / scaled_flex_shrink_factor_sum_nonzero;
							item.target_main_size = item.flex_base_size - distribute_ratio * Math::Absolute(remaining_free_space);
						}
					}
				}
			}

			// Clamp min/max violations
			float total_minmax_violation = 0.f;

			for (FlexItem& item : line.items)
			{
				if (!item.frozen)
				{
					const float inner_target_main_size = Math::Max(0.0f, item.target_main_size - item.main.sum_edges);
					const float clamped_target_main_size =
						Math::Clamp(inner_target_main_size, item.main.min_size, item.main.max_size) + item.main.sum_edges;

					const float violation_diff = clamped_target_main_size - item.target_main_size;
					item.violation = (violation_diff > 0.0f ? FlexItem::Violation::Min
															: (violation_diff < 0.f ? FlexItem::Violation::Max : FlexItem::Violation::None));
					item.target_main_size = clamped_target_main_size;

					total_minmax_violation += violation_diff;
				}
			}

			for (FlexItem& item : line.items)
			{
				if (total_minmax_violation > 0.0f)
					item.frozen |= (item.violation == FlexItem::Violation::Min);
				else if (total_minmax_violation < 0.0f)
					item.frozen |= (item.violation == FlexItem::Violation::Max);
				else
					item.frozen = true;
			}
		}

		// Now, each item's used main size is found!
		for (FlexItem& item : line.items)
			item.used_main_size = item.target_main_size;
	}

	// -- Align main axis (§9.5) --
	// Main alignment is done before cross sizing. Due to rounding to the pixel grid, the main size can
	// change slightly after main alignment/offseting. Also, the cross sizing depends on the main sizing
	// so doing it in this order ensures no surprises (overflow/wrapping issues) due to pixel rounding.
	for (FlexLine& line : container.lines)
	{
		const float remaining_free_space = used_main_size -
			std::accumulate(line.items.begin(), line.items.end(), 0.f, [](float value, const FlexItem& item) { return value + item.used_main_size; });

		if (remaining_free_space > 0.0f)
		{
			const int num_auto_margins = std::accumulate(line.items.begin(), line.items.end(), 0,
				[](int value, const FlexItem& item) { return value + int(item.main.auto_margin_a) + int(item.main.auto_margin_b); });

			if (num_auto_margins > 0)
			{
				// Distribute the remaining space to the auto margins.
				const float space_per_auto_margin = remaining_free_space / float(num_auto_margins);
				for (FlexItem& item : line.items)
				{
					if (item.main.auto_margin_a)
						item.main_auto_margin_size_a = space_per_auto_margin;
					if (item.main.auto_margin_b)
						item.main_auto_margin_size_b = space_per_auto_margin;
				}
			}
			else
			{
				// Distribute the remaining space based on the 'justify-content' property.
				using Style::JustifyContent;
				const int num_items = int(line.items.size());

				switch (computed_flex.justify_content())
				{
				case JustifyContent::SpaceBetween:
					if (num_items > 1)
					{
						const float space_per_edge = remaining_free_space / float(2 * num_items - 2);
						for (int i = 0; i < num_items; i++)
						{
							FlexItem& item = line.items[i];
							if (i > 0)
								item.main_auto_margin_size_a = space_per_edge;
							if (i < num_items - 1)
								item.main_auto_margin_size_b = space_per_edge;
						}
						break;
					}
					//-fallthrough
				case JustifyContent::FlexStart: line.items.back().main_auto_margin_size_b = remaining_free_space; break;
				case JustifyContent::FlexEnd: line.items.front().main_auto_margin_size_a = remaining_free_space; break;
				case JustifyContent::Center:
					line.items.front().main_auto_margin_size_a = 0.5f * remaining_free_space;
					line.items.back().main_auto_margin_size_b = 0.5f * remaining_free_space;
					break;
				case JustifyContent::SpaceAround:
				{
					const float space_per_edge = remaining_free_space / float(2 * num_items);
					for (FlexItem& item : line.items)
					{
						item.main_auto_margin_size_a = space_per_edge;
						item.main_auto_margin_size_b = space_per_edge;
					}
				}
				break;
				case JustifyContent::SpaceEvenly:
				{
					const float space_per_edge = remaining_free_space / float(2 * (num_items + 1));
					for (int i = 0; i < num_items; i++)
					{
						FlexItem& item = line.items[i];
						item.main_auto_margin_size_a = space_per_edge;
						item.main_auto_margin_size_b = space_per_edge;
						if (i == 0)
							item.main_auto_margin_size_a *= 2.0f;
						else if (i == num_items - 1)
							item.main_auto_margin_size_b *= 2.0f;
					}
				}
				break;
				}
			}
		}

		// Now find the offset and snap the outer edges to the pixel grid.
		float cursor = 0.0f;
		for (FlexItem& item : line.items)
		{
			if (direction_reverse)
				item.main_offset = used_main_size - (cursor + item.used_main_size + item.main_auto_margin_size_a - item.main.margin_b);
			else
				item.main_offset = cursor + item.main.margin_a + item.main_auto_margin_size_a;

			cursor += item.used_main_size + item.main_auto_margin_size_a + item.main_auto_margin_size_b;
			Math::SnapToPixelGrid(item.main_offset, item.used_main_size);
		}
	}

	// Apply cross axis gaps to every item in every line except the last line.
	if (cross_gap_size > 0.f)
	{
		for (size_t i = 0; i < container.lines.size() - 1; i++)
		{
			FlexLine& line = container.lines[i];
			for (FlexItem& item : line.items)
			{
				item.cross.margin_b += cross_gap_size;
				item.cross.sum_edges += cross_gap_size;
			}
		}
	}

	// -- Determine cross size (§9.4) --
	// First, determine the cross size of each item, format it if necessary.
	for (FlexLine& line : container.lines)
	{
		for (FlexItem& item : line.items)
		{
			const Vector2f content_size = item.box.GetSize();
			const float used_main_size_inner = item.used_main_size - item.main.sum_edges;

			if (main_axis_horizontal)
			{
				if (content_size.y < 0.0f)
				{
					item.box.SetContent(Vector2f(used_main_size_inner, content_size.y));
					FormattingContext::FormatIndependent(flex_container_box, item.element, &item.box, FormattingContextType::Block);
					item.hypothetical_cross_size = item.element->GetBox().GetSize().y + item.cross.sum_edges;
				}
				else
				{
					item.hypothetical_cross_size = content_size.y + item.cross.sum_edges;
				}
			}
			else
			{
				if (content_size.x < 0.0f || item.cross.auto_size)
				{
					item.box.SetContent(Vector2f(content_size.x, used_main_size_inner));
					item.hypothetical_cross_size =
						LayoutDetails::GetShrinkToFitWidth(item.element, flex_content_containing_block) + item.cross.sum_edges;
				}
				else
				{
					item.hypothetical_cross_size = content_size.x + item.cross.sum_edges;
				}
			}
		}
	}

	// Determine cross size of each line.
	if (cross_available_size >= 0.f && flex_single_line && container.lines.size() == 1)
	{
		container.lines[0].cross_size = cross_available_size;
	}
	else
	{
		for (FlexLine& line : container.lines)
		{
			const float largest_hypothetical_cross_size =
				std::max_element(line.items.begin(), line.items.end(), [](const FlexItem& a, const FlexItem& b) {
					return a.hypothetical_cross_size < b.hypothetical_cross_size;
				})->hypothetical_cross_size;

			// Currently, we don't handle the case where baseline alignment could extend the line's cross size, see CSS specs 9.4.8.
			line.cross_size = Math::Max(0.0f, Math::Round(largest_hypothetical_cross_size));

			if (flex_single_line)
				line.cross_size = Math::Clamp(line.cross_size, cross_min_size, cross_max_size);
		}
	}

	// Stretch out the lines if we have extra space.
	if (cross_available_size >= 0.f && computed_flex.align_content() == Style::AlignContent::Stretch)
	{
		int remaining_space = static_cast<int>(cross_available_size -
			std::accumulate(container.lines.begin(), container.lines.end(), 0.f,
				[](float value, const FlexLine& line) { return value + line.cross_size; }));

		if (remaining_space > 0)
		{
			// Here we use integer math to ensure all space is distributed to pixel boundaries.
			const int num_lines = (int)container.lines.size();
			for (int i = 0; i < num_lines; i++)
			{
				const int add_space_to_line = remaining_space / (num_lines - i);
				remaining_space -= add_space_to_line;
				container.lines[i].cross_size += static_cast<float>(add_space_to_line);
			}
		}
	}

	// Determine the used cross size of items.
	for (FlexLine& line : container.lines)
	{
		for (FlexItem& item : line.items)
		{
			const bool stretch_item = (item.align_self == Style::AlignSelf::Stretch);
			if (stretch_item && item.cross.auto_size && !item.cross.auto_margin_a && !item.cross.auto_margin_b)
			{
				item.used_cross_size =
					Math::Clamp(line.cross_size - item.cross.sum_edges, item.cross.min_size, item.cross.max_size) + item.cross.sum_edges;
				// Here we are supposed to re-format the item with the new size, so that percentages can be resolved, see CSS specs Sec. 9.4.11. Seems
				// very slow, we skip this for now.
			}
			else
			{
				item.used_cross_size = item.hypothetical_cross_size;
			}
		}
	}

	// -- Align cross axis (§9.6) --
	for (FlexLine& line : container.lines)
	{
		constexpr float UndefinedBaseline = -FLT_MAX;
		float max_baseline_edge_distance = UndefinedBaseline;
		FlexItem* max_baseline_item = nullptr;

		for (FlexItem& item : line.items)
		{
			const float remaining_space = line.cross_size - item.used_cross_size;

			item.cross_offset = item.cross.margin_a;
			item.cross_baseline_top = UndefinedBaseline;

			const int num_auto_margins = int(item.cross.auto_margin_a) + int(item.cross.auto_margin_b);
			if (num_auto_margins > 0)
			{
				const float space_per_auto_margin = Math::Max(remaining_space, 0.0f) / float(num_auto_margins);
				item.cross_offset = item.cross.margin_a + (item.cross.auto_margin_a ? space_per_auto_margin : 0.f);
			}
			else
			{
				using Style::AlignSelf;
				const AlignSelf align_self = item.align_self;

				switch (align_self)
				{
				case AlignSelf::Auto:
					// Never encountered here: should already have been replaced by container's align-items property.
					RMLUI_ERROR;
					break;
				case AlignSelf::FlexStart:
					// Do nothing, cross offset set above with this behavior.
					break;
				case AlignSelf::FlexEnd: item.cross_offset = item.cross.margin_a + remaining_space; break;
				case AlignSelf::Center: item.cross_offset = item.cross.margin_a + 0.5f * remaining_space; break;
				case AlignSelf::Baseline:
				{
					// We don't currently have a good way to get the true baseline here, so we make a very rough zero-effort approximation.
					const float baseline_heuristic = 0.5f * item.element->GetLineHeight();
					const float sum_edges_top = (wrap_reverse ? item.cross.sum_edges - item.cross.sum_edges_a : item.cross.sum_edges_a);

					item.cross_baseline_top = sum_edges_top + baseline_heuristic;

					const float baseline_edge_distance = (wrap_reverse ? item.used_cross_size - item.cross_baseline_top : item.cross_baseline_top);
					if (baseline_edge_distance > max_baseline_edge_distance)
					{
						max_baseline_item = &item;
						max_baseline_edge_distance = baseline_edge_distance;
					}
				}
				break;
				case AlignSelf::Stretch:
					// Handled above
					break;
				}
			}

			if (wrap_reverse)
			{
				const float reverse_offset = line.cross_size - item.used_cross_size + item.cross.margin_a + item.cross.margin_b;
				item.cross_offset = reverse_offset - item.cross_offset;
			}
		}

		if (max_baseline_item)
		{
			// Align all baseline items such that their baselines are aligned with the one with the max. baseline distance.
			// Cross offset for all baseline items are currently set as in 'flex-start'.
			const float max_baseline_margin_top = (wrap_reverse ? max_baseline_item->cross.margin_b : max_baseline_item->cross.margin_a);
			const float line_top_to_baseline_distance =
				max_baseline_item->cross_offset - max_baseline_margin_top + max_baseline_item->cross_baseline_top;

			for (FlexItem& item : line.items)
			{
				if (item.cross_baseline_top != UndefinedBaseline)
				{
					const float margin_top = (wrap_reverse ? item.cross.margin_b : item.cross.margin_a);
					item.cross_offset = line_top_to_baseline_distance - item.cross_baseline_top + margin_top;
				}
			}
		}

		// Snap the outer item cross edges to the pixel grid.
		for (FlexItem& item : line.items)
			Math::SnapToPixelGrid(item.cross_offset, item.used_cross_size);
	}

	const float accumulated_lines_cross_size = std::accumulate(container.lines.begin(), container.lines.end(), 0.f,
		[](float value, const FlexLine& line) { return value + line.cross_size; });

	// If the available cross size is infinite, the used cross size becomes the accumulated line cross size.
	const float used_cross_size_unconstrained = cross_available_size >= 0.f ? cross_available_size : accumulated_lines_cross_size;
	const float used_cross_size = Math::Clamp(used_cross_size_unconstrained, cross_min_size, cross_max_size);

	// Align the lines along the cross-axis.
	{
		const float remaining_free_space = used_cross_size - accumulated_lines_cross_size;
		const int num_lines = int(container.lines.size());

		if (remaining_free_space > 0.f)
		{
			using Style::AlignContent;

			switch (computed_flex.align_content())
			{
			case AlignContent::SpaceBetween:
				if (num_lines > 1)
				{
					const float space_per_edge = remaining_free_space / float(2 * num_lines - 2);
					for (int i = 0; i < num_lines; i++)
					{
						FlexLine& line = container.lines[i];
						if (i > 0)
							line.cross_spacing_a = space_per_edge;
						if (i < num_lines - 1)
							line.cross_spacing_b = space_per_edge;
					}
				}
				//-fallthrough
			case AlignContent::FlexStart: container.lines.back().cross_spacing_b = remaining_free_space; break;
			case AlignContent::FlexEnd: container.lines.front().cross_spacing_a = remaining_free_space; break;
			case AlignContent::Center:
				container.lines.front().cross_spacing_a = 0.5f * remaining_free_space;
				container.lines.back().cross_spacing_b = 0.5f * remaining_free_space;
				break;
			case AlignContent::SpaceAround:
			{
				const float space_per_edge = remaining_free_space / float(2 * num_lines);
				for (FlexLine& line : container.lines)
				{
					line.cross_spacing_a = space_per_edge;
					line.cross_spacing_b = space_per_edge;
				}
			}
			break;
			case AlignContent::SpaceEvenly:
			{
				const float space_per_edge = remaining_free_space / float(2 * (num_lines + 1));
				for (int i = 0; i < num_lines; i++)
				{
					FlexLine& line = container.lines[i];
					line.cross_spacing_a = space_per_edge;
					line.cross_spacing_b = space_per_edge;
					if (i == 0)
						line.cross_spacing_a *= 2.0f;
					else if (i == num_lines - 1)
						line.cross_spacing_b *= 2.0f;
				}
			}
			break;
			case AlignContent::Stretch:
				// Handled above.
				break;
			}
		}

		// Now find the offset and snap the line edges to the pixel grid.
		float cursor = 0.f;
		for (FlexLine& line : container.lines)
		{
			if (wrap_reverse)
				line.cross_offset = used_cross_size - (cursor + line.cross_spacing_a + line.cross_size);
			else
				line.cross_offset = cursor + line.cross_spacing_a;

			cursor += line.cross_spacing_a + line.cross_size + line.cross_spacing_b;
			Math::SnapToPixelGrid(line.cross_offset, line.cross_size);
		}
	}

	auto MainCrossToVec2 = [main_axis_horizontal](const float v_main, const float v_cross) {
		return main_axis_horizontal ? Vector2f(v_main, v_cross) : Vector2f(v_cross, v_main);
	};

	bool baseline_set = false;

	// -- Format items --
	for (FlexLine& line : container.lines)
	{
		for (FlexItem& item : line.items)
		{
			const Vector2f item_size = MainCrossToVec2(item.used_main_size - item.main.sum_edges, item.used_cross_size - item.cross.sum_edges);
			const Vector2f item_offset = MainCrossToVec2(item.main_offset, line.cross_offset + item.cross_offset);

			item.box.SetContent(item_size);

			UniquePtr<LayoutBox> item_layout_box =
				FormattingContext::FormatIndependent(flex_container_box, item.element, &item.box, FormattingContextType::Block);

			// Set the position of the element within the the flex container
			item.element->SetOffset(flex_content_offset + item_offset, element_flex);

			// The flex container baseline is simply set to the first flex item that has a baseline.
			if (!baseline_set && item_layout_box->GetBaselineOfLastLine(flex_baseline))
			{
				flex_baseline += flex_content_offset.y + item_offset.y;
				baseline_set = true;
			}

			// The cell contents may overflow, propagate this to the flex container.
			const Vector2f overflow_size = item_offset + item_layout_box->GetVisibleOverflowSize();

			flex_content_overflow_size = Math::Max(flex_content_overflow_size, overflow_size);
		}
	}

	flex_resulting_content_size = MainCrossToVec2(used_main_size, used_cross_size);
}

} // namespace Rml
