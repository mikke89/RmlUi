#pragma once

#include "../../../Include/RmlUi/Core/EventListener.h"

namespace Rml {

class ElementFormControl;

/**
    Widget for drop-down functionality.
 */

class WidgetDropDown : public EventListener {
public:
	WidgetDropDown(ElementFormControl* element);
	virtual ~WidgetDropDown();

	/// Updates the select value rml if necessary.
	void OnUpdate();
	/// Updates the selection box layout if necessary.
	void OnRender();
	/// Positions the drop-down's internal elements.
	void OnLayout();

	/// Sets the value of the widget.
	void OnValueChange(const String& value);

	/// Sets the option element as the new selection.
	/// @param[in] option_element The option element to select.
	/// @param[in] force Forces the new selection, even if the widget believes the selection to not have changed.
	void SetSelection(Element* option_element, bool force = false);
	/// Seek to the next or previous valid (visible and not disabled) option.
	/// @param[in] seek_forward True to select the next valid option, false to select the previous valid option.
	void SeekSelection(bool seek_forward = true);
	/// Returns the index of the currently selected item.
	int GetSelection() const;

	/// Adds a new option to the select control.
	/// @param[in] rml The RML content used to represent the option.
	/// @param[in] value The value of the option.
	/// @param[in] before The index of the element to insert the new option before.
	/// @param[in] select True to select the new option.
	/// @param[in] selectable If true this option can be selected. If false, this option is not selectable.
	/// @return The index of the new option.
	int AddOption(const String& rml, const String& value, int before, bool select, bool selectable = true);
	/// Moves an option element to the select control.
	/// @param[in] element Element to move.
	/// @param[in] before The index of the element to insert the new option before.
	/// @return The index of the new option, or -1 if invalid.
	int AddOption(ElementPtr element, int before);
	/// Removes an option from the select control.
	/// @param[in] index The index of the option to remove.
	void RemoveOption(int index);
	/// Removes all options from the list.
	void ClearOptions();

	/// Returns one of the widget's options.
	/// @param[in] index The index of the desired option.
	/// @return The option element or nullptr if the index was out of bounds.
	Element* GetOption(int index);
	/// Returns the number of options in the widget.
	/// @return The number of options.
	int GetNumOptions() const;

	// Handle newly added option elements.
	void OnChildAdd(Element* element);
	// Handle newly removed option elements.
	void OnChildRemove(Element* element);

	/// Processes the incoming event.
	void ProcessEvent(Event& event) override;

	/// Shows the selection box.
	void ShowSelectBox();
	/// Hides the selection box.
	void HideSelectBox();
	/// Revert to the value selected when the selection box was opened, then hide the box.
	void CancelSelectBox();
	/// Check whether the select box is visible or not.
	bool IsSelectBoxVisible();

private:
	void AttachScrollEvent();
	void DetachScrollEvent();

	// Parent element that holds this widget
	ElementFormControl* parent_element;

	// The elements making up the drop-down process.
	Element* button_element;
	Element* selection_element;
	Element* value_element;

	String selected_value_on_box_open;

	bool lock_selection = false;
	bool selection_dirty = false;
	bool value_rml_dirty = false;
	bool value_layout_dirty = false;
	bool value_changed_since_last_box_format = false;
	bool box_layout_dirty = false;
	bool box_opened_since_last_format = false;
	bool box_visible = false;
};

} // namespace Rml
