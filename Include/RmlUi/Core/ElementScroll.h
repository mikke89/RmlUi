#pragma once

#include "Header.h"
#include "Types.h"

namespace Rml {

class Element;
class WidgetScroll;

/**
    Manages an element's scrollbars and scrolling state.
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

	/// Updates the scrollbar elements to reflect their current state.
	void UpdateProperties();

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
