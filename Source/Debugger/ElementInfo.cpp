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

#include "ElementInfo.h"
#include "../../Include/RmlUi/Core/Context.h"
#include "../../Include/RmlUi/Core/Core.h"
#include "../../Include/RmlUi/Core/ElementText.h"
#include "../../Include/RmlUi/Core/ElementUtilities.h"
#include "../../Include/RmlUi/Core/Factory.h"
#include "../../Include/RmlUi/Core/PropertiesIteratorView.h"
#include "../../Include/RmlUi/Core/Property.h"
#include "../../Include/RmlUi/Core/PropertyDefinition.h"
#include "../../Include/RmlUi/Core/RenderManager.h"
#include "../../Include/RmlUi/Core/StyleSheet.h"
#include "../../Include/RmlUi/Core/StyleSheetSpecification.h"
#include "../../Include/RmlUi/Core/SystemInterface.h"
#include "CommonSource.h"
#include "Geometry.h"
#include "InfoSource.h"
#include <algorithm>

namespace Rml {
namespace Debugger {

ElementInfo::ElementInfo(const String& tag) : ElementDebugDocument(tag)
{
	hover_element = nullptr;
	source_element = nullptr;
	enable_element_select = true;
	show_source_element = true;
	update_source_element = true;
	force_update_once = false;
	title_dirty = true;
	previous_update_time = 0.0;
}

ElementInfo::~ElementInfo()
{
	RemoveEventListener(EventId::Click, this);
	RemoveEventListener(EventId::Mouseover, this);
	RemoveEventListener(EventId::Mouseout, this);
}

bool ElementInfo::Initialise()
{
	SetInnerRML(info_rml);
	SetId("rmlui-debug-info");

	AddEventListener(EventId::Click, this);
	AddEventListener(EventId::Mouseover, this);
	AddEventListener(EventId::Mouseout, this);

	SharedPtr<StyleSheetContainer> style_sheet = Factory::InstanceStyleSheetString(String(common_rcss) + String(info_rcss));
	if (!style_sheet)
		return false;

	SetStyleSheetContainer(std::move(style_sheet));

	return true;
}

void ElementInfo::Reset()
{
	hover_element = nullptr;
	show_source_element = true;
	update_source_element = true;
	SetSourceElement(nullptr);
}

void ElementInfo::OnUpdate()
{
	if (source_element && (update_source_element || force_update_once) && IsVisible())
	{
		const double t = GetSystemInterface()->GetElapsedTime();
		const float dt = (float)(t - previous_update_time);

		constexpr float update_interval = 0.3f;

		if (dt > update_interval || (force_update_once))
		{
			if (force_update_once && source_element)
			{
				// Since an update is being forced, it is possibly because we are reacting to an event and made some changes.
				// Update the source element's document to reflect any recent changes.
				if (auto document = source_element->GetOwnerDocument())
					document->UpdateDocument();
			}
			force_update_once = false;

			UpdateSourceElement();
		}
	}

	if (title_dirty)
	{
		UpdateTitle();
		title_dirty = false;
	}
}

void ElementInfo::OnElementDestroy(Element* element)
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
		ElementUtilities::ApplyTransform(*hover_element);
		for (int i = 0; i < hover_element->GetNumBoxes(); i++)
		{
			// Render the content area.
			Vector2f box_offset;
			const Box& element_box = hover_element->GetBox(i, box_offset);
			Vector2f size = element_box.GetSize(BoxArea::Border);
			size = Vector2f(std::max(size.x, 2.0f), std::max(size.y, 2.0f));
			Geometry::RenderOutline(hover_element->GetAbsoluteOffset(BoxArea::Border) + box_offset, size, Colourb(255, 0, 0, 255), 1);
		}
	}
}

