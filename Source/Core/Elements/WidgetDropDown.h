/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019-2023 The RmlUi Team, and contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#ifndef RMLUI_CORE_ELEMENTS_WIDGETDROPDOWN_H
#define RMLUI_CORE_ELEMENTS_WIDGETDROPDOWN_H

#include "../../../Include/RmlUi/Core/EventListener.h"

namespace Rml {

class ElementFormControl;

/**
    Widget for drop-down functionality.
    @author Lloyd Weehuizen
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
	/// @param[in] The index of the desired option.
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

private:
	// Shows or hides the selection box.
	void ShowSelectBox(bool show);

	void AttachScrollEvent();
	void DetachScrollEvent();

	// Parent element that holds this widget
	ElementFormControl* parent_element;

	// The elements making up the drop-down process.
	Element* button_element;
	Element* selection_element;
	Element* value_element;

	bool lock_selection;
	bool selection_dirty;
	bool value_rml_dirty;
	bool value_layout_dirty;
	bool box_layout_dirty;
	bool box_visible;
};

} // namespace Rml
#endif
