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

#ifndef RMLUI_CORE_BOXSHADOWCACHE_H
#define RMLUI_CORE_BOXSHADOWCACHE_H

#include "../../Include/RmlUi/Core/CallbackTexture.h"
#include "../../Include/RmlUi/Core/Geometry.h"
#include "../../Include/RmlUi/Core/RenderBox.h"
#include "../../Include/RmlUi/Core/Types.h"

namespace Rml {
struct BoxShadowGeometryInfo;

struct BoxShadowData : NonCopyMoveable {
	BoxShadowData(CallbackTexture&& texture, Geometry&& geometry, const BoxShadowGeometryInfo& geometry_info);
	~BoxShadowData();

	CallbackTexture texture;
	Geometry geometry;
	const BoxShadowGeometryInfo& cache_key;
};

class BoxShadowCache {
public:
	static void Initialize();
	static void Shutdown();

	/// Returns a handle to BoxShadow data matching the parameters - creates new data if none is found.
	/// @param[in] element Element for which to calculate and cache the box shadow
	/// @return A handle to the BoxShadow data, with automatic reference counting.
	static SharedPtr<BoxShadowData> GetHandle(Element* element, Geometry& background_border_geometry);
};

} // namespace Rml
#endif