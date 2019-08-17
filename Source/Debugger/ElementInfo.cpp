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

#include "ElementInfo.h"
#include "../../Include/RmlUi/Core.h"
#include "../../Include/RmlUi/Core/Property.h"
#include "../../Include/RmlUi/Core/PropertiesIteratorView.h"
#include "../../Include/RmlUi/Core/Factory.h"
#include "../../Include/RmlUi/Core/StyleSheet.h"
#include "../../Include/RmlUi/Core/StyleSheetSpecification.h"
#include "Geometry.h"
#include "CommonSource.h"
#include "InfoSource.h"

namespace Rml {
namespace Debugger {

static void PrettyFormatNumbers(Core::String& string)
{
	// Removes trailing zeros and truncates decimal digits to the specified number of significant digits.
	constexpr int num_significant_digits = 4;

	if (string.empty())
		return;

	// First, check for a decimal point. No point, no chance of trailing zeroes!
	size_t decimal_point_position = 0;

	while ((decimal_point_position = string.find('.', decimal_point_position + 1)) != Core::String::npos)
	{
		// Find the left-most digit.
		int pos_left = (int)decimal_point_position - 1; // non-inclusive
		while (pos_left >= 0 && string[pos_left] >= '0' && string[pos_left] <= '9')
			pos_left--;

		// Significant digits left of the decimal point. We also consider all zero digits significant on the left side.
		const int significant_left = (int)decimal_point_position - (pos_left + 1);

		// Let's not touch numbers that don't start with a digit before the decimal.
		if (significant_left == 0)
			continue;

		const int max_significant_right = std::max(num_significant_digits - significant_left, 0);

		// Find the right-most digit and number of non-zero digits less than our maximum.
		int pos_right = (int)decimal_point_position + 1; // non-inclusive
		int significant_right = 0;
		while (pos_right < (int)string.size() && string[pos_right] >= '0' && string[pos_right] <= '9')
		{
			const int current_digit_right = pos_right - (int)decimal_point_position;
			if (string[pos_right] != '0' && current_digit_right <= max_significant_right)
				significant_right = current_digit_right;
			pos_right++;
		}

		size_t pos_cut_start = decimal_point_position + (size_t)(significant_right + 1);
		size_t pos_cut_end = (size_t)pos_right;

		// Remove the decimal point if we don't have any right digits.
		if (pos_cut_start == decimal_point_position + 1)
			pos_cut_start = decimal_point_position;

		string.erase(string.begin() + pos_cut_start, string.begin() + pos_cut_end);
	}

	return;
}

static bool TestPrettyFormat(Core::String string, Core::String should_be)
{
	Core::String original = string;
	PrettyFormatNumbers(string);
	bool result = (string == should_be);
	if (!result)
		Core::Log::Message(Core::Log::LT_ERROR, "Remove trailing string failed. PrettyFormatNumbers('%s') == '%s' != '%s'", original.c_str(), string.c_str(), should_be.c_str());
	return result;
}

ElementInfo::ElementInfo(const Core::String& tag) : Core::ElementDocument(tag)
{
	hover_element = nullptr;
	source_element = nullptr;
	previous_update_time = 0.0;

	RMLUI_ASSERT(TestPrettyFormat("0.15", "0.15"));
	RMLUI_ASSERT(TestPrettyFormat("0.150", "0.15"));
	RMLUI_ASSERT(TestPrettyFormat("1.15", "1.15"));
	RMLUI_ASSERT(TestPrettyFormat("1.150", "1.15"));
	RMLUI_ASSERT(TestPrettyFormat("123.15", "123.1"));
	RMLUI_ASSERT(TestPrettyFormat("1234.5", "1234"));
	RMLUI_ASSERT(TestPrettyFormat("12.15", "12.15"));
	RMLUI_ASSERT(TestPrettyFormat("12.154", "12.15"));
	RMLUI_ASSERT(TestPrettyFormat("12.154666", "12.15"));
	RMLUI_ASSERT(TestPrettyFormat("15889", "15889"));
	RMLUI_ASSERT(TestPrettyFormat("15889.1", "15889"));
	RMLUI_ASSERT(TestPrettyFormat("0.00660", "0.006"));
	RMLUI_ASSERT(TestPrettyFormat("0.000001", "0"));
	RMLUI_ASSERT(TestPrettyFormat("0.00000100", "0"));
	RMLUI_ASSERT(TestPrettyFormat("a .", "a ."));
	RMLUI_ASSERT(TestPrettyFormat("a .0", "a .0"));
	RMLUI_ASSERT(TestPrettyFormat("a 0.0", "a 0"));
	RMLUI_ASSERT(TestPrettyFormat("hello.world: 14.5600 1.1 0.55623 more.values: 0.1544 0.", "hello.world: 14.56 1.1 0.556 more.values: 0.154 0"));
}

ElementInfo::~ElementInfo()
{
}

// Initialises the info element.
bool ElementInfo::Initialise()
{
	SetInnerRML(info_rml);
	SetId("rmlui-debug-info");

	AddEventListener(Core::EventId::Click, this);
	AddEventListener(Core::EventId::Mouseover, this);

	Core::SharedPtr<Core::StyleSheet> style_sheet = Core::Factory::InstanceStyleSheetString(Core::String(common_rcss) + Core::String(info_rcss));
	if (!style_sheet)
		return false;

	SetStyleSheet(std::move(style_sheet));

	return true;
}

// Clears the element references.
void ElementInfo::Reset()
{
	hover_element = nullptr;
	SetSourceElement(nullptr);
}

void ElementInfo::OnUpdate()
{
	if (source_element && IsVisible())
	{
		const double t = Core::GetSystemInterface()->GetElapsedTime();
		const float dt = (float)(t - previous_update_time);

		constexpr float update_interval = 0.3f;

		if (dt > update_interval)
		{
			UpdateSourceElement();
		}
	}
}

// Called when an element is destroyed.
void ElementInfo::OnElementDestroy(Core::Element* element)
{
	if (hover_element == element)
		hover_element = nullptr;

	if (source_element == element)
		source_element = nullptr;
}

void ElementInfo::RenderHoverElement()
{
	if (hover_element)
	{
		Core::ElementUtilities::ApplyTransform(*hover_element);
		for (int i = 0; i < hover_element->GetNumBoxes(); i++)
		{
			// Render the content area.
			const Core::Box element_box = hover_element->GetBox(i);
			Geometry::RenderOutline(
				hover_element->GetAbsoluteOffset(Core::Box::BORDER) + element_box.GetPosition(Core::Box::BORDER), 
				element_box.GetSize(Core::Box::BORDER), 
				Core::Colourb(255, 0, 0, 255), 
				1
			);
		}
		Core::ElementUtilities::UnapplyTransform(*hover_element);
	}
}

void ElementInfo::RenderSourceElement()
{
	if (source_element)
	{
		Core::ElementUtilities::ApplyTransform(*source_element);

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

		Core::ElementUtilities::UnapplyTransform(*source_element);
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
							if (new_source_element != nullptr)
								new_source_element = new_source_element->GetParentNode();
						}
						SetSourceElement(new_source_element);
					}
					else if (sscanf(target_element->GetId().c_str(), "c %d", &element_index) == 1)
					{
						if (source_element != nullptr)
							SetSourceElement(source_element->GetChild(element_index));
					}
					event.StopPropagation();
				}
			}
			// Otherwise we just want to focus on the clicked element (unless it's on a debug element)
			else if (target_element->GetOwnerDocument() != nullptr && !IsDebuggerElement(target_element))
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
						if (hover_element != nullptr)
							hover_element = hover_element->GetParentNode();
					}
				}
				else if (sscanf(target_element->GetId().c_str(), "c %d", &element_index) == 1)
				{
					if (source_element != nullptr)
						hover_element = source_element->GetChild(element_index);
				}
			}
			// Otherwise we just want to focus on the clicked element (unless it's on a debug element)
			else if (owner_document != nullptr && owner_document->GetId().find("rmlui-debug-") != 0)
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
	previous_update_time = Core::GetSystemInterface()->GetElapsedTime();

	// Set the title:
	Core::Element* title_content = GetElementById("title-content");
	if (title_content != nullptr)
	{
		if (source_element != nullptr)
			title_content->SetInnerRML(source_element->GetTagName());
		else
			title_content->SetInnerRML("Element Information");
	}


	// Set the attributes:
	Core::Element* attributes_content = GetElementById("attributes-content");
	if (attributes_content)
	{
		Core::String attributes;

		if (source_element != nullptr)
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
			attributes_rml.clear();
		}
		else if (attributes != attributes_rml)
		{
			attributes_content->SetInnerRML(attributes);
			attributes_rml = std::move(attributes);
		}
	}

	// Set the properties:
	Core::Element* properties_content = GetElementById("properties-content");
	if (properties_content)
	{
		Core::String properties;
		if (source_element != nullptr)
			BuildElementPropertiesRML(properties, source_element, source_element);

		if (properties.empty())
		{
			while (properties_content->HasChildNodes())
				properties_content->RemoveChild(properties_content->GetChild(0));
			properties_rml.clear();
		}
		else if (properties != properties_rml)
		{
			properties_content->SetInnerRML(properties);
			properties_rml = std::move(properties);
		}
	}

	// Set the events:
	Core::Element* events_content = GetElementById("events-content");
	if (events_content)
	{
		Core::String events;

		if (source_element != nullptr)
		{
			events = source_element->GetEventDispatcherSummary();
		}

		if (events.empty())
		{
			while (events_content->HasChildNodes())
				events_content->RemoveChild(events_content->GetChild(0));
			events_rml.clear();
		}
		else if (events != events_rml)
		{
			events_content->SetInnerRML(events);
			events_rml = std::move(events);
		}
	}

	// Set the position:
	Core::Element* position_content = GetElementById("position-content");
	if (position_content)
	{
		// left, top, width, height.
		if (source_element != nullptr)
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
		Core::Element* element_ancestor = nullptr;
		if (source_element != nullptr)
			element_ancestor = source_element->GetParentNode();

		int ancestor_depth = 1;
		while (element_ancestor)
		{
			Core::String ancestor_name = element_ancestor->GetAddress(false, false);
			ancestors += Core::CreateString(ancestor_name.size() + 32, "<p id=\"a %d\">%s</p>", ancestor_depth, ancestor_name.c_str());
			element_ancestor = element_ancestor->GetParentNode();
			ancestor_depth++;
		}

		if (ancestors.empty())
		{
			while (ancestors_content->HasChildNodes())
				ancestors_content->RemoveChild(ancestors_content->GetFirstChild());
			ancestors_rml.clear();
		}
		else if (ancestors != ancestors_rml)
		{
			ancestors_content->SetInnerRML(ancestors);
			ancestors_rml = std::move(ancestors);
		}
	}

	// Set the children:
	Core::Element* children_content = GetElementById("children-content");
	if (children_content)
	{
		Core::String children;
		if (source_element != nullptr)
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
			children_rml.clear();
		}
		else if(children != children_rml)
		{
			children_content->SetInnerRML(children);
			children_rml = std::move(children);
		}
	}
}

