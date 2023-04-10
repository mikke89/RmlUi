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

#include "ElementLabel.h"

namespace Rml {

ElementLabel::ElementLabel(const String& tag) : Element(tag)
{
	AddEventListener(EventId::Click, this, true);
}

ElementLabel::~ElementLabel()
{
	RemoveEventListener(EventId::Click, this, true);
}

void ElementLabel::OnPseudoClassChange(const String& pseudo_class, bool activate)
{
	if (pseudo_class == "active" || pseudo_class == "hover")
	{
		if (Element* target = GetTarget())
		{
			OverridePseudoClass(target, pseudo_class, activate);
		}
	}
}

void ElementLabel::ProcessEvent(Event& event)
{
	// Forward clicks to the target.
	if (event == EventId::Click && !disable_click)
	{
		if (event.GetPhase() == EventPhase::Capture || event.GetPhase() == EventPhase::Target)
		{
			if (Element* target = GetTarget())
			{
				// Temporarily disable click captures to avoid infinite recursion in case this element is on the path to the target element.
				disable_click = true;
				event.StopPropagation();
				target->Focus();
				target->Click();
				disable_click = false;
			}
		}
	}
}

// Get the first descending element whose tag name matches one of tags.
static Element* TagMatchRecursive(const StringList& tags, Element* element)
{
	const int num_children = element->GetNumChildren();

	for (int i = 0; i < num_children; i++)
	{
		Element* child = element->GetChild(i);

		for (const String& tag : tags)
		{
			if (child->GetTagName() == tag)
				return child;
		}

		Element* matching_element = TagMatchRecursive(tags, child);
		if (matching_element)
			return matching_element;
	}

	return nullptr;
}

Element* ElementLabel::GetTarget()
{
	const String target_id = GetAttribute<String>("for", "");

	if (target_id.empty())
	{
		const StringList matching_tags = {"button", "input", "textarea", "progress", "progressbar", "select"};

		return TagMatchRecursive(matching_tags, this);
	}
	else
	{
		Element* target = GetElementById(target_id);
		if (target != this)
			return target;
	}

	return nullptr;
}

} // namespace Rml
