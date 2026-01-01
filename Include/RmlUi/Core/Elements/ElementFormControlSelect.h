#pragma once

#include "../Header.h"
#include "ElementFormControl.h"

namespace Rml {

class WidgetDropDown;

/**
    A drop-down select form control.
 */

class RMLUICORE_API ElementFormControlSelect : public ElementFormControl {
public:
	RMLUI_RTTI_DefineWithParent(ElementFormControlSelect, ElementFormControl)

	/// Constructs a new ElementFormControlSelect. This should not be called directly; use the
	/// Factory instead.
	/// @param[in] tag The tag the element was declared as in RML.
	ElementFormControlSelect(const String& tag);
	virtual ~ElementFormControlSelect();

	/// Returns a string representation of the current value of the form control.
	/// @return The value of the form control.
	String GetValue() const override;
	/// Sets the current value of the form control.
	/// @param[in] value The new value of the form control.
	void SetValue(const String& value) override;

	/// Sets the index of the selection. If the new index lies outside the bounds, it will be clamped.
	/// @param[in] selection The new selection index.
	void SetSelection(int selection);
	/// Returns the index of the currently selected item.
	/// @return The index of the currently selected item.
	int GetSelection() const;

	/// Returns one of the select control's option elements.
	/// @param[in] index The index of the desired option.
	/// @return The option element or nullptr if the index was out of bounds.
	Element* GetOption(int index);
	/// Returns the number of options in the select control.
	/// @return The number of options.
	int GetNumOptions();

	/// Adds a new option to the select control.
	/// @param[in] rml The RML content used to represent the option. This is usually a simple string, but can include RML tags.
	/// @param[in] value The value of the option. This is used to identify the option, but does not necessarily need to be unique.
	/// @param[in] before The index of the element to insert the new option before. If out of bounds of the control's option list (the default), the
	/// new option will be added at the end of the list.
	/// @param[in] selectable If true, this option can be selected. If false, this option is not selectable.
	/// @return The index of the new option.
	int Add(const String& rml, const String& value, int before = -1, bool selectable = true);
	/// Adds a new option to the select control.
	/// @param[in] element The element to add, must be an 'option' element.
	/// @param[in] before The index of the element to insert the new option before.
	/// @return The index of the new option.
	int Add(ElementPtr element, int before = -1);
	/// Removes an option from the select control.
	/// @param[in] index The index of the option to remove. If this is outside of the bounds of the control's option list, no option will be removed.
	void Remove(int index);

	/// Removes all options from the select control.
	void RemoveAll();

	/// Show the selection box.
	void ShowSelectBox();
	/// Hide the selection box.
	void HideSelectBox();
	/// Revert to the value selected when the selection box was opened, then hide the box.
	void CancelSelectBox();
	/// Check whether the select box is opened or not.
	bool IsSelectBoxVisible();

protected:
	/// Moves all children to be under control of the widget.
	void OnUpdate() override;
	/// Updates the layout of the widget's elements.
	void OnRender() override;

	/// Forces an internal layout.
	void OnLayout() override;

	void OnChildAdd(Element* child) override;

	void OnChildRemove(Element* child) override;

	/// Returns true to mark this element as replaced.
	/// @param[out] intrinsic_dimensions Set to the arbitrary dimensions of 128 x 16 just to give this element a size. Resize with the 'width' and
	/// 'height' properties.
	/// @param[out] intrinsic_ratio Ignored.
	/// @return True.
	bool GetIntrinsicDimensions(Vector2f& intrinsic_dimensions, float& intrinsic_ratio) override;

	/// Respond to a change to the value attribute.
	void OnAttributeChange(const ElementAttributes& changed_attributes) override;

	void MoveChildren();

	WidgetDropDown* widget;
};

} // namespace Rml
