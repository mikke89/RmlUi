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

#ifndef RMLUI_CORE_ELEMENTSCROLL_H
#define RMLUI_CORE_ELEMENTSCROLL_H

#include "Header.h"
#include "Types.h"

namespace Rml {

class Element;
class WidgetScroll;

/**
    Manages an element's scrollbars and scrolling state.

    @author Peter Curry
 */

class RMLUICORE_API ElementScroll {
public:
	enum Orientation { VERTICAL = 0, HORIZONTAL = 1 };

	ElementScroll(Element* element);
	~ElementScroll();

	/// Updates the increment / decrement arrows.
	void Update();

	/// Enables and sizes one of the scrollbars.
	/// @param[in] orientation Which scrollbar (vertical or horizontal) to enable.
	/// @param[in] element_width The current computed width of the element, used only to resolve percentage properties.
	void EnableScrollbar(Orientation orientation, float element_width);
	/// Disables and hides one of the scrollbars.
	/// @param[in] orientation Which scrollbar (vertical or horizontal) to disable.
	void DisableScrollbar(Orientation orientation);

	/// Updates the position of the scrollbar.
	/// @param[in] orientation Which scrollbar (vertical or horizontal) to update).
	void UpdateScrollbar(Orientation orientation);

	/// Returns one of the scrollbar elements.
	/// @param[in] orientation Which scrollbar to return.
	/// @return The requested scrollbar, or nullptr if it does not exist.
	Element* GetScrollbar(Orientation orientation);
	/// Returns the size, in pixels, of one of the scrollbars; for a vertical scrollbar, this is width, for a horizontal scrollbar, this is height.
	/// @param[in] orientation Which scrollbar (vertical or horizontal) to query.
	/// @return The size of the scrollbar, or 0 if the scrollbar is disabled.
	float GetScrollbarSize(Orientation orientation);

	/// Formats the enabled scrollbars based on the current size of the host element.
	void FormatScrollbars();

private:
	struct Scrollbar {
		Scrollbar();
		~Scrollbar();

		Element* element = nullptr;
		UniquePtr<WidgetScroll> widget;
		bool enabled = false;
		float size = 0;
	};

	// Creates one of the scroll component's scrollbar.
	bool CreateScrollbar(Orientation orientation);
	// Creates the scrollbar corner.
	bool CreateCorner();
	// Update properties of scroll elements immediately after construction.
	void UpdateScrollElementProperties(Element* scroll_element);

	Element* element;

	Scrollbar scrollbars[2];
	Element* corner;
};

} // namespace Rml
#endif
