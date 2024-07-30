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

#ifndef RMLUI_CORE_ELEMENTUTILITIES_H
#define RMLUI_CORE_ELEMENTUTILITIES_H

#include "Header.h"
#include "RenderManager.h"
#include "Types.h"

namespace Rml {

class Box;
class Context;
namespace Style {
	class ComputedValues;
}

/**
    Utility functions for dealing with elements.

    @author Lloyd Weehuizen
 */

class RMLUICORE_API ElementUtilities {
public:
	enum PositionAnchor {
		TOP = 1 << 0,
		BOTTOM = 1 << 1,
		LEFT = 1 << 2,
		RIGHT = 1 << 3,

		TOP_LEFT = TOP | LEFT,
		TOP_RIGHT = TOP | RIGHT,
		BOTTOM_LEFT = BOTTOM | LEFT,
		BOTTOM_RIGHT = BOTTOM | RIGHT
	};

	/// Get the element with the given id.
	/// @param[in] root_element First element to check.
	/// @param[in] id ID of the element to look for.
	static Element* GetElementById(Element* root_element, const String& id);
	/// Get all elements with the given tag.
	/// @param[out] elements Resulting elements.
	/// @param[in] root_element First element to check.
	/// @param[in] tag Tag to search for.
	static void GetElementsByTagName(ElementList& elements, Element* root_element, const String& tag);
	/// Get all elements with the given class set on them.
	/// @param[out] elements Resulting elements.
	/// @param[in] root_element First element to check.
	/// @param[in] tag Class name to search for.
	static void GetElementsByClassName(ElementList& elements, Element* root_element, const String& class_name);

	/// Returns an element's density-independent pixel ratio, defined by it's context
	/// @param[in] element The element to determine the density-independent pixel ratio for.
	/// @return The density-independent pixel ratio of the context, or 1.0 if no context assigned.
	static float GetDensityIndependentPixelRatio(Element* element);
	/// Returns the width of a string rendered within the context of the given element.
	/// @param[in] element The element to measure the string from.
	/// @param[in] string The string to measure.
	/// @param[in] prior_character The character placed just before this string, used for kerning.
	/// @return The string width, in pixels.
	static int GetStringWidth(Element* element, StringView string, Character prior_character = Character::Null);

	/// Generates the clipping region for an element.
	/// @param[in] element The element to generate the clipping region for.
	/// @param[out] clip_region The element's clipping region in window coordinates.
	/// @param[out] clip_mask_list Optional, returns a list of geometry that defines the element's clip mask.
	/// @param[in] force_clip_self If true, also clips to the border area of the provided element regardless.
	/// @return True if a clipping region exists for the element and clip_origin and clip_window were set, false if not.
	static bool GetClippingRegion(Element* element, Rectanglei& clip_region, ClipMaskGeometryList* clip_mask_list = nullptr,
		bool force_clip_self = false);
	/// Sets the clipping region from an element and its ancestors.
	/// @param[in] element The element to generate the clipping region from.
	/// @param[in] force_clip_self If true, also clips to the border area of the provided element regardless.
	/// @return The visibility of the given element within its clipping region.
	static bool SetClippingRegion(Element* element, bool force_clip_self = false);

	/// Returns a rectangle covering the element's area in window coordinate space.
	/// @param[in] out_rectangle The resulting rectangle covering the projected element's box.
	/// @param[in] element The element to find the bounding box of.
	/// @param[in] area The box area to consider, 'Auto' means the border box in addition to any ink overflow.
	/// @return True on success, otherwise false.
	static bool GetBoundingBox(Rectanglef& out_rectangle, Element* element, BoxArea area);

	/// Formats the contents of an element. This does not need to be called for ordinary elements, but can be useful
	/// for non-DOM elements of custom elements.
	/// @param[in] element The element to lay out.
	/// @param[in] containing_block The size of the element's containing block.
	static void FormatElement(Element* element, Vector2f containing_block);

	/// Generates the box for an element.
	/// @param[out] box The box to be built.
	/// @param[in] containing_block The dimensions of the content area of the block containing the element.
	/// @param[in] element The element to build the box for.
	/// @param[in] inline_element True if the element is placed in an inline context, false if not.
	static void BuildBox(Box& box, Vector2f containing_block, Element* element, bool inline_element = false);

	/// Sizes an element, and positions it within its parent offset from the borders of its content area. Any relative
	/// values will be evaluated against the size of the element parent's content area.
	/// @param element[in] The element to size and position.
	/// @param offset[in] The offset from the parent's borders.
	/// @param anchor[in] Defines which corner or edge the border is to be positioned relative to.
	static bool PositionElement(Element* element, Vector2f offset, PositionAnchor anchor);

	/// Applies an element's accumulated transform matrix, determined from its and ancestor's `perspective' and `transform' properties.
	/// @param[in] element The element whose transform to apply, or nullptr for identity transform.
	/// @return True if the transform could be submitted to the render interface.
	static bool ApplyTransform(Element& element);

	/// Creates data views and data controllers if a data model applies to the element.
	/// Attributes such as 'data-' are used to create the views and controllers.
	/// @return True if a data view or controller was constructed.
	static bool ApplyDataViewsControllers(Element* element);

	/// Creates data views that use a raw inner xml content string to construct child elements.
	/// Right now, this only applies to the 'data-for' view.
	/// @return True if a data view was constructed.
	static bool ApplyStructuralDataViews(Element* element, const String& inner_rml);
};

} // namespace Rml
#endif
