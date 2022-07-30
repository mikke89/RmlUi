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

#include "StyleSheetSelector.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "StyleSheetNode.h"

namespace Rml {

static inline bool IsTextElement(const Element* element)
{
	return element->GetTagName() == "#text";
}

// Returns true if a positive integer can be found for n in the equation an + b = count.
static bool IsNth(int a, int b, int count)
{
	int x = count;
	x -= b;
	if (a != 0)
		x /= a;

	return (x >= 0 && x * a + b == count);
}

bool IsSelectorApplicable(const Element* element, const StructuralSelector& selector)
{
	RMLUI_ASSERT(element);

	switch (selector.type)
	{
	case StructuralSelectorType::Nth_Child:
	{
		Element* parent = element->GetParentNode();
		if (!parent)
			return false;

		// Start counting elements until we find this one.
		int element_index = 1;
		for (int i = 0; i < parent->GetNumChildren(); i++)
		{
			Element* child = parent->GetChild(i);

			// Skip text nodes.
			if (IsTextElement(child))
				continue;

			// If we've found our element, then break; the current index is our element's index.
			if (child == element)
				break;

			element_index++;
		}

		return IsNth(selector.a, selector.b, element_index);
	}
	break;
	case StructuralSelectorType::Nth_Last_Child:
	{
		Element* parent = element->GetParentNode();
		if (!parent)
			return false;

		// Start counting elements until we find this one.
		int element_index = 1;
		for (int i = parent->GetNumChildren() - 1; i >= 0; --i)
		{
			Element* child = parent->GetChild(i);

			// Skip text nodes.
			if (IsTextElement(child))
				continue;

			// If we've found our element, then break; the current index is our element's index.
			if (child == element)
				break;

			element_index++;
		}

		return IsNth(selector.a, selector.b, element_index);
	}
	break;
	case StructuralSelectorType::Nth_Of_Type:
	{
		Element* parent = element->GetParentNode();
		if (!parent)
			return false;

		// Start counting elements until we find this one.
		int element_index = 1;
		const int num_children = parent->GetNumChildren();
		for (int i = 0; i < num_children; i++)
		{
			Element* child = parent->GetChild(i);

			// If we've found our element, then break; the current index is our element's index.
			if (child == element)
				break;

			// Skip nodes that don't share our tag.
			if (child->GetTagName() != element->GetTagName())
				continue;

			element_index++;
		}

		return IsNth(selector.a, selector.b, element_index);
	}
	break;
	case StructuralSelectorType::Nth_Last_Of_Type:
	{
		Element* parent = element->GetParentNode();
		if (!parent)
			return false;

		// Start counting elements until we find this one.
		int element_index = 1;
		for (int i = parent->GetNumChildren() - 1; i >= 0; --i)
		{
			Element* child = parent->GetChild(i);

			// If we've found our element, then break; the current index is our element's index.
			if (child == element)
				break;

			// Skip nodes that don't share our tag.
			if (child->GetTagName() != element->GetTagName())
				continue;

			element_index++;
		}

		return IsNth(selector.a, selector.b, element_index);
	}
	break;
	case StructuralSelectorType::First_Child:
	{
		Element* parent = element->GetParentNode();
		if (!parent)
			return false;

		int child_index = 0;
		while (child_index < parent->GetNumChildren())
		{
			// If this child (the first non-text child) is our element, then the selector succeeds.
			Element* child = parent->GetChild(child_index);
			if (child == element)
				return true;

			// If this child is not a text element, then the selector fails; this element is non-trivial.
			if (!IsTextElement(child))
				return false;

			// Otherwise, skip over the text element to find the last non-trivial element.
			child_index++;
		}

		return false;
	}
	break;
	case StructuralSelectorType::Last_Child:
	{
		Element* parent = element->GetParentNode();
		if (!parent)
			return false;

		int child_index = parent->GetNumChildren() - 1;
		while (child_index >= 0)
		{
			// If this child (the last non-text child) is our element, then the selector succeeds.
			Element* child = parent->GetChild(child_index);
			if (child == element)
				return true;

			// If this child is not a text element, then the selector fails; this element is non-trivial.
			if (!IsTextElement(child))
				return false;

			// Otherwise, skip over the text element to find the last non-trivial element.
			child_index--;
		}

		return false;
	}
	break;
	case StructuralSelectorType::First_Of_Type:
	{
		Element* parent = element->GetParentNode();
		if (!parent)
			return false;

		int child_index = 0;
		while (child_index < parent->GetNumChildren())
		{
			// If this child is our element, then it's the first one we've found with our tag; the selector succeeds.
			Element* child = parent->GetChild(child_index);
			if (child == element)
				return true;

			// Otherwise, if this child shares our element's tag, then our element is not the first tagged child; the selector fails.
			if (child->GetTagName() == element->GetTagName())
				return false;

			child_index++;
		}

		return false;
	}
	break;
	case StructuralSelectorType::Last_Of_Type:
	{
		Element* parent = element->GetParentNode();
		if (!parent)
			return false;

		int child_index = parent->GetNumChildren() - 1;
		while (child_index >= 0)
		{
			// If this child is our element, then it's the first one we've found with our tag; the selector succeeds.
			Element* child = parent->GetChild(child_index);
			if (child == element)
				return true;

			// Otherwise, if this child shares our element's tag, then our element is not the first tagged child; the selector fails.
			if (child->GetTagName() == element->GetTagName())
				return false;

			child_index--;
		}

		return false;
	}
	break;
	case StructuralSelectorType::Only_Child:
	{
		Element* parent = element->GetParentNode();
		if (!parent)
			return false;

		const int num_children = parent->GetNumChildren();
		for (int i = 0; i < num_children; i++)
		{
			Element* child = parent->GetChild(i);

			// Skip the child if it is our element.
			if (child == element)
				continue;

			// Skip the child if it is trivial.
			if (IsTextElement(child))
				continue;

			return false;
		}

		return true;
	}
	break;
	case StructuralSelectorType::Only_Of_Type:
	{
		Element* parent = element->GetParentNode();
		if (!parent)
			return false;

		const int num_children = parent->GetNumChildren();
		for (int i = 0; i < num_children; i++)
		{
			Element* child = parent->GetChild(i);

			// Skip the child if it is our element.
			if (child == element)
				continue;

			// Skip the child if it does not share our tag.
			if (child->GetTagName() != element->GetTagName())
				continue;

			// We've found a similarly-tagged child to our element; selector fails.
			return false;
		}

		return true;
	}
	break;
	case StructuralSelectorType::Empty:
	{
		return element->GetNumChildren() == 0;
	}
	break;
	case StructuralSelectorType::Not:
	{
		if (!selector.selector_tree)
		{
			RMLUI_ERROR;
			return false;
		}

		bool inner_selector_matches = false;

		for (const StyleSheetNode* node : selector.selector_tree->leafs)
		{
			if (node->IsApplicable(element))
			{
				inner_selector_matches = true;
				break;
			}
		}

		return !inner_selector_matches;
	}
	break;
	case StructuralSelectorType::Invalid:
	{
		RMLUI_ERROR;
	}
	break;
	}

	return false;
}

} // namespace Rml
