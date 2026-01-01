#include "InputTypeText.h"
#include "../../../Include/RmlUi/Core/ElementUtilities.h"
#include "../../../Include/RmlUi/Core/Elements/ElementFormControlInput.h"
#include "../../../Include/RmlUi/Core/PropertyIdSet.h"
#include "../../../Include/RmlUi/Core/StyleSheetSpecification.h"
#include "WidgetTextInputSingleLine.h"
#include "WidgetTextInputSingleLinePassword.h"

namespace Rml {

InputTypeText::InputTypeText(ElementFormControlInput* element, Visibility visibility) : InputType(element)
{
	if (visibility == VISIBLE)
		widget = MakeUnique<WidgetTextInputSingleLine>(element);
	else
		widget = MakeUnique<WidgetTextInputSingleLinePassword>(element);
}

InputTypeText::~InputTypeText() {}

void InputTypeText::OnUpdate()
{
	widget->OnUpdate();
}

void InputTypeText::OnRender()
{
	widget->OnRender();
}

void InputTypeText::OnResize()
{
	widget->OnResize();
}

void InputTypeText::OnLayout()
{
	widget->OnLayout();
}

bool InputTypeText::OnAttributeChange(const ElementAttributes& changed_attributes)
{
	bool dirty_layout = false;

	// Check if maxlength has been defined.
	auto it = changed_attributes.find("maxlength");
	if (it != changed_attributes.end())
		widget->SetMaxLength(it->second.Get(-1));

	// Check if size has been defined.
	it = changed_attributes.find("size");
	if (it != changed_attributes.end())
	{
		size = it->second.Get(20);
		dirty_layout = true;
	}

	// Check if the value has been changed.
	it = changed_attributes.find("value");
	if (it != changed_attributes.end())
		widget->SetValue(it->second.Get<String>());

	return !dirty_layout;
}

void InputTypeText::OnPropertyChange(const PropertyIdSet& changed_properties)
{
	// Some inherited properties require text formatting update, mainly font and line-height properties.
	const PropertyIdSet changed_inherited_layout_properties = changed_properties &
		(StyleSheetSpecification::GetRegisteredInheritedProperties() & StyleSheetSpecification::GetRegisteredPropertiesForcingLayout());

	if (!changed_inherited_layout_properties.Empty())
		widget->ForceFormattingOnNextLayout();

	if (changed_properties.Contains(PropertyId::Color) || changed_properties.Contains(PropertyId::BackgroundColor))
		widget->UpdateSelectionColours();

	if (changed_properties.Contains(PropertyId::CaretColor))
		widget->GenerateCursor();
}

void InputTypeText::ProcessDefaultAction(Event& /*event*/) {}

bool InputTypeText::GetIntrinsicDimensions(Vector2f& dimensions, float& /*ratio*/)
{
	dimensions.x = (float)(size * ElementUtilities::GetStringWidth(element, "m"));
	dimensions.y = Math::Round(element->GetLineHeight());

	return true;
}

void InputTypeText::Select()
{
	widget->Select();
}

void InputTypeText::SetSelectionRange(int selection_start, int selection_end)
{
	widget->SetSelectionRange(selection_start, selection_end);
}

void InputTypeText::GetSelection(int* selection_start, int* selection_end, String* selected_text) const
{
	widget->GetSelection(selection_start, selection_end, selected_text);
}

void InputTypeText::SetCompositionRange(int range_start, int range_end)
{
	widget->SetCompositionRange(range_start, range_end);
}

} // namespace Rml
