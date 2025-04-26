/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019- The RmlUi Team, and contributors
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

#include "../../Include/RmlUi/Core/Texture.h"
#include "../../Include/RmlUi/Core/Types.h"

namespace Rml {

class Element;
class Geometry;

namespace SVG {

	struct SVGKey;

	struct SVGData : NonCopyMoveable {
		SVGData(Geometry& geometry, Texture texture, Vector2f intrinsic_dimensions, const SVGKey& cache_key);
		~SVGData();

		Geometry& geometry;
		Texture texture;
		Vector2f intrinsic_dimensions;
		const SVGKey& cache_key;
	};

	class SVGCache {
	public:
		static void Initialize();
		static void Shutdown();

		/// Returns a handle to SVG data matching the parameters - creates new data if none is found.
		/// @param[in] source Path to a file containing the SVG source data.
		/// @param[in] element Element for which to calculate the dimensions and color.
		/// @param[in] crop_to_content Crop the rendered SVG to its contents.
		/// @param[in] area The area of the element used to determine the SVG dimensions.
		/// @return A handle to the SVG data, with automatic reference counting.
		///	@note When changing color or dimensions of an SVG without changing the source file, it's best to get a
		/// new handle before releasing the old one, to avoid unnecessarily reloading data.
		static SharedPtr<SVGData> GetHandle(const String& source, Element* element, bool crop_to_content, BoxArea area);
	};

} // namespace SVG
} // namespace Rml

#endif
