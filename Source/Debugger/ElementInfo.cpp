/*
 * This source file is part of libRocket, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://www.librocket.com
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
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

#include "ElementInfo.h"
#include "../../Include/Rocket/Core/Property.h"
#include "../../Include/Rocket/Core/PropertiesIteratorView.h"
#include "../../Include/Rocket/Core/Factory.h"
#include "../../Include/Rocket/Core/StyleSheet.h"
#include "../../Include/Rocket/Core/StyleSheetSpecification.h"
#include "Geometry.h"
#include "CommonSource.h"
#include "InfoSource.h"
#include <map>

namespace Rocket {
namespace Debugger {

ElementInfo::ElementInfo(const Core::String& tag) : Core::ElementDocument(tag)
{
	hover_element = NULL;
	source_element = NULL;
}

ElementInfo::~ElementInfo()
{
}

// Initialises the info element.
bool ElementInfo::Initialise()
{
	SetInnerRML(info_rml);
	SetId("rkt-debug-info");

	Core::StyleSheet* style_sheet = Core::Factory::InstanceStyleSheetString(Core::String(common_rcss) + Core::String(info_rcss));
	if (style_sheet == NULL)
		return false;

	SetStyleSheet(style_sheet);
	style_sheet->RemoveReference();

	return true;
}

// Clears the element references.
void ElementInfo::Reset()
{
	hover_element = NULL;
	SetSourceElement(NULL);
}

// Called when an element is destroyed.
void ElementInfo::OnElementDestroy(Core::Element* element)
{
	if (hover_element == element)
		hover_element = NULL;

	if (source_element == element)
		source_element = NULL;
}

void ElementInfo::RenderHoverElement()
{
	if (hover_element)
	{
		for (int i = 0; i < hover_element->GetNumBoxes(); i++)
		{
			// Render the content area.
			const Core::Box element_box = hover_element->GetBox(i);
			Geometry::RenderOutline(hover_element->GetAbsoluteOffset(Core::Box::BORDER) + element_box.GetPosition(Core::Box::BORDER), element_box.GetSize(Core::Box::BORDER), Core::Colourb(255, 0, 0, 255), 1);
		}
	}
}

void ElementInfo::RenderSourceElement()
{
	if (source_element)
	{
		for (int i = 0; i < source_element->GetNumBoxes(); i++)
		{
			const Core::Box element_box = source_element->GetBox(i);

			// Content area:
			Geometry::RenderBox(source_element->GetAbsoluteOffset(Core::Box::BORDER) + element_box.GetPosition(Core::Box::CONTENT), element_box.GetSize(), Core::Colourb(158, 214, 237, 128));

			// Padding area:
			Geometry::RenderBox(source_element->GetAbsoluteOffset(Core::Box::BORDER) + element_box.GetPosition(Core::Box::PADDING), element_box.GetSize(Core::Box::PADDING), source_element->GetAbsoluteOffset(Core::Box::BORDER) + element_box.GetPosition(Core::Box::CONTENT), element_box.GetSize(), Core::Colourb(135, 122, 214, 128));

			// Border area:
			Geometry::RenderBox(source_element->GetAbsoluteOffset(Core::Box::BORDER) + element_box.GetPosition(Core::Box::BORDER), element_box.GetSize(Core::Box::BORDER), source_element->GetAbsoluteOffset(Core::Box::BORDER) + element_box.GetPosition(Core::Box::PADDING), element_box.GetSize(Core::Box::PADDING), Core::Colourb(133, 133, 133, 128));

			// Border area:
			Geometry::RenderBox(source_element->GetAbsoluteOffset(Core::Box::BORDER) + element_box.GetPosition(Core::Box::MARGIN), element_box.GetSize(Core::Box::MARGIN), source_element->GetAbsoluteOffset(Core::Box::BORDER) + element_box.GetPosition(Core::Box::BORDER), element_box.GetSize(Core::Box::BORDER), Core::Colourb(240, 255, 131, 128));
		}
	}
}

void ElementInfo::ProcessEvent(Core::Event& event)
{
	// Only process events if we're visible
	if (IsVisible())
	{
		if (event == Core::EventId::Click)
		{
			Core::Element* target_element = event.GetTargetElement();

			// Deal with clicks on our own elements differently.
			if (target_element->GetOwnerDocument() == this)
			{
				// If it's a pane title, then we need to toggle the visibility of its sibling (the contents pane underneath it).
				if (target_element->GetTagName() == "h2")
				{
					Core::Element* panel = target_element->GetNextSibling();
					if (panel->IsVisible())
						panel->SetProperty(Core::PropertyId::Display, Core::Property(Core::Style::Display::None));
					else
						panel->SetProperty(Core::PropertyId::Display, Core::Property(Core::Style::Display::Block));
					event.StopPropagation();
				}
				else if (event.GetTargetElement()->GetId() == "close_button")
				{
					if (IsVisible())
						SetProperty(Core::PropertyId::Visibility, Core::Property(Core::Style::Visibility::Hidden));
				}
				// Check if the id is in the form "a %d" or "c %d" - these are the ancestor or child labels.
				else
				{
					int element_index;
					if (sscanf(target_element->GetId().c_str(), "a %d", &element_index) == 1)
					{
						Core::Element* new_source_element = source_element;
						for (int i = 0; i < element_index; i++)
						{
							if (new_source_element != NULL)
								new_source_element = new_source_element->GetParentNode();
						}
						SetSourceElement(new_source_element);
					}
					else if (sscanf(target_element->GetId().c_str(), "c %d", &element_index) == 1)
					{
						if (source_element != NULL)
							SetSourceElement(source_element->GetChild(element_index));
					}
					event.StopPropagation();
				}
			}
			// Otherwise we just want to focus on the clicked element (unless it's on a debug element)
			else if (target_element->GetOwnerDocument() != NULL && !IsDebuggerElement(target_element))
			{
				Core::Element* new_source_element = target_element;
				if (new_source_element != source_element)
				{
					SetSourceElement(new_source_element);
					event.StopPropagation();
				}
			}
		}
		else if (event == Core::EventId::Mouseover)
		{
			Core::Element* target_element = event.GetTargetElement();

			// Deal with clicks on our own elements differently.
			Core::ElementDocument* owner_document = target_element->GetOwnerDocument();
			if (owner_document == this)
			{
				// Check if the id is in the form "a %d" or "c %d" - these are the ancestor or child labels.
				int element_index;
				if (sscanf(target_element->GetId().c_str(), "a %d", &element_index) == 1)
				{
					hover_element = source_element;
					for (int i = 0; i < element_index; i++)
					{
						if (hover_element != NULL)
							hover_element = hover_element->GetParentNode();
					}
				}
				else if (sscanf(target_element->GetId().c_str(), "c %d", &element_index) == 1)
				{
					if (source_element != NULL)
						hover_element = source_element->GetChild(element_index);
				}
			}
			// Otherwise we just want to focus on the clicked element (unless it's on a debug element)
			else if (owner_document != NULL && owner_document->GetId().find("rkt-debug-") != 0)
			{
				hover_element = target_element;
			}
		}
	}
}

void ElementInfo::SetSourceElement(Core::Element* new_source_element)
{
	source_element = new_source_element;
	UpdateSourceElement();
}

void ElementInfo::UpdateSourceElement()
{
	// Set the title:
	Core::Element* title_content = GetElementById("title-content");
	if (title_content != NULL)
	{
		if (source_element != NULL)
			title_content->SetInnerRML(source_element->GetTagName());
		else
			title_content->SetInnerRML("Element Information");
	}


	// Set the attributes:
	Core::Element* attributes_content = GetElementById("attributes-content");
	if (attributes_content)
	{
		Core::String attributes;

		if (source_element != NULL)
		{
			{
				Core::String name;
				Core::String value;

				// The element's attribute list is not always synchronized with its internal values, fetch  
				// them manually here (see e.g. Element::OnAttributeChange for relevant attributes)
				{
					name = "id";
					value = source_element->GetId();
					if (!value.empty())
						attributes += Core::CreateString(name.size() + value.size() + 32, "%s: <em>%s</em><br />", name.c_str(), value.c_str());
				}
				{
					name = "class";
					value = source_element->GetClassNames();
					if (!value.empty())
						attributes += Core::CreateString(name.size() + value.size() + 32, "%s: <em>%s</em><br />", name.c_str(), value.c_str());
				}
				{
					// Not actually an attribute, but may be useful
					name = "pseudo";
					value.clear();
					for (auto str : source_element->GetActivePseudoClasses())
						value += " :" + str;
					if (!value.empty())
						attributes += Core::CreateString(name.size() + value.size() + 32, "%s: <em>%s</em><br />", name.c_str(), value.c_str());
				}
				{
					name = "style";
					value = "";
					auto local_properties = source_element->GetLocalProperties();
					if (local_properties)
					{
						for (auto nvp : *local_properties)
						{
							auto& prop_name = Core::StyleSheetSpecification::GetPropertyName(nvp.first);
							auto prop_value = nvp.second.ToString();
							value += Core::CreateString(prop_name.size() + prop_value.size() + 12, "%s: %s; ", prop_name.c_str(), prop_value.c_str());
						}
					}
					if (!value.empty())
						attributes += Core::CreateString(name.size() + value.size() + 32, "%s: <em>%s</em><br />", name.c_str(), value.c_str());
				}
			}

			for(const auto& pair : source_element->GetAttributes())
			{
				auto& name = pair.first;
				auto& variant = pair.second;
				Core::String value = variant.Get<Core::String>();
				if(name != "class" && name != "style" && name != "id") 
					attributes += Core::CreateString(name.size() + value.size() + 32, "%s: <em>%s</em><br />", name.c_str(), value.c_str());
			}
		}

		if (attributes.empty())
		{
			while (attributes_content->HasChildNodes())
				attributes_content->RemoveChild(attributes_content->GetChild(0));
		}
		else
			attributes_content->SetInnerRML(attributes);
	}

	// Set the properties:
	Core::Element* properties_content = GetElementById("properties-content");
	if (properties_content)
	{
		Core::String properties;
		if (source_element != NULL)
			BuildElementPropertiesRML(properties, source_element, source_element);

		if (properties.empty())
		{
			while (properties_content->HasChildNodes())
				properties_content->RemoveChild(properties_content->GetChild(0));
		}
		else
			properties_content->SetInnerRML(properties);
	}

	// Set the events:
	Core::Element* events_content = GetElementById("events-content");
	if (events_content)
	{
		Core::String events;

		if (source_element != NULL)
		{
			events = source_element->GetEventDispatcherSummary();
		}

		if (events.empty())
		{
			while (events_content->HasChildNodes())
				events_content->RemoveChild(events_content->GetChild(0));
		}
		else
			events_content->SetInnerRML(events);
	}

	// Set the position:
	Core::Element* position_content = GetElementById("position-content");
	if (position_content)
	{
		// left, top, width, height.
		if (source_element != NULL)
		{
			Core::Vector2f element_offset = source_element->GetRelativeOffset(Core::Box::BORDER);
			Core::Vector2f element_size = source_element->GetBox().GetSize(Core::Box::BORDER);

			Core::String positions;
			positions += Core::CreateString(64, "left: <em>%.0fpx</em><br />", element_offset.x);
			positions += Core::CreateString(64, "top: <em>%.0fpx</em><br />", element_offset.y);
			positions += Core::CreateString(64, "width: <em>%.0fpx</em><br />", element_size.x);
			positions += Core::CreateString(64, "height: <em>%.0fpx</em><br />", element_size.y);

			position_content->SetInnerRML(positions);
		}
		else
		{
			while (position_content->HasChildNodes())
				position_content->RemoveChild(position_content->GetFirstChild());
		}
	}

	// Set the ancestors:
	Core::Element* ancestors_content = GetElementById("ancestors-content");
	if (ancestors_content)
	{
		Core::String ancestors;
		Core::Element* element_ancestor = NULL;
		if (source_element != NULL)
			element_ancestor = source_element->GetParentNode();

		int ancestor_depth = 1;
		while (element_ancestor)
		{
			Core::String ancestor_name = element_ancestor->GetTagName();
			const Core::String ancestor_id = element_ancestor->GetId();
			if (!ancestor_id.empty())
			{
				ancestor_name += "#";
				ancestor_name += ancestor_id;
			}

			ancestors += Core::CreateString(ancestor_name.size() + 32, "<p id=\"a %d\">%s</p>", ancestor_depth, ancestor_name.c_str());
			element_ancestor = element_ancestor->GetParentNode();
			ancestor_depth++;
		}

		if (ancestors.empty())
		{
			while (ancestors_content->HasChildNodes())
				ancestors_content->RemoveChild(ancestors_content->GetFirstChild());
		}
		else
			ancestors_content->SetInnerRML(ancestors);
	}

	// Set the children:
	Core::Element* children_content = GetElementById("children-content");
	if (children_content)
	{
		Core::String children;
		if (source_element != NULL)
		{
			for (int i = 0; i < source_element->GetNumChildren(); i++)
			{
				Core::Element* child = source_element->GetChild(i);

				// If this is a debugger document, do not show it.
				if (IsDebuggerElement(child))
					continue;

				Core::String child_name = child->GetTagName();
				const Core::String child_id = child->GetId();
				if (!child_id.empty())
				{
					child_name += "#";
					child_name += child_id;
				}

				children += Core::CreateString(child_name.size() + 32, "<p id=\"c %d\">%s</p>", i, child_name.c_str());
			}
		}

		if (children.empty())
		{
			while (children_content->HasChildNodes())
				children_content->RemoveChild(children_content->GetChild(0));
		}
		else
			children_content->SetInnerRML(children);
	}
}

void ElementInfo::BuildElementPropertiesRML(Core::String& property_rml, Core::Element* element, Core::Element* primary_element)
{
	NamedPropertyMap property_map;

	for(auto it = element->IterateLocalProperties(); !it.AtEnd(); ++it)
	{
		const Core::PropertyId property_id = it.GetId();
		const Core::String& property_name = it.GetName();
		const Core::Property* property = &it.GetProperty();
		const Core::PseudoClassList& property_pseudo_classes = it.GetPseudoClassList();

		// Check that this property isn't overridden or just not inherited.
		if (primary_element->GetProperty(property_id) != property)
			continue;

		NamedPropertyMap::iterator i = property_map.find(property_pseudo_classes);
		if (i == property_map.end())
			property_map[property_pseudo_classes] = NamedPropertyList(1, NamedProperty(property_name, property));
		else
		{
			// Find a place in this list of properties to insert the new one.
			NamedPropertyList& properties = (*i).second;
			NamedPropertyList::iterator insert_iterator = properties.begin();
			while (insert_iterator != properties.end())
			{
				int source_cmp = strcasecmp((*insert_iterator).second->source.c_str(), property->source.c_str());
				if (source_cmp > 0 ||
					(source_cmp == 0 && (*insert_iterator).second->source_line_number >= property->source_line_number))
					break;

				++insert_iterator;
			}

			(*i).second.insert(insert_iterator, NamedProperty(property_name, property));
		}
	}

	if (!property_map.empty())
	{
		// Print the 'inherited from ...' header if we're not the primary element.
		if (element != primary_element)
			property_rml += Core::CreateString(element->GetTagName().size() + 32, "<h3>inherited from %s</h3>", element->GetTagName().c_str());

		NamedPropertyMap::iterator base_properties = property_map.find(Core::PseudoClassList());
		if (base_properties != property_map.end())
			BuildPropertiesRML(property_rml, (*base_properties).second);

		for (NamedPropertyMap::iterator i = property_map.begin(); i != property_map.end(); ++i)
		{
			// Skip the base property list, we've already printed it.
			if (i == base_properties)
				continue;

			// Print the pseudo-class header.
			property_rml += "<h3>";

			for (Core::PseudoClassList::const_iterator j = (*i).first.begin(); j != (*i).first.end(); ++j)
			{
				property_rml += " :";
				property_rml += *j;
			}

			property_rml += "</h3>";

			BuildPropertiesRML(property_rml, (*i).second);
		}
	}

	if (element->GetParentNode() != NULL)
		BuildElementPropertiesRML(property_rml, element->GetParentNode(), primary_element);
}

void ElementInfo::BuildPropertiesRML(Core::String& property_rml, const NamedPropertyList& properties)
{
	Core::String last_source;
	int last_source_line = -1;

	for (size_t i = 0; i < properties.size(); ++i)
	{
		if (i == 0 ||
			last_source != properties[i].second->source ||
			last_source_line != properties[i].second->source_line_number)
		{
			last_source = properties[i].second->source;
			last_source_line = properties[i].second->source_line_number;

			property_rml += "<h4>";

			if (last_source.empty() &&
				last_source_line == 0)
				property_rml += "<em>inline</em>";
			else
				property_rml += Core::CreateString(last_source.size() + 32, "<em>%s</em>: %d", last_source.c_str(), last_source_line);

			property_rml += "</h4>";
		}

		BuildPropertyRML(property_rml, properties[i].first, properties[i].second);
	}
}

void ElementInfo::BuildPropertyRML(Core::String& property_rml, const Core::String& name, const Core::Property* property)
{
	Core::String property_value = property->ToString();
	RemoveTrailingZeroes(property_value);

	property_rml += Core::CreateString(name.size() + property_value.size() + 32, "%s: <em>%s;</em><br />", name.c_str(), property_value.c_str());
}

void ElementInfo::RemoveTrailingZeroes(Core::String& string)
{
	if (string.empty())
	{
		return;
	}

	// First, check for a decimal point. No point, no chance of trailing zeroes!
	size_t decimal_point_position = string.find(".");
	if (decimal_point_position != Core::String::npos)
	{
		// Ok, so now we start at the back of the string and find the first
		// numeral. If the character we find is a zero, then we start counting
		// back till we find something that isn't a zero or a decimal point -
		// and then remove all that we've counted.
		size_t last_zero = string.size() - 1;
		while ((string[last_zero] < '0' || string[last_zero] > '9') && string[last_zero] != '.')
		{
			if (last_zero == 0)
			{
				return;
			}
			if (string[last_zero] == '.')
			{
				break;
			}
			last_zero--;
		}

		if (!(string[last_zero] == '0' || string[last_zero] == '.'))
		{
			return;
		}

		// Now find the first character that isn't a zero (unless we're just
		// chopping off the dangling decimal point)
		size_t first_zero = last_zero;
		if (string[last_zero] == '0')
		{
			while (first_zero > 0 && string[first_zero - 1] == '0')
			{
				first_zero--;
			}

			// Check for a preceeding decimal point - if it's all zeroes until the
			// decimal, then we should remove it too.
			if (string[first_zero - 1] == '.')
			{
				first_zero--;
			}
		}

		// Now remove everything between first_zero and last_zero, inclusive.
		if (last_zero > first_zero)
			string.erase(first_zero, (last_zero - first_zero) + 1);
	}
}

bool ElementInfo::IsDebuggerElement(Core::Element* element)
{
	return element->GetOwnerDocument()->GetId().find("rkt-debug-") == 0;
}

}
}
