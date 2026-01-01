#pragma once

#include "InputType.h"

namespace Rml {

class WidgetSlider;

/**
    A range input type handler.
 */

class InputTypeRange : public InputType {
public:
	InputTypeRange(ElementFormControlInput* element);
	virtual ~InputTypeRange();

	/// Returns a string representation of the current value of the form control.
	/// @return The value of the form control.
	String GetValue() const override;

	/// Called every update from the host element.
	void OnUpdate() override;

	/// Called every time the host element's size changes.
	void OnResize() override;

	/// Checks for necessary functional changes in the control as a result of changed attributes.
	/// @param[in] changed_attributes The list of changed attributes.
	/// @return True if no layout is required, false if the layout needs to be dirtied.
	bool OnAttributeChange(const ElementAttributes& changed_attributes) override;

	/// Checks for necessary functional changes in the control as a result of the event.
	/// @param[in] event The event to process.
	void ProcessDefaultAction(Event& event) override;

	/// Sizes the dimensions to the element's inherent size.
	/// @return True.
	bool GetIntrinsicDimensions(Vector2f& dimensions, float& ratio) override;

private:
	WidgetSlider* widget;
};

} // namespace Rml