void ElementInfo::RenderSourceElement()
{
	if (source_element && show_source_element)
	{
		ElementUtilities::ApplyTransform(*source_element);

		for (int i = 0; i < source_element->GetNumBoxes(); i++)
		{
			Vector2f box_offset;
			const Box& element_box = source_element->GetBox(i, box_offset);
			const Vector2f border_offset = box_offset + source_element->GetAbsoluteOffset(BoxArea::Border);

			// Content area:
			Geometry::RenderBox(border_offset + element_box.GetPosition(BoxArea::Content), element_box.GetSize(), Colourb(158, 214, 237, 128));

			// Padding area:
			Geometry::RenderBox(border_offset + element_box.GetPosition(BoxArea::Padding), element_box.GetSize(BoxArea::Padding),
				border_offset + element_box.GetPosition(BoxArea::Content), element_box.GetSize(), Colourb(135, 122, 214, 128));

			// Border area:
			Geometry::RenderBox(border_offset + element_box.GetPosition(BoxArea::Border), element_box.GetSize(BoxArea::Border),
				border_offset + element_box.GetPosition(BoxArea::Padding), element_box.GetSize(BoxArea::Padding), Colourb(133, 133, 133, 128));

			// Border area:
			Geometry::RenderBox(border_offset + element_box.GetPosition(BoxArea::Margin), element_box.GetSize(BoxArea::Margin),
				border_offset + element_box.GetPosition(BoxArea::Border), element_box.GetSize(BoxArea::Border), Colourb(240, 255, 131, 128));
		}

		if (Context* context = source_element->GetContext())
		{
			context->GetRenderManager().SetTransform(nullptr);

			Rectanglef bounding_box;
			if (ElementUtilities::GetBoundingBox(bounding_box, source_element, BoxArea::Auto))
			{
				bounding_box = bounding_box.Extend(1.f);
				Math::ExpandToPixelGrid(bounding_box);
				Geometry::RenderOutline(bounding_box.Position(), bounding_box.Size(), Colourb(255, 255, 255, 200), 1.f);
			}
		}
	}
}

