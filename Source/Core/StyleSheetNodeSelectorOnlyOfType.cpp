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

#include "StyleSheetNodeSelectorOnlyOfType.h"
#include "../../Include/RmlUi/Core/ElementText.h"

namespace Rml {

StyleSheetNodeSelectorOnlyOfType::StyleSheetNodeSelectorOnlyOfType()
{
}

StyleSheetNodeSelectorOnlyOfType::~StyleSheetNodeSelectorOnlyOfType()
{
}

// Returns true if the element is the only DOM child of its parent of its type.
bool StyleSheetNodeSelectorOnlyOfType::IsApplicable(const Element* element, int RMLUI_UNUSED_PARAMETER(a), int RMLUI_UNUSED_PARAMETER(b))
{
	RMLUI_UNUSED(a);
	RMLUI_UNUSED(b);

	Element* parent = element->GetParentNode();
	if (parent == nullptr)
		return false;

	for (int i = 0; i < parent->GetNumChildren(); ++i)
	{
		Element* child = parent->GetChild(i);

		// Skip the child if it is our element.
		if (child == element)
			continue;

		// Skip the child if it does not share our tag.
		if (child->GetTagName() != element->GetTagName() ||
			child->GetDisplay() == Style::Display::None)
			continue;

		// We've found a similarly-tagged child to our element; selector fails.
		return false;
	}

	return true;
}

} // namespace Rml
