/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019 The RmlUi Team, and contributors
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
#include "../../Include/RmlUi/Core/Types.h"

namespace Rml {

class ScrollController {
public:
	enum class Mode { None, Smoothscroll, Autoscroll };

	void ActivateAutoscroll(Element* in_target, Vector2i start_position);

	void Update(Vector2i mouse_position, float dp_ratio)
	{
		if (mode == Mode::Autoscroll && target)
			UpdateAutoscroll(mouse_position, dp_ratio);
		else if (mode == Mode::Smoothscroll && target)
			UpdateSmoothscroll(mouse_position, dp_ratio);
	}

	bool ProcessMouseWheel(Vector2f wheel_delta, Element* hover, float dp_ratio);

	void ProcessMouseButtonUp()
	{
		if (mode == Mode::Autoscroll && autoscroll_holding)
			Reset();
	}

	void OnElementDetach(Element* element)
	{
		if (element == target)
			Reset();
	}

	// Reset autoscroll state, disabling the mode.
	void Reset() { *this = ScrollController{}; }

	// Returns the autoscroll cursor based on the scroll direction.
	String GetAutoscrollCursor(Vector2i mouse_position, float dp_ratio) const;

	bool operator==(Mode in_mode) const { return mode == in_mode; }

	explicit operator bool() const { return mode != Mode::None; }

private:
	void ActivateSmoothscroll(Element* in_target)
	{
		Reset();
		mode = Mode::Smoothscroll;
		target = in_target;
		UpdateTime();
	}

	// Updates time to now, and returns the delta time since the previous time update.
	float UpdateTime();

	// Update autoscroll state (scrolling with middle mouse button), and submit scroll events as necessary.
	void UpdateAutoscroll(Vector2i mouse_position, float dp_ratio);

	void UpdateSmoothscroll(Vector2i mouse_position, float dp_ratio);

	Mode mode = Mode::None;

	Element* target = nullptr;
	double previous_update_time = 0;

	Vector2i autoscroll_start_position;
	Vector2f autoscroll_accumulated_length;
	bool autoscroll_holding = false;

	Vector2f smoothscroll_target_distance;
	Vector2f smoothscroll_scrolled_distance;
};

} // namespace Rml
#endif