void ElementInfo::ProcessEvent(Event& event)
{
	// Only process events if we're visible
	if (IsVisible())
	{
		if (event == EventId::Click)
		{
			Element* target_element = event.GetTargetElement();

			// Deal with clicks on our own elements differently.
			if (target_element->GetOwnerDocument() == this)
			{
				const String& id = event.GetTargetElement()->GetId();

				if (id == "close_button")
				{
					if (IsVisible())
						SetProperty(PropertyId::Visibility, Property(Style::Visibility::Hidden));
				}
				else if (id == "update_source")
				{
					update_source_element = !update_source_element;
					target_element->SetClass("active", update_source_element);
				}
				else if (id == "show_source")
				{
					show_source_element = !target_element->IsClassSet("active");
					target_element->SetClass("active", show_source_element);
				}
				else if (id == "enable_element_select")
				{
					enable_element_select = !target_element->IsClassSet("active");
					target_element->SetClass("active", enable_element_select);
				}
				else if (target_element->GetTagName() == "pseudo" && source_element)
				{
					const String name = target_element->GetAttribute<String>("name", "");

					if (!name.empty())
					{
						bool pseudo_active = target_element->IsClassSet("active");
						if (name == "focus")
						{
							if (!pseudo_active)
								source_element->Focus();
							else if (auto document = source_element->GetOwnerDocument())
								document->Focus();
						}
						else
						{
							source_element->SetPseudoClass(name, !pseudo_active);
						}

						force_update_once = true;
					}
				}
				else if (id == "offset_parent")
				{
					if (source_element)
					{
						Element* offset_parent = source_element->GetOffsetParent();
						if (offset_parent)
							SetSourceElement(offset_parent);
					}
				}
				// Check if the id is in the form "a %d" or "c %d" - these are the ancestor or child labels.
				else
				{
					int element_index;
					if (sscanf(target_element->GetId().c_str(), "a %d", &element_index) == 1)
					{
						Element* new_source_element = source_element;
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
				}
				event.StopPropagation();
			}
			// Otherwise we just want to focus on the clicked element (unless it's on a debug element)
			else if (enable_element_select && target_element->GetOwnerDocument() != nullptr && !IsDebuggerElement(target_element))
			{
				Element* new_source_element = target_element;
				if (new_source_element != source_element)
				{
					SetSourceElement(new_source_element);
					event.StopPropagation();
				}
			}
		}
		else if (event == EventId::Mouseover)
		{
			Element* target_element = event.GetTargetElement();
			ElementDocument* owner_document = target_element->GetOwnerDocument();
			if (owner_document == this)
			{
				// Check if the id is in the form "a %d" or "c %d" - these are the ancestor or child labels.
				const String& id = target_element->GetId();
				int element_index;
				if (sscanf(id.c_str(), "a %d", &element_index) == 1)
				{
					hover_element = source_element;
					for (int i = 0; i < element_index; i++)
					{
						if (hover_element != nullptr)
							hover_element = hover_element->GetParentNode();
					}
				}
				else if (sscanf(id.c_str(), "c %d", &element_index) == 1)
				{
					if (source_element != nullptr)
						hover_element = source_element->GetChild(element_index);
				}
				else if (id == "offset_parent")
				{
					if (source_element)
						hover_element = source_element->GetOffsetParent();
				}
				else
				{
					hover_element = nullptr;
				}

				if (id == "show_source" && !show_source_element)
				{
					// Preview the source element view while hovering
					show_source_element = true;
				}

				if (id == "show_source" || id == "update_source" || id == "enable_element_select")
				{
					title_dirty = true;
				}
			}
			// Otherwise we just want to focus on the clicked element (unless it's on a debug element)
			else if (enable_element_select && owner_document && owner_document->GetId().find("rmlui-debug-") != 0)
			{
				if (Context* context = owner_document->GetContext())
					hover_element = context->GetHoverElement();
			}
		}
		else if (event == EventId::Mouseout)
		{
			Element* target_element = event.GetTargetElement();
			ElementDocument* owner_document = target_element->GetOwnerDocument();
			if (owner_document == this)
			{
				const String& id = target_element->GetId();
				if (id == "show_source")
				{
					// Disable the preview of the source element view
					if (show_source_element && !target_element->IsClassSet("active"))
						show_source_element = false;
				}

				if (id == "show_source" || id == "update_source" || id == "enable_element_select")
				{
					title_dirty = true;
				}
			}
			else if (enable_element_select && owner_document && owner_document->GetId().find("rmlui-debug-") != 0)
			{
				// Update hover element also on MouseOut as we may not get a MouseOver event when moving the mouse to a parent element.
				if (Context* context = owner_document->GetContext())
					hover_element = context->GetHoverElement();
			}
		}
	}
}

void ElementInfo::SetSourceElement(Element* new_source_element)
{
	source_element = new_source_element;
	force_update_once = true;
}

void ElementInfo::UpdateSourceElement()
{
	previous_update_time = GetSystemInterface()->GetElapsedTime();
	title_dirty = true;

	// Set the pseudo classes
	if (Element* pseudo = GetElementById("pseudo"))
	{
		StringList list;
		if (source_element)
			list = source_element->GetActivePseudoClasses();

		auto EraseFromList = [](StringList& list, const String& value) {
			auto it = std::find(list.begin(), list.end(), value);
			if (it == list.end())
				return false;
			list.erase(it);
			return true;
		};

		// There are some fixed pseudo classes that we always show and iterate through to determine if they are set.
		// We also want to show other pseudo classes when they are set, they are added under the #extra element last.
		for (int i = 0; i < pseudo->GetNumChildren(); i++)
		{
			Element* child = pseudo->GetChild(i);
			const String name = child->GetAttribute<String>("name", "");

			if (!name.empty())
			{
				bool active = EraseFromList(list, name);
				child->SetClass("active", active);
			}
			else if (child->GetId() == "extra")
			{
				// First, we iterate through the extra elements and remove those that are no longer active.
				for (int j = 0; j < child->GetNumChildren(); j++)
				{
					Element* grandchild = child->GetChild(j);
					const String grandchild_name = grandchild->GetAttribute<String>("name", "");
					bool active = (EraseFromList(list, grandchild_name) == 1);
					if (!active)
						child->RemoveChild(grandchild);
				}
				// Finally, create new pseudo buttons for the rest of the active pseudo classes.
				for (auto& extra_pseudo : list)
				{
					Element* grandchild = child->AppendChild(CreateElement("pseudo"));
					grandchild->SetClass("active", true);
					grandchild->SetAttribute("name", extra_pseudo);
					grandchild->SetInnerRML(":" + extra_pseudo);
				}
			}
		}
	}

	// Set the attributes
	if (Element* attributes_content = GetElementById("attributes-content"))
	{
		String attributes;

		if (source_element != nullptr)
		{
			{
				String name;
				String value;

				// The element's attribute list is not always synchronized with its internal values, fetch
				// them manually here (see e.g. Element::OnAttributeChange for relevant attributes)
				{
					name = "id";
					value = source_element->GetId();
					if (!value.empty())
						attributes += CreateString("%s: <em>%s</em><br />", name.c_str(), value.c_str());
				}
				{
					name = "class";
					value = source_element->GetClassNames();
					if (!value.empty())
						attributes += CreateString("%s: <em>%s</em><br />", name.c_str(), value.c_str());
				}
			}

			for (const auto& pair : source_element->GetAttributes())
			{
				auto& name = pair.first;
				auto& variant = pair.second;
				String value = StringUtilities::EncodeRml(variant.Get<String>());
				if (name != "class" && name != "style" && name != "id")
					attributes += CreateString("%s: <em>%s</em><br />", name.c_str(), value.c_str());
			}

			// Text is not an attribute but useful nonetheless
			if (auto text_element = rmlui_dynamic_cast<ElementText*>(source_element))
			{
				const String& text_content = text_element->GetText();
				attributes += CreateString("Text: <em>%s</em><br />", text_content.c_str());
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

	// Set the properties
	if (Element* properties_content = GetElementById("properties-content"))
	{
		String properties;
		if (source_element != nullptr)
			BuildElementPropertiesRML(properties, source_element, source_element);

		if (properties != properties_rml)
		{
			properties_content->SetInnerRML(properties);
			properties_rml = std::move(properties);
		}
	}

	// Set the events
	if (Element* events_content = GetElementById("events-content"))
	{
		String events;

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

	// Set the position
	if (Element* position_content = GetElementById("position-content"))
	{
		String position;

		if (source_element)
		{
			const Vector2f element_offset = source_element->GetRelativeOffset(BoxArea::Border);
			const auto& box = source_element->GetBox();

			const Vector2f element_size = source_element->GetBox().GetSize(BoxArea::Border);
			Element* offset_parent = source_element->GetOffsetParent();
			const String offset_parent_rml =
				(offset_parent ? StringUtilities::EncodeRml(offset_parent->GetAddress(false, false)) : String("<em>none</em>"));

			auto box_string = [&box](BoxDirection direction) {
				const BoxEdge edge1 = (direction == BoxDirection::Horizontal ? BoxEdge::Left : BoxEdge::Top);
				const BoxEdge edge2 = (direction == BoxDirection::Horizontal ? BoxEdge::Right : BoxEdge::Bottom);
				const float content_size = (direction == BoxDirection::Horizontal ? box.GetSize().x : box.GetSize().y);
				const String edge1_str = ToString(box.GetEdge(BoxArea::Margin, edge1)) + "|" + ToString(box.GetEdge(BoxArea::Border, edge1)) + "|" +
					ToString(box.GetEdge(BoxArea::Padding, edge1));
				const String edge2_str = ToString(box.GetEdge(BoxArea::Padding, edge2)) + "|" + ToString(box.GetEdge(BoxArea::Border, edge2)) + "|" +
					ToString(box.GetEdge(BoxArea::Margin, edge2));
				return CreateString("%s &lt;%s&gt; %s", edge1_str.c_str(), ToString(content_size).c_str(), edge2_str.c_str());
			};

			position = "<span class='name'>left: </span><em>" + ToString(element_offset.x) + "px</em><br/>" +                                 //
				"<span class='name'>top: </span><em>" + ToString(element_offset.y) + "px</em><br/>" +                                         //
				"<span class='name'>width: </span><em>" + ToString(element_size.x) + "px</em><br/>" +                                         //
				"<span class='name'>height: </span><em>" + ToString(element_size.y) + "px</em><br/>" +                                        //
				"<span class='name'>offset parent: </span><p style='display: inline' id='offset_parent'>" + offset_parent_rml + "</p><br/>" + //
				"<span class='name'>box-x (px): </span>" + box_string(BoxDirection::Horizontal) + "<br/>" +                                   //
				"<span class='name'>box-y (px): </span>" + box_string(BoxDirection::Vertical);
		}

		if (position != position_rml)
		{
			position_content->SetInnerRML(position);
			position_rml = std::move(position);
		}
	}

	// Set the ancestors
	if (Element* ancestors_content = GetElementById("ancestors-content"))
	{
		String ancestors;
		Element* element_ancestor = nullptr;
		if (source_element != nullptr)
			element_ancestor = source_element->GetParentNode();

		int ancestor_depth = 1;
		while (element_ancestor)
		{
			String ancestor_name = element_ancestor->GetAddress(false, false);
			ancestors += CreateString("<p id=\"a %d\">%s</p>", ancestor_depth, ancestor_name.c_str());
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

	// Set the children
	if (Element* children_content = GetElementById("children-content"))
	{
		String children;
		if (source_element != nullptr)
		{
			const int num_dom_children = (source_element->GetNumChildren(false));

			for (int i = 0; i < source_element->GetNumChildren(true); i++)
			{
				Element* child = source_element->GetChild(i);

				// If this is a debugger document, do not show it.
				if (IsDebuggerElement(child))
					continue;

				String child_name = child->GetAddress(false, false);
				auto document = rmlui_dynamic_cast<ElementDocument*>(child);
				if (document && !document->GetTitle().empty())
					child_name += " (" + document->GetTitle() + ')';

				const char* non_dom_string = (i >= num_dom_children ? " class=\"non_dom\"" : "");

				children += CreateString("<p id=\"c %d\"%s>%s</p>", i, non_dom_string, child_name.c_str());
			}
		}

		if (children.empty())
		{
			while (children_content->HasChildNodes())
				children_content->RemoveChild(children_content->GetChild(0));
			children_rml.clear();
		}
		else if (children != children_rml)
		{
			children_content->SetInnerRML(children);
			children_rml = std::move(children);
		}
	}
}

void ElementInfo::BuildElementPropertiesRML(String& property_rml, Element* element, Element* primary_element)
{
	NamedPropertyList property_list;

	for (auto it = element->IterateLocalProperties(); !it.AtEnd(); ++it)
	{
		PropertyId property_id = it.GetId();
		const String& property_name = it.GetName();
		const Property* prop = &it.GetProperty();

		// Check that this property isn't overridden or just not inherited.
		if (primary_element->GetProperty(property_id) != prop)
			continue;

		property_list.push_back(NamedProperty{property_name, prop});
	}

	std::sort(property_list.begin(), property_list.end(), [](const NamedProperty& a, const NamedProperty& b) {
		if (a.second->source && !b.second->source)
			return false;
		if (!a.second->source && b.second->source)
			return true;
		if (a.second->specificity < b.second->specificity)
			return false;
		if (a.second->specificity > b.second->specificity)
			return true;
		if (a.second->definition && !b.second->definition)
			return false;
		if (!a.second->definition && b.second->definition)
			return true;
		const String& a_name = StyleSheetSpecification::GetPropertyName(a.second->definition->GetId());
		const String& b_name = StyleSheetSpecification::GetPropertyName(b.second->definition->GetId());
		return a_name < b_name;
	});

	if (!property_list.empty())
	{
		// Print the 'inherited from ...' header if we're not the primary element.
		if (element != primary_element)
		{
			property_rml += "<h3 class='strong'>inherited from " + element->GetAddress(false, false) + "</h3>";
		}

		const PropertySource* previous_source = nullptr;
		bool first_iteration = true;

		for (auto& named_property : property_list)
		{
			auto& source = named_property.second->source;
			if (source.get() != previous_source || first_iteration)
			{
				previous_source = source.get();
				first_iteration = false;

				// Print the rule name header.
				if (source)
				{
					String str_line_number;
					TypeConverter<int, String>::Convert(source->line_number, str_line_number);
					property_rml += "<h3>" + source->rule_name + "</h3>";
					property_rml += "<h4><span class='break-all'>" + source->path + "</span> : " + str_line_number + "</h4>";
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

void ElementInfo::BuildPropertyRML(String& property_rml, const String& name, const Property* property)
{
	const String property_value = property->ToString();

	property_rml += "<span class='name'>" + name + "</span>: " + property_value + "<br/>";
}

void ElementInfo::UpdateTitle()
{
	auto title_content = GetElementById("title-content");
	auto enable_select = GetElementById("enable_element_select");
	auto show_source = GetElementById("show_source");
	auto update_source = GetElementById("update_source");

	if (title_content && enable_select && show_source && update_source)
	{
		if (enable_select->IsPseudoClassSet("hover"))
			title_content->SetInnerRML("<em>(select elements)</em>");
		else if (show_source->IsPseudoClassSet("hover"))
			title_content->SetInnerRML("<em>(draw element dimensions)</em>");
		else if (update_source->IsPseudoClassSet("hover"))
			title_content->SetInnerRML("<em>(update info continuously)</em>");
		else if (source_element)
			title_content->SetInnerRML(source_element->GetTagName());
		else
			title_content->SetInnerRML("Element Information");
	}
}

bool ElementInfo::IsDebuggerElement(Element* element)
{
	return element->GetOwnerDocument()->GetId().find("rmlui-debug-") == 0;
}

} // namespace Debugger
} // namespace Rml
