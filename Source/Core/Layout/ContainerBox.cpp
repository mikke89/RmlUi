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

#include "ContainerBox.h"
#include "../../../Include/RmlUi/Core/ComputedValues.h"
#include "../../../Include/RmlUi/Core/Element.h"
#include "../../../Include/RmlUi/Core/ElementScroll.h"
#include "../../../Include/RmlUi/Core/Profiling.h"
#include "FlexFormattingContext.h"
#include "FormattingContext.h"
#include "LayoutDetails.h"
#include <algorithm>
#include <cmath>

namespace Rml {

void ContainerBox::ResetScrollbars(const Box& box)
{
	RMLUI_ASSERT(element);
	if (overflow_x == Style::Overflow::Scroll)
		element->GetElementScroll()->EnableScrollbar(ElementScroll::HORIZONTAL, box.GetSizeAcross(BoxDirection::Horizontal, BoxArea::Padding));
	else
		element->GetElementScroll()->DisableScrollbar(ElementScroll::HORIZONTAL);

	if (overflow_y == Style::Overflow::Scroll)
		element->GetElementScroll()->EnableScrollbar(ElementScroll::VERTICAL, box.GetSizeAcross(BoxDirection::Horizontal, BoxArea::Padding));
	else
		element->GetElementScroll()->DisableScrollbar(ElementScroll::VERTICAL);
}

void ContainerBox::AddAbsoluteElement(Element* element, Vector2f static_position, Element* static_relative_offset_parent)
{
	// We may possibly be adding the same element from a previous layout iteration. If so, this ensures it is updated with the latest static position.
	absolute_elements[element] = AbsoluteElement{static_position, static_relative_offset_parent};
}

void ContainerBox::AddRelativeElement(Element* element)
{
	// The same relative element may be added multiple times during repeated layout iterations, avoid any duplicates.
	if (std::find(relative_elements.begin(), relative_elements.end(), element) == relative_elements.end())
		relative_elements.push_back(element);
}

void ContainerBox::ClosePositionedElements()
{
	// Any relatively positioned elements that we act as containing block for may need to be have their positions
	// updated to reflect changes to the size of this block box. Update relative offsets before handling absolute
	// elements, as this may affect the resolved static position of the absolute elements.
	for (Element* child : relative_elements)
		child->UpdateOffset();

	relative_elements.clear();

	while (!absolute_elements.empty())
	{
		// New absolute elements may be added to this box during formatting below. To avoid invalidated iterators and
		// references, move the list to a local copy to iterate over, and repeat if new elements are added.
		AbsoluteElementMap absolute_elements_iterate = std::move(absolute_elements);
		absolute_elements.clear();

		for (const auto& absolute_element_pair : absolute_elements_iterate)
		{
			Element* absolute_element = absolute_element_pair.first;
			const Vector2f static_position = absolute_element_pair.second.static_position;
			Element* static_position_offset_parent = absolute_element_pair.second.static_position_offset_parent;

			// Find the static position relative to this containing block. First, calculate the offset from ourself to
			// the static position's offset parent. Assumes (1) that this container box is part of the containing block
			// chain of the static position offset parent, and (2) that all offsets in this chain has been set already.
			Vector2f relative_position;
			for (Element* ancestor = static_position_offset_parent; ancestor && ancestor != element; ancestor = ancestor->GetOffsetParent())
				relative_position += ancestor->GetRelativeOffset(BoxArea::Border);

			// Now simply add the result to the stored static position to get the static position in our local space.
			Vector2f offset = relative_position + static_position;

			// Lay out the element.
			FormattingContext::FormatIndependent(this, absolute_element, nullptr, FormattingContextType::Block);

			// Now that the element's box has been built, we can offset the position we determined was appropriate for
			// it by the element's margin. This is necessary because the coordinate system for the box begins at the
			// border, not the margin.
			offset.x += absolute_element->GetBox().GetEdge(BoxArea::Margin, BoxEdge::Left);
			offset.y += absolute_element->GetBox().GetEdge(BoxArea::Margin, BoxEdge::Top);

			// Set the offset of the element; the element itself will take care of any RCSS-defined positional offsets.
			absolute_element->SetOffset(offset, element);
		}
	}
}

void ContainerBox::SetElementBaseline(float element_baseline)
{
	element->SetBaseline(element_baseline);
}

void ContainerBox::SubmitElementLayout()
{
	element->OnLayout();
}

ContainerBox::ContainerBox(Type type, Element* element, ContainerBox* parent_container) :
	LayoutBox(type), element(element), parent_container(parent_container)
{
	if (element)
	{
		const auto& computed = element->GetComputedValues();
		overflow_x = computed.overflow_x();
		overflow_y = computed.overflow_y();
		is_absolute_positioning_containing_block = (computed.position() != Style::Position::Static || computed.has_local_transform() ||
			computed.has_local_perspective() || computed.has_filter() || computed.has_backdrop_filter() || computed.has_mask_image());
	}
}

bool ContainerBox::CatchOverflow(const Vector2f content_overflow_size, const Box& box, const float max_height) const
{
	if (!IsScrollContainer())
		return true;

	const Vector2f padding_bottom_right = {box.GetEdge(BoxArea::Padding, BoxEdge::Right), box.GetEdge(BoxArea::Padding, BoxEdge::Bottom)};
	const float padding_width = box.GetSizeAcross(BoxDirection::Horizontal, BoxArea::Padding);

	Vector2f available_space = box.GetSize();
	if (available_space.y < 0.f)
		available_space.y = max_height;
	if (available_space.y < 0.f)
		available_space.y = HUGE_VALF;

	RMLUI_ASSERT(available_space.x >= 0.f && available_space.y >= 0.f);

	// Allow overflow onto the padding area.
	available_space += padding_bottom_right;

	ElementScroll* element_scroll = element->GetElementScroll();
	bool scrollbar_size_changed = false;

	// @performance If we have auto-height sizing and the horizontal scrollbar is enabled, then we can in principle
	// simply add the scrollbar size to the height instead of formatting the element all over again.
	if (overflow_x == Style::Overflow::Auto && content_overflow_size.x > available_space.x + 0.5f)
	{
		if (element_scroll->GetScrollbarSize(ElementScroll::HORIZONTAL) == 0.f)
		{
			element_scroll->EnableScrollbar(ElementScroll::HORIZONTAL, padding_width);
			const float new_size = element_scroll->GetScrollbarSize(ElementScroll::HORIZONTAL);
			scrollbar_size_changed = (new_size != 0.f);
			available_space.y -= new_size;
		}
	}

	// If we're auto-scrolling and our height is fixed, we have to check if this box has exceeded our client height.
	if (overflow_y == Style::Overflow::Auto && content_overflow_size.y > available_space.y + 0.5f)
	{
		if (element_scroll->GetScrollbarSize(ElementScroll::VERTICAL) == 0.f)
		{
			element_scroll->EnableScrollbar(ElementScroll::VERTICAL, padding_width);
			const float new_size = element_scroll->GetScrollbarSize(ElementScroll::VERTICAL);
			scrollbar_size_changed |= (new_size != 0.f);
		}
	}

	return !scrollbar_size_changed;
}

bool ContainerBox::SubmitBox(const Vector2f content_overflow_size, const Box& box, const float max_height)
{
	Vector2f visible_overflow_size;

	// Set the computed box on the element.
	if (element)
	{
		// Calculate the dimensions of the box's scrollable overflow rectangle. This is the union of the tightest-
		// fitting box around all of the internal elements, and this element's padding box. We really only care about
		// overflow on the bottom-right sides, as these are the only ones allowed to be scrolled to in CSS.
		//
		// If we are a scroll container (use any other value than 'overflow: visible'), then any overflow outside our
		// padding box should be caught here. Otherwise, our overflow should be included in the overflow calculations of
		// our nearest scroll container ancestor.

		// If our content is larger than our padding box, we can add scrollbars if we're set to auto-scrollbars. If
		// we're set to always use scrollbars, then the scrollbars have already been enabled.
		if (!CatchOverflow(content_overflow_size, box, max_height))
			return false;

		const Vector2f padding_top_left = {box.GetEdge(BoxArea::Padding, BoxEdge::Left), box.GetEdge(BoxArea::Padding, BoxEdge::Top)};
		const Vector2f padding_bottom_right = {box.GetEdge(BoxArea::Padding, BoxEdge::Right), box.GetEdge(BoxArea::Padding, BoxEdge::Bottom)};
		const Vector2f padding_size = box.GetSize() + padding_top_left + padding_bottom_right;

		const bool is_scroll_container = IsScrollContainer();
		const Vector2f scrollbar_size = {
			is_scroll_container ? element->GetElementScroll()->GetScrollbarSize(ElementScroll::VERTICAL) : 0.f,
			is_scroll_container ? element->GetElementScroll()->GetScrollbarSize(ElementScroll::HORIZONTAL) : 0.f,
		};

		element->SetBox(box);

		// Scrollable overflow is the set of things extending our padding area, for which scrolling could be provided.
		const Vector2f scrollable_overflow_size = Math::Max(padding_size - scrollbar_size, padding_top_left + content_overflow_size);
		// Set the overflow size but defer clamping of the scroll offset, see `LayoutEngine::FormatElement`.
		element->SetScrollableOverflowRectangle(scrollable_overflow_size, false);

		const Vector2f border_size = padding_size + box.GetFrameSize(BoxArea::Border);

		// Set the visible overflow size so that ancestors can catch any overflow produced by us. That is, hiding it or
		// providing a scrolling mechanism. If this box is a scroll container we catch our own overflow here. Thus, in
		// this case, only our border box is visible from our ancestor's perpective.
		if (is_scroll_container)
		{
			visible_overflow_size = border_size;

			// Format any scrollbars in case they were enabled on this element.
			element->GetElementScroll()->FormatScrollbars();
		}
		else
		{
			const Vector2f border_top_left = {box.GetEdge(BoxArea::Border, BoxEdge::Left), box.GetEdge(BoxArea::Border, BoxEdge::Top)};
			visible_overflow_size = Math::Max(border_size, content_overflow_size + border_top_left + padding_top_left);
		}
	}

	SetVisibleOverflowSize(visible_overflow_size);

	return true;
}

String RootBox::DebugDumpTree(int depth) const
{
	return String(depth * 2, ' ') + "RootBox";
}

FlexContainer::FlexContainer(Element* element, ContainerBox* parent_container) : ContainerBox(Type::FlexContainer, element, parent_container)
{
	RMLUI_ASSERT(element);
}

bool FlexContainer::Close(const Vector2f content_overflow_size, const Box& box, float element_baseline)
{
	if (!SubmitBox(content_overflow_size, box, -1.f))
		return false;

	ClosePositionedElements();

	SubmitElementLayout();
	SetElementBaseline(element_baseline);
	return true;
}

float FlexContainer::GetShrinkToFitWidth() const
{
	// For the trivial case of a fixed width, we simply return that.
	if (element->GetComputedValues().width().type == Style::Width::Type::Length)
		return box.GetSize().x;

	// Infer shrink-to-fit width from the intrinsic width of the element.
	return FlexFormattingContext::GetMaxContentSize(element).x;
}

String FlexContainer::DebugDumpTree(int depth) const
{
	return String(depth * 2, ' ') + "FlexContainer" + " | " + LayoutDetails::GetDebugElementName(element);
}

TableWrapper::TableWrapper(Element* element, ContainerBox* parent_container) : ContainerBox(Type::TableWrapper, element, parent_container)
{
	RMLUI_ASSERT(element);
}

void TableWrapper::Close(const Vector2f content_overflow_size, const Box& box, float element_baseline)
{
	bool result = SubmitBox(content_overflow_size, box, -1.f);

	// Since the table wrapper cannot generate scrollbars, this should always pass.
	RMLUI_ASSERT(result);
	(void)result;

	ClosePositionedElements();

	SubmitElementLayout();
	SetElementBaseline(element_baseline);
}

float TableWrapper::GetShrinkToFitWidth() const
{
	// We don't currently support shrink-to-fit layout of tables. However, for the trivial case of a fixed width, we
	// simply return that.
	if (element->GetComputedValues().width().type == Style::Width::Type::Length)
		return box.GetSize().x;

	return 0.0f;
}

String TableWrapper::DebugDumpTree(int depth) const
{
	return String(depth * 2, ' ') + "TableWrapper" + " | " + LayoutDetails::GetDebugElementName(element);
}

} // namespace Rml
