#include "InputType.h"
#include "../../../Include/RmlUi/Core/Elements/ElementFormControlInput.h"

namespace Rml {

InputType::InputType(ElementFormControlInput* element) : element(element) {}

InputType::~InputType() {}

String InputType::GetValue() const
{
	return element->GetAttribute<String>("value", "");
}

bool InputType::IsSubmitted()
{
	return true;
}

void InputType::OnUpdate() {}

void InputType::OnRender() {}

void InputType::OnResize() {}

void InputType::OnLayout() {}

bool InputType::OnAttributeChange(const ElementAttributes& /*changed_attributes*/)
{
	return true;
}

void InputType::OnPropertyChange(const PropertyIdSet& /*changed_properties*/) {}

void InputType::OnChildAdd() {}

void InputType::OnChildRemove() {}

void InputType::Select() {}

void InputType::SetSelectionRange(int /*selection_start*/, int /*selection_end*/) {}

void InputType::GetSelection(int* /*selection_start*/, int* /*selection_end*/, String* /*selected_text*/) const {}

void InputType::SetCompositionRange(int /*range_start*/, int /*range_end*/) {}

} // namespace Rml
