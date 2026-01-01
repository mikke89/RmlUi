#include "../../../Include/RmlUi/Core/Elements/ElementFormControl.h"
#include "../../../Include/RmlUi/Core/ComputedValues.h"

namespace Rml {

ElementFormControl::ElementFormControl(const String& tag) : Element(tag)
{
	SetProperty(PropertyId::TabIndex, Property(Style::TabIndex::Auto));
}

ElementFormControl::~ElementFormControl() {}

String ElementFormControl::GetName() const
{
	return GetAttribute<String>("name", "");
}

void ElementFormControl::SetName(const String& name)
{
	SetAttribute("name", name);
}

bool ElementFormControl::IsSubmitted()
{
	return true;
}

bool ElementFormControl::IsDisabled() const
{
	return HasAttribute("disabled");
}

void ElementFormControl::SetDisabled(bool disable)
{
	if (disable)
		SetAttribute("disabled", "");
	else
		RemoveAttribute("disabled");
}

void ElementFormControl::OnAttributeChange(const ElementAttributes& changed_attributes)
{
	Element::OnAttributeChange(changed_attributes);

	if (changed_attributes.find("disabled") != changed_attributes.end())
	{
		bool is_disabled = IsDisabled();
		SetPseudoClass("disabled", is_disabled);

		// Disable focus when element is disabled. This will also prevent click
		// events (when originating from user inputs, see Context) to reach the element.
		if (is_disabled)
		{
			SetProperty(PropertyId::Focus, Property(Style::Focus::None));
			Blur();
		}
		else
			RemoveProperty(PropertyId::Focus);
	}
}

} // namespace Rml
