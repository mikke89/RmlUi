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
			if (Element* label_target = GetTarget())
			{
				Element* event_target = event.GetTargetElement();
				// If the event is already on the way to the label target, there is no reason to intervene with the
				// click. Just let the event move down the chain without interrupting it. Otherwise, intervene to
				// manually redirect the click to the label target.
				if (!label_target->Contains(event_target))
				{
					// Temporarily disable click captures to avoid infinite recursion in case this element is on the path to the target element.
					disable_click = true;
					event.StopPropagation();
					label_target->Focus();
					label_target->Click();
					disable_click = false;
				}
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
