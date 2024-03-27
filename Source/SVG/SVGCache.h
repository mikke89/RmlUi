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

#ifndef RMLUI_SVG_SVG_CACHE_H
#define RMLUI_SVG_SVG_CACHE_H

#include "../../Include/RmlUi/Core/Box.h"
#include "../../Include/RmlUi/SVG/SVGTypes.h"

namespace Rml {

class Geometry;
class Element;

namespace SVG {

/**
	@author Leah Lindner
 */

class SVGCache
{
public:
	static void Deninitialize();

	/// Returns a handle to some SVG data matching the parameters - creates new data if none is found
	/// @param[in] source Path to a file containing the SVG source data
	/// @param[in] dimensions Size of the computed texture to provide for rendering
	/// @param[in] content_fit Crop the rendered svg to the scale of it's content
	/// @param[in] colour Colour for the computed geometry
	/// @return A valid handle to the SVG data, or 0 if there is a problem with the SVG data
	static SVGHandle GetHandle(const String& source, const Vector2i& dimensions, const bool content_fit, const Colourb colour);

	/// Returns a handle to some SVG data matching the parameters - creates new data if none is found
	/// @param[in] source Path to a file containing the SVG source data
	/// @param[in] element Element for which to calculate the dimensions and colour
	/// @param[in] content_fit Crop the rendered svg to the scale of it's content
	/// @return A valid handle to the SVG data, or 0 if there is a problem with the SVG data
	static SVGHandle GetHandle(const String& source, Element* const element, const bool content_fit, const Box::Area area);

	/// Decreases the ref count for a specific set of the SVG data, and deletes the data if there are no more users
	/// When changing colour or dimensions of an SVG without changing the source file, it's best to get a new handle
	/// first before releasing the old one, to avoid unnecessarily reloading data
	static void ReleaseHandle(const SVGHandle handle);

	/// Return the geometry ready for rendering corresponding to a set of SVG data, or nullptr for invalid handles
	/// Lifetime of the geometry lasts as long as the caller maintains a valid handle
	/// @param[out] intrinsic_dimensions Dimensions of the image specified by the svg source data
	static Geometry* GetGeometry(const SVGHandle handle, Vector2f& intrinsic_dimensions);
};

} // namespace SVG
} // namespace Rml

#endif
