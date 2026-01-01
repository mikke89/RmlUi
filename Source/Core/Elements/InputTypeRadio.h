#pragma once

#include "InputType.h"

namespace Rml {

/**
    A radio button input type handler.
 */

class InputTypeRadio : public InputType {
public:
	InputTypeRadio(ElementFormControlInput* element);
	virtual ~InputTypeRadio();

	/// Returns a string representation of the current value of the form control.
	/// @return The value of the form control.
	String GetValue() const override;

	/// Returns if this value should be submitted with the form.
	/// @return True if the form control is to be submitted, false otherwise.
	bool IsSubmitted() override;

	/// Checks for necessary functional changes in the control as a result of changed attributes.
	/// @param[in] changed_attributes The list of changed attributes.
	/// @return True if no layout is required, false if the layout needs to be dirtied.
	bool OnAttributeChange(const ElementAttributes& changed_attributes) override;

	/// Pops the element's radio set if we are checked.
	void OnChildAdd() override;

	/// Checks for necessary functional changes in the control as a result of the event.
	/// @param[in] event The event to process.
	void ProcessDefaultAction(Event& event) override;

	/// Sizes the dimensions to the element's inherent size.
	/// @return True.
	bool GetIntrinsicDimensions(Vector2f& dimensions, float& ratio) override;

private:
	/// Pops all other radio buttons in our form that share our name.
	void PopRadioSet();
};

} // namespace Rml
