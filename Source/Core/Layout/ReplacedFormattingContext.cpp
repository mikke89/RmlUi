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

#include "ReplacedFormattingContext.h"
#include "../../../Include/RmlUi/Core/ComputedValues.h"
#include "../../../Include/RmlUi/Core/Element.h"
#include "BlockFormattingContext.h"
#include "ContainerBox.h"
#include "LayoutDetails.h"

namespace Rml {

UniquePtr<LayoutBox> ReplacedFormattingContext::Format(ContainerBox* parent_container, Element* element, const Box* override_initial_box)
{
	RMLUI_ASSERT(element->IsReplaced());

	// Replaced elements provide their own rendering, we just set their box here and notify them that the element has been sized.
	auto replaced_box = MakeUnique<ReplacedBox>(element);
	Box& box = replaced_box->GetBox();
	if (override_initial_box)
		box = *override_initial_box;
	else
	{
		const Vector2f containing_block = LayoutDetails::GetContainingBlock(parent_container, element->GetPosition()).size;
		LayoutDetails::BuildBox(box, containing_block, element);
	}

	// Submit the box and notify the element.
	replaced_box->Close();

	// Usually, replaced elements add children to the hidden DOM. If we happen to have any normal DOM children, e.g.
	// added by the user, we format them using normal block formatting rules. Since replaced elements provide their
	// own rendering, this could cause conflicting or strange layout results, and is done at the user's own risk.
	if (element->HasChildNodes())
	{
		RootBox root(box);
		BlockFormattingContext::Format(&root, element, &box);
	}

	return replaced_box;
}

void ReplacedBox::Close()
{
	element->SetBox(box);
	element->OnLayout();
}

String ReplacedBox::DebugDumpTree(int depth) const
{
	return String(depth * 2, ' ') + "ReplacedBox";
}

} // namespace Rml
