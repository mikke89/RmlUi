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
#include "../../../Include/RmlUi/Core/Log.h"
#include "../../../Include/RmlUi/Core/Profiling.h"
#include "ContainerBox.h"
#include "FormattingContext.h"

namespace Rml {

void LayoutEngine::FormatElement(Element* element, Vector2f containing_block, Vector2f absolutely_positioning_containing_block, bool allow_cache)
{
	RMLUI_ASSERT(element && containing_block.x >= 0 && containing_block.y >= 0);

	RootBox absolute_root(Box(absolutely_positioning_containing_block), FormattingMode{FormattingMode::Constraint::None, allow_cache});
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

	{
		RMLUI_ZoneScopedN("CommitLayoutRecursive");
		element->CommitLayoutRecursive();
	}
}

} // namespace Rml
