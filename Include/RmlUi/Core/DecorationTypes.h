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

#ifndef RMLUI_CORE_DECORATIONTYPES_H
#define RMLUI_CORE_DECORATIONTYPES_H

#include "NumericValue.h"
#include "Types.h"

namespace Rml {

struct ColorStop {
	ColourbPremultiplied color;
	NumericValue position;
};
inline bool operator==(const ColorStop& a, const ColorStop& b)
{
	return a.color == b.color && a.position == b.position;
}
inline bool operator!=(const ColorStop& a, const ColorStop& b)
{
	return !(a == b);
}

struct BoxShadow {
	ColourbPremultiplied color;
	NumericValue offset_x, offset_y;
	NumericValue blur_radius;
	NumericValue spread_distance;
	bool inset = false;
};
inline bool operator==(const BoxShadow& a, const BoxShadow& b)
{
	return a.color == b.color && a.offset_x == b.offset_x && a.offset_y == b.offset_y && a.blur_radius == b.blur_radius &&
		a.spread_distance == b.spread_distance && a.inset == b.inset;
}
inline bool operator!=(const BoxShadow& a, const BoxShadow& b)
{
	return !(a == b);
}

} // namespace Rml
#endif
