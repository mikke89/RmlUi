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

#ifndef RMLUI_CORE_WIDGETSCROLL_H
#define RMLUI_CORE_WIDGETSCROLL_H

#include "../../Include/RmlUi/Core/EventListener.h"

namespace Rml {

class Element;
enum class ScrollBehavior;

/**
    A widget for incorporating scrolling functionality into an element.

    @author Peter Curry
 */

class WidgetScroll final : public EventListener {
public:
	enum Orientation { UNKNOWN, VERTICAL, HORIZONTAL };

	WidgetScroll(Element* parent);
	virtual ~WidgetScroll();

	/// Initialises the slider to a given orientation.
	bool Initialise(Orientation orientation);

	/// Updates the key repeats for the increment / decrement arrows.
	void Update();

	/// Sets the position of the bar.
	/// @param[in] bar_position The new position of the bar (0 representing the start of the track, 1 representing the end).
	void SetBarPosition(float bar_position);
	/// Returns the current position of the bar.
	/// @return The current position of the bar (0 representing the start of the track, 1 representing the end).
	float GetBarPosition() const;

	/// Returns the slider's orientation.
	/// @return The orientation of the slider.
	Orientation GetOrientation() const;

	/// Sets the dimensions to the size of the slider.
	/// @param[in] dimensions The dimensions to size.
	void GetDimensions(Vector2f& dimensions) const;

	/// Sets the length of the entire track in scrollable units (usually lines or characters). This affects the
	/// length of the bar element and the speed of scrolling using the mouse-wheel or arrows.
	/// @param[in] track_length The length of the track.
	void SetTrackLength(float track_length);
	/// Sets the length the bar represents in scrollable units (usually lines or characters), relative to the track
	/// length. For example, for a scroll bar, this would represent how big each visible 'page' is compared to the
	/// whole document (which would be set as the track length).
	/// @param[in] bar_length The length of the slider's bar.
	void SetBarLength(float bar_length);

	/// Lays out and resizes the internal elements.
	/// @param[in] containing_block The padded box containing the slider. This is used to resolve relative properties.
	/// @param[in] slider_length The total length, in pixels, of the slider widget.
	void FormatElements(Vector2f containing_block, float slider_length);

private:
	/// Handles events coming through from the slider's components.
	void ProcessEvent(Event& event) override;

	/// Lays out and positions the bar element.
	void FormatBar();

	// Set the offset on 'bar' based on its position.
	void PositionBar();

	void ScrollLineDown();
	void ScrollLineUp();
	void ScrollPageDown();
	void ScrollPageUp();

	// Scrolls the parent element by the given distance.
	void Scroll(float distance, ScrollBehavior behavior);

	Element* parent;

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

	float track_length;
	float bar_length;
};

} // namespace Rml
#endif
