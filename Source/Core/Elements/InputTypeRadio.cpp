#include "InputTypeRadio.h"
#include "../../../Include/RmlUi/Core/ElementDocument.h"
#include "../../../Include/RmlUi/Core/ElementUtilities.h"
#include "../../../Include/RmlUi/Core/Elements/ElementForm.h"
#include "../../../Include/RmlUi/Core/Elements/ElementFormControlInput.h"

namespace Rml {

InputTypeRadio::InputTypeRadio(ElementFormControlInput* element) : InputType(element)
{
	if (element->HasAttribute("checked"))
		PopRadioSet();
}

InputTypeRadio::~InputTypeRadio() {}

String InputTypeRadio::GetValue() const
{
	auto value = InputType::GetValue();
	return value.empty() ? "on" : value;
}

bool InputTypeRadio::IsSubmitted()
{
	return element->HasAttribute("checked");
}

bool InputTypeRadio::OnAttributeChange(const ElementAttributes& changed_attributes)
{
	if (changed_attributes.count("checked"))
	{
		const bool checked = element->HasAttribute("checked");
		element->SetPseudoClass("checked", checked);

		if (checked)
			PopRadioSet();

		const auto perceived_value = Variant(checked ? GetValue() : "");
		element->DispatchEvent(EventId::Change,
			{
				{"data-binding-override-value", checked ? perceived_value : Variant()},
				{"value", perceived_value},
				{"checked", Variant(checked)},
			});
	}

	return true;
}

void InputTypeRadio::OnChildAdd()
{
	if (element->HasAttribute("checked"))
		PopRadioSet();
}

void InputTypeRadio::ProcessDefaultAction(Event& event)
{
	if (event == EventId::Click && !element->IsDisabled())
		element->SetAttribute("checked", "");
}

bool InputTypeRadio::GetIntrinsicDimensions(Vector2f& dimensions, float& ratio)
{
	dimensions.x = 16;
	dimensions.y = 16;
	ratio = 1;

	return true;
}

void InputTypeRadio::PopRadioSet()
{
	// Uncheck all other radio buttons with our name in the form.
	String stop_tag;
	Element* parent = element->GetParentNode();
	while (parent != nullptr && rmlui_dynamic_cast<ElementForm*>(parent) == nullptr)
		parent = parent->GetParentNode();

	// If no containing form was found, use the containing document as the parent
	if (parent == nullptr)
	{
		parent = element->GetOwnerDocument();
		stop_tag = "form"; // Don't include any radios that are inside form elements
	}

	if (parent != nullptr)
	{
		ElementList form_controls;
		ElementUtilities::GetElementsByTagName(form_controls, parent, "input", stop_tag);

		for (size_t i = 0; i < form_controls.size(); ++i)
		{
			ElementFormControlInput* radio_control = rmlui_dynamic_cast<ElementFormControlInput*>(form_controls[i]);
			if (radio_control != nullptr && element != radio_control && radio_control->GetAttribute<String>("type", "text") == "radio" &&
				radio_control->GetName() == element->GetName())
			{
				radio_control->RemoveAttribute("checked");
			}
		}
	}
}

} // namespace Rml
