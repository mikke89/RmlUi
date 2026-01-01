#pragma once

#include "InputType.h"

namespace Rml {

class WidgetTextInput;

/**
    A single-line input type handler.
 */

class InputTypeText : public InputType {
public:
	enum Visibility { VISIBLE, OBSCURED };

	InputTypeText(ElementFormControlInput* element, Visibility visibility = VISIBLE);
	virtual ~InputTypeText();

	/// Called every update from the host element.
	void OnUpdate() override;

	/// Called every render from the host element.
	void OnRender() override;

	/// Called when the parent element's size changes.
	void OnResize() override;

	/// Called when the parent element is layed out.
	void OnLayout() override;

	/// Checks for necessary functional changes in the control as a result of changed attributes.
	/// @param[in] changed_attributes The list of changed attributes.
	/// @return True if no layout is required, false if the layout needs to be dirtied.
	bool OnAttributeChange(const ElementAttributes& changed_attributes) override;
	/// Called when properties on the control are changed.
	/// @param[in] changed_properties The properties changed on the element.
	void OnPropertyChange(const PropertyIdSet& changed_properties) override;

	/// Checks for necessary functional changes in the control as a result of the event.
	/// @param[in] event The event to process.
	void ProcessDefaultAction(Event& event) override;

	/// Sizes the dimensions to the element's inherent size.
	/// @return True.
	bool GetIntrinsicDimensions(Vector2f& dimensions, float& ratio) override;

	/// Selects all text.
	void Select() override;
	/// Selects the text in the given character range.
	void SetSelectionRange(int selection_start, int selection_end) override;
	/// Retrieves the selection range and text.
	void GetSelection(int* selection_start, int* selection_end, String* selected_text) const override;

	/// Sets visual feedback for the IME composition in the given character range.
	void SetCompositionRange(int range_start, int range_end) override;

private:
	int size = 20;

	UniquePtr<WidgetTextInput> widget;
};

} // namespace Rml
