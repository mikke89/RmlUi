#pragma once

#include "../../../Include/RmlUi/Core/Event.h"
#include "../../../Include/RmlUi/Core/Types.h"

namespace Rml {

class ElementFormControlInput;

/**
    An interface for a input type handler used by ElementFormControlInput. A concrete InputType object handles the
    functionality of an input element.
 */

class InputType {
public:
	InputType(ElementFormControlInput* element);
	virtual ~InputType();

	/// Returns a string representation of the current value of the form control.
	/// @return The value of the form control.
	virtual String GetValue() const;
	/// Returns if this value should be submitted with the form.
	/// @return True if the form control is to be submitted, false otherwise.
	virtual bool IsSubmitted();

	/// Called every update from the host element.
	virtual void OnUpdate();

	/// Called every render from the host element.
	virtual void OnRender();

	/// Called every time the host element's size changes.
	virtual void OnResize();

	/// Called every time the host element is layed out.
	virtual void OnLayout();

	/// Checks for necessary functional changes in the control as a result of changed attributes.
	/// @param[in] changed_attributes The list of changed attributes.
	/// @return True if no layout is required, false if the layout needs to be dirtied.
	virtual bool OnAttributeChange(const ElementAttributes& changed_attributes);
	/// Called when properties on the control are changed.
	/// @param[in] changed_properties The properties changed on the element.
	virtual void OnPropertyChange(const PropertyIdSet& changed_properties);

	/// Called when the element is added into a hierarchy.
	virtual void OnChildAdd();
	/// Called when the element is removed from a hierarchy.
	virtual void OnChildRemove();

	/// Checks for necessary functional changes in the control as a result of the event.
	/// @param[in] event The event to process.
	virtual void ProcessDefaultAction(Event& event) = 0;

	/// Sizes the dimensions to the element's inherent size.
	virtual bool GetIntrinsicDimensions(Vector2f& dimensions, float& ratio) = 0;

	/// Selects all text.
	virtual void Select();
	/// Selects the text in the given character range.
	virtual void SetSelectionRange(int selection_start, int selection_end);
	/// Retrieves the selection range and text.
	virtual void GetSelection(int* selection_start, int* selection_end, String* selected_text) const;

	/// Sets visual feedback for the IME composition in the given character range.
	virtual void SetCompositionRange(int range_start, int range_end);

protected:
	ElementFormControlInput* element;
};

} // namespace Rml
