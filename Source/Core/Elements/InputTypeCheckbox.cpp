#include "InputTypeCheckbox.h"
#include "../../../Include/RmlUi/Core/Elements/ElementFormControlInput.h"

namespace Rml {

InputTypeCheckbox::InputTypeCheckbox(ElementFormControlInput* element) : InputType(element) {}

InputTypeCheckbox::~InputTypeCheckbox() {}

String InputTypeCheckbox::GetValue() const
{
	auto value = InputType::GetValue();
	return value.empty() ? "on" : value;
}

bool InputTypeCheckbox::IsSubmitted()
{
	return element->HasAttribute("checked");
}

bool InputTypeCheckbox::OnAttributeChange(const ElementAttributes& changed_attributes)
{
	if (changed_attributes.count("checked"))
	{
		const bool checked = element->HasAttribute("checked");
		element->SetPseudoClass("checked", checked);
		element->DispatchEvent(EventId::Change,
			{
				{"data-binding-override-value", Variant(checked)},
				{"value", Variant(checked ? GetValue() : "")},
				{"checked", Variant(checked)},
			});
	}

	return true;
}

void InputTypeCheckbox::ProcessDefaultAction(Event& event)
{
	if (event == EventId::Click && !element->IsDisabled())
	{
		if (element->HasAttribute("checked"))
			element->RemoveAttribute("checked");
		else
			element->SetAttribute("checked", "");
	}
}

bool InputTypeCheckbox::GetIntrinsicDimensions(Vector2f& dimensions, float& ratio)
{
	dimensions.x = 16;
	dimensions.y = 16;
	ratio = 1;

	return true;
}

} // namespace Rml
