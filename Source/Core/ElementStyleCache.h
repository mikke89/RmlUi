/*
 * This source file is part of libRocket, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://www.librocket.com
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
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

#ifndef ROCKETCOREELEMENTSTYLECACHE_H
#define ROCKETCOREELEMENTSTYLECACHE_H

#include "ElementDefinition.h"
#include "../../Include/Rocket/Core/Types.h"

namespace Rocket {
namespace Core {

class ElementStyle;

/**
	Manages caching of layout-important properties and provides
	O(1) access to them (note that for invalidated cache, the access
	time is still O(log(N)) as per standard std::map).
	@author Victor Luchits
 */

class ElementStyleCache
{
public:
	ElementStyleCache(ElementStyle *style);

	/// Invalidation function for all non-inherited properties
	void Clear();

	/// Invalidation function for all inherited properties
	void ClearInherited();

	/// Invalidation functions for individual and grouped non-inherited properties
	void ClearBorder();
	void ClearMargin();
	void ClearPadding();
	void ClearDimensions();
	void ClearPosition();
	void ClearFloat();
	void ClearDisplay();
	void ClearWhitespace();
	void ClearOverflow();

	/// Invalidation functions for individual and grouped inherited properties
	void ClearLineHeight();
	void ClearTextAlign();
	void ClearTextTransform();
	void ClearVerticalAlign();

	/// Returns 'border-width' properties from element's style or local cache.
	void GetBorderWidthProperties(const Property **border_top_width, const Property **border_bottom_width, const Property **border_left_width, const Property **border_right_width);
	/// Returns 'margin' properties from element's style or local cache.
	void GetMarginProperties(const Property **margin_top, const Property **margin_bottom, const Property **margin_left, const Property **margin_right);
	/// Returns 'padding' properties from element's style or local cache.
	void GetPaddingProperties(const Property **padding_top, const Property **padding_bottom, const Property **padding_left, const Property **padding_right);
	/// Returns 'width' and 'height' properties from element's style or local cache.
	void GetDimensionProperties(const Property **width, const Property **height);
	/// Returns local 'width' and 'height' properties from element's style or local cache,
	/// ignoring default values.
	void GetLocalDimensionProperties(const Property **width, const Property **height);
	/// Returns 'overflow' properties' values from element's style or local cache.
	void GetOverflow(int *overflow_x, int *overflow_y);
	/// Returns 'position' property value from element's style or local cache.
	int GetPosition();
	/// Returns 'float' property value from element's style or local cache.
	int GetFloat();
	/// Returns 'display' property value from element's style or local cache.
	int GetDisplay();
	/// Returns 'white-space' property value from element's style or local cache.
	int GetWhitespace();

	/// Returns 'line-height' property value from element's style or local cache.
	const Property *GetLineHeightProperty();
	/// Returns 'text-align' property value from element's style or local cache.
	int GetTextAlign();
	/// Returns 'text-transform' property value from element's style or local cache.
	int GetTextTransform();
	/// Returns 'vertical-align' property value from element's style or local cache.
	const Property *GetVerticalAlignProperty();

private:
	/// Element style that owns this cache instance.
	ElementStyle *style;

	/// Cached properties.
	const Property *border_top_width, *border_bottom_width, *border_left_width, *border_right_width;
	const Property *margin_top, *margin_bottom, *margin_left, *margin_right;
	const Property *padding_top, *padding_bottom, *padding_left, *padding_right;
	const Property *width, *height;
	const Property *local_width, *local_height;
	bool have_local_width, have_local_height;
	int overflow_x, overflow_y;
	int position;
	int float_;
	int display;
	int whitespace;
	const Property *line_height;
	int text_align;
	int text_transform;
	const Property *vertical_align;
};

}
}

#endif
