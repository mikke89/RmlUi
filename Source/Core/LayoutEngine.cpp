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

#include "LayoutEngine.h"
#include "../../Include/RmlUi/Core/ComputedValues.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/Profiling.h"
#include "../../Include/RmlUi/Core/Types.h"
#include "LayoutBlockBoxSpace.h"
#include "LayoutDetails.h"
#include "LayoutFlex.h"
#include "LayoutInlineBoxText.h"
#include "LayoutTable.h"
#include "Pool.h"
#include <cstddef>
#include <float.h>

namespace Rml {

#define MAX(a, b) (a > b ? a : b)

template <size_t Size>
struct LayoutChunk {
	alignas(std::max_align_t) byte buffer[Size];
};

static constexpr std::size_t ChunkSizeBig = sizeof(LayoutBlockBox);
static constexpr std::size_t ChunkSizeMedium = MAX(sizeof(LayoutInlineBox), sizeof(LayoutInlineBoxText));
static constexpr std::size_t ChunkSizeSmall = MAX(sizeof(LayoutLineBox), sizeof(LayoutBlockBoxSpace));

static Pool< LayoutChunk<ChunkSizeBig> > layout_chunk_pool_big(50, true);
static Pool< LayoutChunk<ChunkSizeMedium> > layout_chunk_pool_medium(50, true);
static Pool< LayoutChunk<ChunkSizeSmall> > layout_chunk_pool_small(50, true);


// Formats the contents for a root-level element (usually a document or floating element).
void LayoutEngine::FormatElement(Element* element, Vector2f containing_block, const Box* override_initial_box, Vector2f* out_visible_overflow_size)
{
	RMLUI_ASSERT(element && containing_block.x >= 0 && containing_block.y >= 0);
#ifdef RMLUI_ENABLE_PROFILING
	RMLUI_ZoneScopedC(0xB22222);
	auto name = CreateString(80, "%s %x", element->GetAddress(false, false).c_str(), element);
	RMLUI_ZoneName(name.c_str(), name.size());
#endif

	auto containing_block_box = MakeUnique<LayoutBlockBox>(nullptr, nullptr, Box(containing_block), 0.0f, FLT_MAX);

	Box box;
	if (override_initial_box)
		box = *override_initial_box;
	else
		LayoutDetails::BuildBox(box, containing_block, element);

	float min_height, max_height;
	LayoutDetails::GetDefiniteMinMaxHeight(min_height, max_height, element->GetComputedValues(), box, containing_block.y);

	LayoutBlockBox* block_context_box = containing_block_box->AddBlockElement(element, box, min_height, max_height);

	for (int layout_iteration = 0; layout_iteration < 2; layout_iteration++)
	{
		for (int i = 0; i < element->GetNumChildren(); i++)
		{
			if (!FormatElement(block_context_box, element->GetChild(i)))
				i = -1;
		}

		if (block_context_box->Close() == LayoutBlockBox::OK)
			break;
	}

	block_context_box->CloseAbsoluteElements();

	if (out_visible_overflow_size)
		*out_visible_overflow_size = block_context_box->GetVisibleOverflowSize();

	element->OnLayout();
}

void* LayoutEngine::AllocateLayoutChunk(size_t size)
{
	static_assert(ChunkSizeBig > ChunkSizeMedium && ChunkSizeMedium > ChunkSizeSmall, "The following assumes a strict ordering of the chunk sizes.");

	// Note: If any change is made here, make sure a corresponding change is applied to the deallocation procedure below.
	if (size <= ChunkSizeSmall)
		return layout_chunk_pool_small.AllocateAndConstruct();
	else if (size <= ChunkSizeMedium)
		return layout_chunk_pool_medium.AllocateAndConstruct();
	else if (size <= ChunkSizeBig)
		return layout_chunk_pool_big.AllocateAndConstruct();

	RMLUI_ERROR;
	return nullptr;
}

void LayoutEngine::DeallocateLayoutChunk(void* chunk, size_t size)
{
	// Note: If any change is made here, make sure a corresponding change is applied to the allocation procedure above.
	if (size <= ChunkSizeSmall)
		layout_chunk_pool_small.DestroyAndDeallocate((LayoutChunk<ChunkSizeSmall>*)chunk);
	else if (size <= ChunkSizeMedium)
		layout_chunk_pool_medium.DestroyAndDeallocate((LayoutChunk<ChunkSizeMedium>*)chunk);
	else if (size <= ChunkSizeBig)
		layout_chunk_pool_big.DestroyAndDeallocate((LayoutChunk<ChunkSizeBig>*)chunk);
	else
	{
		RMLUI_ERROR;
	}
}

// Positions a single element and its children within this layout.
bool LayoutEngine::FormatElement(LayoutBlockBox* block_context_box, Element* element)
{
#ifdef RMLUI_ENABLE_PROFILING
	RMLUI_ZoneScoped;
	auto name = CreateString(80, ">%s %x", element->GetAddress(false, false).c_str(), element);
	RMLUI_ZoneName(name.c_str(), name.size());
#endif

	auto& computed = element->GetComputedValues();

	// Check if we have to do any special formatting for any elements that don't fit into the standard layout scheme.
	if (FormatElementSpecial(block_context_box, element))
		return true;

	const Style::Display display = element->GetDisplay();

	// Fetch the display property, and don't lay this element out if it is set to a display type of none.
	if (display == Style::Display::None)
		return true;

	// Tables and flex boxes need to be specially handled when they are absolutely positioned or floated. Currently it is assumed for both
	// FormatElement(element, containing_block) and GetShrinkToFitWidth(), and possibly others, that they are strictly called on block boxes.
	// The mentioned functions need to be updated if we want to support all combinations of display, position, and float properties.
	auto uses_unsupported_display_position_float_combination = [display, element](const char* abs_positioned_or_floated) -> bool {
		if (display == Style::Display::Flex || display == Style::Display::Table)
		{
			const char* element_type = (display == Style::Display::Flex ? "Flex" : "Table");
			Log::Message(Log::LT_WARNING,
				"%s elements cannot be %s. Instead, wrap it within a parent block element which is %s. Element will not be formatted: %s",
				element_type, abs_positioned_or_floated, abs_positioned_or_floated, element->GetAddress().c_str());
			return true;
		}
		return false;
	};

	// Check for an absolute position; if this has been set, then we remove it from the flow and add it to the current
	// block box to be laid out and positioned once the block has been closed and sized.
	if (computed.position() == Style::Position::Absolute || computed.position() == Style::Position::Fixed)
	{
		if (uses_unsupported_display_position_float_combination("absolutely positioned"))
			return true;

		// Display the element as a block element.
		block_context_box->AddAbsoluteElement(element);
		return true;
	}

	// If the element is floating, we remove it from the flow.
	if (computed.float_() != Style::Float::None)
	{
		if (uses_unsupported_display_position_float_combination("floated"))
			return true;

		LayoutEngine::FormatElement(element, LayoutDetails::GetContainingBlock(block_context_box));
		return block_context_box->AddFloatElement(element);
	}

	// The element is nothing exceptional, so format it according to its display property.
	switch (display)
	{
		case Style::Display::Block:       return FormatElementBlock(block_context_box, element);
		case Style::Display::Inline:      return FormatElementInline(block_context_box, element);
		case Style::Display::InlineBlock: return FormatElementInlineBlock(block_context_box, element);
		case Style::Display::Flex:        return FormatElementFlex(block_context_box, element);
		case Style::Display::Table:       return FormatElementTable(block_context_box, element);

		case Style::Display::TableRow:
		case Style::Display::TableRowGroup:
		case Style::Display::TableColumn:
		case Style::Display::TableColumnGroup:
		case Style::Display::TableCell:
		{
			// These elements should have been handled within FormatElementTable, seems like we're encountering table parts in the wild.
			const Property* display_property = element->GetProperty(PropertyId::Display);
			Log::Message(Log::LT_WARNING, "Element has a display type '%s', but is not located in a table. Element will not be formatted: %s",
				display_property ? display_property->ToString().c_str() : "*unknown*",
				element->GetAddress().c_str()
			);
			return true;
		}
		case Style::Display::None:        RMLUI_ERROR; /* handled above */ break;
	}

	return true;
}

// Formats and positions an element as a block element.
bool LayoutEngine::FormatElementBlock(LayoutBlockBox* block_context_box, Element* element)
{
	RMLUI_ZoneScopedC(0x2F4F4F);

	Box box;
	float min_height, max_height;
	LayoutDetails::BuildBox(box, min_height, max_height, block_context_box, element);

	LayoutBlockBox* new_block_context_box = block_context_box->AddBlockElement(element, box, min_height, max_height);
	if (new_block_context_box == nullptr)
		return false;

	// Format the element's children.
	for (int i = 0; i < element->GetNumChildren(); i++)
	{
		if (!FormatElement(new_block_context_box, element->GetChild(i)))
			i = -1;
	}

	// Close the block box, and check the return code; we may have overflowed either this element or our parent.
	switch (new_block_context_box->Close())
	{
		// We need to reformat ourself; format all of our children again and close the box. No need to check for error
		// codes, as we already have our vertical slider bar.
		case LayoutBlockBox::LAYOUT_SELF:
		{
			for (int i = 0; i < element->GetNumChildren(); i++)
				FormatElement(new_block_context_box, element->GetChild(i));

			if (new_block_context_box->Close() == LayoutBlockBox::OK)
			{
				element->OnLayout();
				break;
			}
		}
		//-fallthrough
		// We caused our parent to add a vertical scrollbar; bail out!
		case LayoutBlockBox::LAYOUT_PARENT:
		{
			return false;
		}
		break;

		default:
			element->OnLayout();
	}

	return true;
}

// Formats and positions an element as an inline element.
bool LayoutEngine::FormatElementInline(LayoutBlockBox* block_context_box, Element* element)
{
	RMLUI_ZoneScopedC(0x3F6F6F);

	const Vector2f containing_block = LayoutDetails::GetContainingBlock(block_context_box);

	Box box;
	LayoutDetails::BuildBox(box, containing_block, element, BoxContext::Inline);
	LayoutInlineBox* inline_box = block_context_box->AddInlineElement(element, box);

	// Format the element's children.
	for (int i = 0; i < element->GetNumChildren(); i++)
	{
		if (!FormatElement(block_context_box, element->GetChild(i)))
			return false;
	}

	inline_box->Close();

	return true;
}

// Positions an element as a sized inline element, formatting its internal hierarchy as a block element.
bool LayoutEngine::FormatElementInlineBlock(LayoutBlockBox* block_context_box, Element* element)
{
	RMLUI_ZoneScopedC(0x1F2F2F);

	// Format the element separately as a block element, then position it inside our own layout as an inline element.
	Vector2f containing_block_size = LayoutDetails::GetContainingBlock(block_context_box);

	FormatElement(element, containing_block_size);

	block_context_box->AddInlineElement(element, element->GetBox())->Close();

	return true;
}

bool LayoutEngine::FormatElementFlex(LayoutBlockBox* block_context_box, Element* element)
{
	const ComputedValues& computed = element->GetComputedValues();
	const Vector2f containing_block = LayoutDetails::GetContainingBlock(block_context_box);
	RMLUI_ASSERT(containing_block.x >= 0.f);

	// Build the initial box as specified by the flex's style, as if it was a normal block element.
	Box box;
	LayoutDetails::BuildBox(box, containing_block, element, BoxContext::Block);

	Vector2f min_size, max_size;
	LayoutDetails::GetMinMaxWidth(min_size.x, max_size.x, computed, box, containing_block.x);
	LayoutDetails::GetMinMaxHeight(min_size.y, max_size.y, computed, box, containing_block.y);

	// Add the flex container element as if it was a normal block element.
	LayoutBlockBox* flex_block_context_box = block_context_box->AddBlockElement(element, box, min_size.y, max_size.y);
	if (!flex_block_context_box)
		return false;

	// Format the flexbox and all its children.
	ElementList absolutely_positioned_elements;
	Vector2f formatted_content_size, content_overflow_size;
	LayoutFlex::Format(
		box, min_size, max_size, containing_block, element, formatted_content_size, content_overflow_size, absolutely_positioned_elements);

	// Set the box content size to match the one determined by the formatting procedure.
	flex_block_context_box->GetBox().SetContent(formatted_content_size);
	// Set the inner content size so that any overflow can be caught.
	flex_block_context_box->ExtendInnerContentSize(content_overflow_size);

	// Finally, add any absolutely positioned flex children.
	for (Element* abs_element : absolutely_positioned_elements)
		flex_block_context_box->AddAbsoluteElement(abs_element);

	// Close the block box, this may result in scrollbars being added to ourself or our parent.
	const auto close_result = flex_block_context_box->Close();
	if (close_result == LayoutBlockBox::LAYOUT_PARENT)
	{
		// Scollbars added to parent, bail out to reformat all its children.
		return false;
	}
	else if (close_result == LayoutBlockBox::LAYOUT_SELF)
	{
		// Scrollbars added to flex container, it needs to be formatted again to account for changed width or height.
		absolutely_positioned_elements.clear();

		LayoutFlex::Format(
			box, min_size, max_size, containing_block, element, formatted_content_size, content_overflow_size, absolutely_positioned_elements);

		flex_block_context_box->GetBox().SetContent(formatted_content_size);
		flex_block_context_box->ExtendInnerContentSize(content_overflow_size);

		if (flex_block_context_box->Close() == LayoutBlockBox::LAYOUT_PARENT)
			return false;
	}

	element->OnLayout();

	return true;
}


bool LayoutEngine::FormatElementTable(LayoutBlockBox* block_context_box, Element* element_table)
{
	const ComputedValues& computed_table = element_table->GetComputedValues();

	const Vector2f containing_block = LayoutDetails::GetContainingBlock(block_context_box);

	// Build the initial box as specified by the table's style, as if it was a normal block element.
	Box box;
	LayoutDetails::BuildBox(box, containing_block, element_table, BoxContext::Block);

	Vector2f min_size, max_size;
	LayoutDetails::GetMinMaxWidth(min_size.x, max_size.x, computed_table, box, containing_block.x);
	LayoutDetails::GetMinMaxHeight(min_size.y, max_size.y, computed_table, box, containing_block.y);
	const Vector2f initial_content_size = box.GetSize();

	// Format the table, this may adjust the box content size.
	const Vector2f table_content_overflow_size = LayoutTable::FormatTable(box, min_size, max_size, element_table);

	const Vector2f final_content_size = box.GetSize();
	RMLUI_ASSERT(final_content_size.y >= 0);

	if (final_content_size != initial_content_size)
	{
		// Perform this step to re-evaluate any auto margins.
		LayoutDetails::BuildBoxSizeAndMargins(box, min_size, max_size, containing_block, element_table, BoxContext::Block, true);
	}

	// Now that the box is finalized, we can add table as a block element. If we did it earlier, eg. just before formatting the table,
	// then the table element's offset would not be correct in cases where table size and auto-margins were adjusted.
	LayoutBlockBox* table_block_context_box = block_context_box->AddBlockElement(element_table, box, final_content_size.y, final_content_size.y);
	if (!table_block_context_box)
		return false;

	// Set the inner content size so that any overflow can be caught.
	table_block_context_box->ExtendInnerContentSize(table_content_overflow_size);

	// If the close failed, it probably means that its parent produced scrollbars.
	if (table_block_context_box->Close() != LayoutBlockBox::OK)
		return false;

	return true;
}

// Executes any special formatting for special elements.
bool LayoutEngine::FormatElementSpecial(LayoutBlockBox* block_context_box, Element* element)
{
	static const String br("br");
	
	// Check for a <br> tag.
	if (element->GetTagName() == br)
	{
		block_context_box->AddBreak();
		element->OnLayout();
		return true;
	}

	return false;
}

} // namespace Rml
