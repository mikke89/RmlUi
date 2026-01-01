#include "InputTypeRange.h"
#include "../../../Include/RmlUi/Core/Elements/ElementFormControlInput.h"
#include "WidgetSlider.h"

namespace Rml {

InputTypeRange::InputTypeRange(ElementFormControlInput* element) : InputType(element)
{
	widget = new WidgetSlider(element);
	widget->Initialise();
}

InputTypeRange::~InputTypeRange()
{
	delete widget;
}

String InputTypeRange::GetValue() const
{
	return CreateString("%f", widget->GetValue());
}

void InputTypeRange::OnUpdate()
{
	widget->Update();
}

void InputTypeRange::OnResize()
{
	widget->FormatElements();
}

bool InputTypeRange::OnAttributeChange(const ElementAttributes& changed_attributes)
{
	bool dirty_layout = false;

	auto it_orientation = changed_attributes.find("orientation");
	if (it_orientation != changed_attributes.end())
	{
		bool is_vertical = (it_orientation->second.Get<String>() == "vertical");
		widget->SetOrientation(is_vertical ? WidgetSlider::VERTICAL : WidgetSlider::HORIZONTAL);
		dirty_layout = true;
	}

	auto it_step = changed_attributes.find("step");
	if (it_step != changed_attributes.end())
		widget->SetStep(it_step->second.Get(1.0f));

	auto it_min = changed_attributes.find("min");
	if (it_min != changed_attributes.end())
		widget->SetMinValue(it_min->second.Get(0.0f));

	auto it_max = changed_attributes.find("max");
	if (it_max != changed_attributes.end())
		widget->SetMaxValue(it_max->second.Get(100.f));

	auto it_value = changed_attributes.find("value");
	if (it_value != changed_attributes.end())
		widget->SetValue(it_value->second.Get(0.0f));

	return !dirty_layout;
}

void InputTypeRange::ProcessDefaultAction(Event& /*event*/) {}

bool InputTypeRange::GetIntrinsicDimensions(Vector2f& dimensions, float& /*ratio*/)
{
	widget->GetDimensions(dimensions);
	return true;
}

} // namespace Rml
