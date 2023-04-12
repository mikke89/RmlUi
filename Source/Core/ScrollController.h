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

#ifndef RMLUI_CORE_SCROLLCONTROLLER_H
#define RMLUI_CORE_SCROLLCONTROLLER_H

#include "../../Include/RmlUi/Core/Header.h"
#include "../../Include/RmlUi/Core/ScrollTypes.h"
#include "../../Include/RmlUi/Core/Types.h"

namespace Rml {

/**
    Implements scrolling behavior that occurs over time.

    Scrolling modes are activated externally, targeting a given element. The actual scrolling takes place during update calls.
 */

class ScrollController {
public:
	enum class Mode {
		None,
		Smoothscroll, // Smooth scrolling to target distance.
		Autoscroll,   // Scrolling with middle mouse button.
	};

	void ActivateAutoscroll(Element* target, Vector2i start_position);

	void ActivateSmoothscroll(Element* target, Vector2f delta_distance, ScrollBehavior scroll_behavior);

	bool Update(Vector2i mouse_position, float dp_ratio);

	void IncrementSmoothscrollTarget(Vector2f delta_distance);

	// Resets any active mode and its state.
	void Reset();

	// Sets the scroll behavior for mouse wheel processing and scrollbar interaction.
	void SetDefaultScrollBehavior(ScrollBehavior scroll_behavior, float speed_factor);

	// Returns the autoscroll cursor based on the active scroll velocity.
	String GetAutoscrollCursor(Vector2i mouse_position, float dp_ratio) const;
	// Returns true if autoscroll mode is active and the cursor has been moved outside the idle scroll area.
	bool HasAutoscrollMoved() const;

	Mode GetMode() const { return mode; }
	Element* GetTarget() const { return target; }

private:
	// Updates time to now, and returns the delta time since the previous time update.
	float UpdateTime();

	void UpdateAutoscroll(Vector2i mouse_position, float dp_ratio);

	void UpdateSmoothscroll(float dp_ratio);

	bool HasSmoothscrollReachedTarget() const;

	void PerformScrollOnTarget(Vector2f delta_distance);

	Mode mode = Mode::None;

	Element* target = nullptr;
	double previous_update_time = 0;

	Vector2i autoscroll_start_position;
	Vector2f autoscroll_accumulated_length;
	bool autoscroll_moved = false;

	bool smoothscroll_prefer_instant = false;
	float smoothscroll_speed_factor = 1.f;

	Vector2f smoothscroll_target_distance;
	Vector2f smoothscroll_scrolled_distance;
};

} // namespace Rml
#endif
