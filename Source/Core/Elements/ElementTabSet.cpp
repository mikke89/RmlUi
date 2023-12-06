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

#include "../../../Include/RmlUi/Core/Elements/ElementTabSet.h"
#include "../../../Include/RmlUi/Core/Factory.h"
#include "../../../Include/RmlUi/Core/Math.h"

namespace Rml {

ElementTabSet::ElementTabSet(const String& tag) : Element(tag)
{
	active_tab = 0;
}

ElementTabSet::~ElementTabSet() {}

void ElementTabSet::SetTab(int tab_index, const String& rml)
{
	ElementPtr element = Factory::InstanceElement(nullptr, "*", "tab", XMLAttributes());
	Factory::InstanceElementText(element.get(), rml);
	SetTab(tab_index, std::move(element));
}

void ElementTabSet::SetPanel(int tab_index, const String& rml)
{
	ElementPtr element = Factory::InstanceElement(nullptr, "*", "panel", XMLAttributes());
	Factory::InstanceElementText(element.get(), rml);
	SetPanel(tab_index, std::move(element));
}

void ElementTabSet::SetTab(int tab_index, ElementPtr element)
{
	Element* tabs = GetChildByTag("tabs");
	if (tab_index >= 0 && tab_index < tabs->GetNumChildren())
		tabs->ReplaceChild(std::move(element), tabs->GetChild(tab_index));
	else
		tabs->AppendChild(std::move(element));
}

void ElementTabSet::SetPanel(int tab_index, ElementPtr element)
{
	// append the window
	Element* windows = GetChildByTag("panels");
	if (tab_index >= 0 && tab_index < windows->GetNumChildren())
		windows->ReplaceChild(std::move(element), windows->GetChild(tab_index));
	else
		windows->AppendChild(std::move(element));
}

void ElementTabSet::RemoveTab(int tab_index)
{
	if (tab_index < 0)
		return;

	Element* panels = GetChildByTag("panels");
	if (panels->GetNumChildren() > tab_index)
	{
		panels->RemoveChild(panels->GetChild(tab_index));
	}

	Element* tabs = GetChildByTag("tabs");
	if (tabs->GetNumChildren() > tab_index)
	{
		tabs->RemoveChild(tabs->GetChild(tab_index));
	}
}

int ElementTabSet::GetNumTabs()
{
	return GetChildByTag("tabs")->GetNumChildren();
}

void ElementTabSet::SetActiveTab(int tab_index)
{
	// Update display if the tab has changed
	if (tab_index != active_tab)
	{
		Element* tabs = GetChildByTag("tabs");
		Element* old_tab = tabs->GetChild(active_tab);
		Element* new_tab = tabs->GetChild(tab_index);

		if (old_tab)
			old_tab->SetPseudoClass("selected", false);
		if (new_tab)
			new_tab->SetPseudoClass("selected", true);

		Element* windows = GetChildByTag("panels");
		Element* old_window = windows->GetChild(active_tab);
		Element* new_window = windows->GetChild(tab_index);

		if (old_window)
			old_window->SetProperty(PropertyId::Display, Property(Style::Display::None));
		if (new_window)
			new_window->RemoveProperty(PropertyId::Display);

		active_tab = tab_index;

		Dictionary parameters;
		parameters["tab_index"] = active_tab;
		DispatchEvent(EventId::Tabchange, parameters);
	}
}

int ElementTabSet::GetActiveTab() const
{
	return active_tab;
}

void ElementTabSet::ProcessDefaultAction(Event& event)
{
	Element::ProcessDefaultAction(event);

	if (event == EventId::Click)
	{
		// Find the tab that this click occured on
		Element* tabs = GetChildByTag("tabs");
		Element* tab = event.GetTargetElement();
		while (tab && tab != this && tab->GetParentNode() != tabs)
			tab = tab->GetParentNode();

		// Abort if we couldn't find the tab the click occured on
		if (!tab || tab == this)
			return;

		// Determine the new active tab index
		int new_active_tab = active_tab;
		for (int i = 0; i < tabs->GetNumChildren(); i++)
		{
			if (tabs->GetChild(i) == tab)
			{
				new_active_tab = i;
				break;
			}
		}

		SetActiveTab(new_active_tab);
	}
}

void ElementTabSet::OnChildAdd(Element* child)
{
	Element::OnChildAdd(child);

	if (child->GetParentNode() == GetChildByTag("tabs"))
	{
		// Set up the new button and append it
		child->RemoveProperty(PropertyId::Display);

		if (child->GetParentNode()->GetChild(active_tab) == child)
			child->SetPseudoClass("selected", true);
	}

	if (child->GetParentNode() == GetChildByTag("panels"))
	{
		// Hide the new tab window
		child->SetProperty(PropertyId::Display, Property(Style::Display::None));

		// Make the new element visible if its the active tab
		if (child->GetParentNode()->GetChild(active_tab) == child)
			child->RemoveProperty(PropertyId::Display);
	}
}

Element* ElementTabSet::GetChildByTag(const String& tag)
{
	// Look for the existing child
	for (int i = 0; i < GetNumChildren(); i++)
	{
		if (GetChild(i)->GetTagName() == tag)
			return GetChild(i);
	}

	// If it doesn't exist, create it
	ElementPtr element = Factory::InstanceElement(this, "*", tag, XMLAttributes());
	Element* result = AppendChild(std::move(element));
	return result;
}

} // namespace Rml
