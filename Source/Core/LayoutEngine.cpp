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
#include "LayoutBlockBoxSpace.h"
#include "LayoutDetails.h"
#include "LayoutInlineBoxText.h"
#include "Pool.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/Profiling.h"
#include "../../Include/RmlUi/Core/Types.h"
#include <cstddef>

namespace Rml {

#define MAX(a, b) (a > b ? a : b)

struct LayoutChunk
{
	static const unsigned int size = MAX(sizeof(LayoutBlockBox), MAX(sizeof(LayoutInlineBox), MAX(sizeof(LayoutInlineBoxText), MAX(sizeof(LayoutLineBox), sizeof(LayoutBlockBoxSpace)))));
	alignas(std::max_align_t) char buffer[size];
};

static Pool< LayoutChunk > layout_chunk_pool(200, true);

// Formats the contents for a root-level element (usually a document or floating element).
bool LayoutEngine::FormatElement(Element* element, Vector2f containing_block)
{
#ifdef RMLUI_ENABLE_PROFILING
	RMLUI_ZoneScopedC(0xB22222);
	auto name = CreateString(80, "%s %x", element->GetAddress(false, false).c_str(), element);
	RMLUI_ZoneName(name.c_str(), name.size());
#endif

	LayoutBlockBox containing_block_box(nullptr, nullptr);
	containing_block_box.GetBox().SetContent(containing_block);

	LayoutBlockBox* block_context_box = containing_block_box.AddBlockElement(element);

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
	element->OnLayout();

	return true;
}

void* LayoutEngine::AllocateLayoutChunk(size_t size)
{
	RMLUI_ASSERT(size <= LayoutChunk::size);
	(void)size;
	
	return layout_chunk_pool.AllocateAndConstruct();
}

void LayoutEngine::DeallocateLayoutChunk(void* chunk)
{
	layout_chunk_pool.DestroyAndDeallocate((LayoutChunk*) chunk);
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

	// Fetch the display property, and don't lay this element out if it is set to a display type of none.
	if (computed.display == Style::Display::None)
		return true;

	// Check for an absolute position; if this has been set, then we remove it from the flow and add it to the current
	// block box to be laid out and positioned once the block has been closed and sized.
	if (computed.position == Style::Position::Absolute || computed.position == Style::Position::Fixed)
	{
		// Display the element as a block element.
		block_context_box->AddAbsoluteElement(element);
		return true;
	}

	// If the element is floating, we remove it from the flow.
	if (computed.float_ != Style::Float::None)
	{
		LayoutEngine::FormatElement(element, LayoutDetails::GetContainingBlock(block_context_box));
		return block_context_box->AddFloatElement(element);
	}

	// The element is nothing exceptional, so we treat it as a normal block, inline or replaced element.
	switch (computed.display)
	{
		case Style::Display::Block:       return FormatElementBlock(block_context_box, element); break;
		case Style::Display::Inline:      return FormatElementInline(block_context_box, element); break;
		case Style::Display::InlineBlock: return FormatElementInlineBlock(block_context_box, element); break;
		default: RMLUI_ERROR;
	}

	return true;
}

// Formats and positions an element as a block element.
bool LayoutEngine::FormatElementBlock(LayoutBlockBox* block_context_box, Element* element)
{
	RMLUI_ZoneScopedC(0x2F4F4F);

	LayoutBlockBox* new_block_context_box = block_context_box->AddBlockElement(element);
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

	Box box;
	float min_height, max_height;
	LayoutDetails::BuildBox(box, min_height, max_height, block_context_box, element, true);
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