void ElementInfo::BuildElementPropertiesRML(Core::String& property_rml, Core::Element* element, Core::Element* primary_element)
{
	NamedPropertyList property_list;

	for(auto it = element->IterateLocalProperties(); !it.AtEnd(); ++it)
	{
		Core::PropertyId property_id = it.GetId();
		const Core::String& property_name = it.GetName();
		const Core::Property* prop = &it.GetProperty();

		// Check that this property isn't overridden or just not inherited.
		if (primary_element->GetLocalProperty(property_id) != prop)
			continue;

		property_list.push_back(NamedProperty{ property_name, prop });
	}

	std::sort(property_list.begin(), property_list.end(),
		[](const NamedProperty& a, const NamedProperty& b) {
			if (a.second->source && !b.second->source) return false;
			if (!a.second->source && b.second->source) return true;
			return a.second->specificity > b.second->specificity; 
		}
	);

	if (!property_list.empty())
	{
		// Print the 'inherited from ...' header if we're not the primary element.
		if (element != primary_element)
		{
			property_rml += "<h3 class='mark'>inherited from " + element->GetAddress(false, false) + "</h3>";
		}

		const Core::PropertySource* previous_source = nullptr;
		bool first_iteration = true;

		for (auto& named_property : property_list)
		{
			auto& source = named_property.second->source;
			if(source.get() != previous_source || first_iteration)
			{
				previous_source = source.get();
				first_iteration = false;

				// Print the rule name header.
				if(source)
				{
					Core::String str_line_number;
					Core::TypeConverter<int, Core::String>::Convert(source->line_number, str_line_number);
					property_rml += "<h3>" + source->rule_name + "</h3>";
					property_rml += "<h4>" + source->path + " : " + str_line_number + "</h4>";
				}
				else
				{
					property_rml += "<h3><em>inline</em></h3>";
				}
			}

			BuildPropertyRML(property_rml, named_property.first, named_property.second);
		}
	}

	if (element->GetParentNode() != nullptr)
		BuildElementPropertiesRML(property_rml, element->GetParentNode(), primary_element);
}

void ElementInfo::BuildPropertyRML(Core::String& property_rml, const Core::String& name, const Core::Property* property)
{
	Core::String property_value = property->ToString();
	PrettyFormatNumbers(property_value);

	property_rml += Core::CreateString(name.size() + property_value.size() + 32, "%s: <em>%s;</em><br />", name.c_str(), property_value.c_str());
}


bool ElementInfo::IsDebuggerElement(Core::Element* element)
{
	return element->GetOwnerDocument()->GetId().find("rmlui-debug-") == 0;
}

}
}
