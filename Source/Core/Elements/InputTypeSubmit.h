#pragma once

#include "../../../Include/RmlUi/Core/ElementText.h"
#include "InputType.h"

namespace Rml {

/**
    A submit input type handler.
 */

class InputTypeSubmit : public InputType {
public:
	InputTypeSubmit(ElementFormControlInput* element);
	virtual ~InputTypeSubmit();

	/// Returns if this value should be submitted with the form.
	/// @return True if the form control is to be submitted, false otherwise.
	bool IsSubmitted() override;

	/// Called when an attribute of the element has changed.
	/// @param[in] changed_attributes The attributes that have changed.
	/// @return True if no layout is required, false if the layout needs to be dirtied.
	bool OnAttributeChange(const ElementAttributes& changed_attributes) override;

	/// Checks for necessary functional changes in the control as a result of the event.
	/// @param[in] event The event to process.
	void ProcessDefaultAction(Event& event) override;

	/// Sizes the dimensions to the element's inherent size.
	/// @return False.
	bool GetIntrinsicDimensions(Vector2f& dimensions, float& ratio) override;

private:
	ElementText* value_element = nullptr;
};

} // namespace Rml
