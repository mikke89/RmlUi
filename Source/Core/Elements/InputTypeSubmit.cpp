#include "InputTypeSubmit.h"
#include "../../../Include/RmlUi/Core/Elements/ElementForm.h"
#include "../../../Include/RmlUi/Core/Elements/ElementFormControlInput.h"
#include "../../../Include/RmlUi/Core/Factory.h"

namespace Rml {

InputTypeSubmit::InputTypeSubmit(ElementFormControlInput* element) : InputType(element) {}

InputTypeSubmit::~InputTypeSubmit() {}
bool InputTypeSubmit::IsSubmitted()
{
	// Submit buttons are never submitted; they submit themselves if appropriate.
	return false;
}

bool InputTypeSubmit::OnAttributeChange(const ElementAttributes& changed_attributes)
{
	if (changed_attributes.find("value") != changed_attributes.end())
	{
		auto value = element->GetAttribute<String>("value", "");
		if (!value.empty() && !value_element)
			value_element =
				rmlui_static_cast<ElementText*>(element->AppendChild(Factory::InstanceElement(element, "#text", "", XMLAttributes()), true));

		if (value_element)
			value_element->SetText(value);

		return false;
	}
	return true;
}

void InputTypeSubmit::ProcessDefaultAction(Event& event)
{
	if (event == EventId::Click && !element->IsDisabled())
	{
		Element* parent = element->GetParentNode();
		while (parent)
		{
			ElementForm* form = rmlui_dynamic_cast<ElementForm*>(parent);
			if (form != nullptr)
			{
				form->Submit(element->GetAttribute<String>("name", ""), element->GetAttribute<String>("value", ""));
				return;
			}
			else
			{
				parent = parent->GetParentNode();
			}
		}
	}
}

bool InputTypeSubmit::GetIntrinsicDimensions(Vector2f& /*dimensions*/, float& /*ratio*/)
{
	return false;
}

} // namespace Rml
