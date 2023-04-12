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

#ifndef RMLUI_CORE_ELEMENTS_WIDGETSLIDER_H
#define RMLUI_CORE_ELEMENTS_WIDGETSLIDER_H

#include "../../../Include/RmlUi/Core/EventListener.h"

namespace Rml {

class ElementFormControl;

/**
    A generic widget for incorporating sliding functionality into an element.

    @author Peter Curry
 */

class WidgetSlider final : public EventListener {
public:
	enum Orientation { VERTICAL, HORIZONTAL };

	WidgetSlider(ElementFormControl* parent);
	virtual ~WidgetSlider();

	/// Initialises the slider's hidden elements.
	bool Initialise();

	/// Updates the key repeats for the increment / decrement arrows.
	void Update();

	/// Sets the position of the bar.
	/// @param[in] bar_position The new position of the bar (0 representing the start of the track, 1 representing the end).
	void SetBarPosition(float bar_position);
	/// Returns the current position of the bar.
	/// @return The current position of the bar (0 representing the start of the track, 1 representing the end).
	float GetBarPosition();

	/// Sets the orientation of the slider.
	void SetOrientation(Orientation orientation);
	/// Returns the slider's orientation.
	Orientation GetOrientation() const;

	/// Sets the dimensions to the size of the slider.
	void GetDimensions(Vector2f& dimensions) const;

	/// Sets a new value on the slider, clamped to the min and max values, and rounded to the nearest increment.
	void SetValue(float value);
	/// Returns the current value of the slider.
	float GetValue() const;

	/// Sets the minimum value of the slider.
	void SetMinValue(float min_value);
	/// Sets the maximum value of the slider.
	void SetMaxValue(float max_value);
	/// Sets the slider's value increment.
	void SetStep(float step);

	/// Formats the slider's elements.
	void FormatElements();

private:
	/// Lays out and resizes the slider's internal elements.
	/// @param[in] containing_block The padded box containing the slider. This is used to resolve relative properties.
	/// @param[in] slider_length The total length, in pixels, of the slider widget.
	void FormatElements(Vector2f containing_block, float slider_length);
	/// Lays out and positions the bar element.
	void FormatBar();

	/// Returns the widget's parent element.
	Element* GetParent() const;

	/// Handles events coming through from the slider's components.
	void ProcessEvent(Event& event) override;

	/// Called when the slider's bar position is set or dragged.
	/// @param[in] bar_position The new position of the bar (0 representing the start of the track, 1 representing the end).
	/// @return The new position of the bar.
	float OnBarChange(float bar_position);
	/// Called when the slider is incremented by one 'line', either by the down / right key or a mouse-click on the
	/// increment arrow.
	/// @return The new position of the bar.
	float OnLineIncrement();
	/// Called when the slider is decremented by one 'line', either by the up / left key or a mouse-click on the
	/// decrement arrow.
	/// @return The new position of the bar.
	float OnLineDecrement();

	/// Determine the normalized bar position given an absolute position coordinate.
	/// @param[in] absolute_position Absolute position along the axis determined by 'orientation'.
	/// @return The normalized bar position [0, 1]
	float AbsolutePositionToBarPosition(float absolute_position) const;

	void PositionBar();

	// Clamps the new value, sets it on the slider and returns it as a normalized number from 0 to 1.
	float SetValueInternal(float new_value, bool force_submit_change_event = true);

	ElementFormControl* parent;

	Orientation orientation;

	// The background track element, across which the bar slides.
	Element* track;
	// The bar element. This is the element that is dragged across the trough.
	Element* bar;
	// The two (optional) buttons for incrementing and decrementing the slider.
	Element* arrows[2];

	// A number from 0 to 1, indicating how far along the track the bar is.
	float bar_position;
	// If the bar is being dragged, this is the pixel offset from the start of the bar to where it was picked up.
	float bar_drag_anchor;

	// Set to the auto-repeat timer if either of the arrow buttons have been pressed, -1 if they haven't.
	float arrow_timers[2];
	double last_update_time;

	float value;
	float min_value;
	float max_value;

	float step;
};

} // namespace Rml
#endif
