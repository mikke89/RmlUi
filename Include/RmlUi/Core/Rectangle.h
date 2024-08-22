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

#ifndef RMLUI_CORE_RECTANGLE_H
#define RMLUI_CORE_RECTANGLE_H

#include "Debug.h"
#include "Vector2.h"

namespace Rml {

/**
    Templated class for a generic axis-aligned rectangle.
 */
template <typename Type>
class Rectangle {
public:
	using Vector2Type = Vector2<Type>;

	Rectangle() = default;

	static inline Rectangle FromPosition(Vector2Type pos) { return Rectangle(pos, pos); }
	static inline Rectangle FromPositionSize(Vector2Type pos, Vector2Type size) { return Rectangle(pos, pos + size); }
	static inline Rectangle FromSize(Vector2Type size) { return Rectangle(Vector2Type(), size); }
	static inline Rectangle FromCorners(Vector2Type top_left, Vector2Type bottom_right) { return Rectangle(top_left, bottom_right); }
	static inline Rectangle MakeInvalid() { return Rectangle(Vector2Type(0), Vector2Type(-1)); }

	Vector2Type Position() const { return p0; }
	Vector2Type Size() const { return p1 - p0; }

	Vector2Type TopLeft() const { return p0; }
	Vector2Type TopRight() const { return {p1.x, p0.y}; }
	Vector2Type BottomRight() const { return p1; }
	Vector2Type BottomLeft() const { return {p0.x, p1.y}; }

	Vector2Type Center() const { return (p0 + p1) / Type(2); }

	Type Left() const { return p0.x; }
	Type Right() const { return p1.x; }
	Type Top() const { return p0.y; }
	Type Bottom() const { return p1.y; }
	Type Width() const { return p1.x - p0.x; }
	Type Height() const { return p1.y - p0.y; }

	Rectangle Extend(Type v) const { return Extend(Vector2Type(v)); }
	Rectangle Extend(Vector2Type v) const { return Rectangle{p0 - v, p1 + v}; }
	Rectangle Extend(Vector2Type top_left, Vector2Type bottom_right) const { return Rectangle{p0 - top_left, p1 + bottom_right}; }

	Rectangle Translate(Vector2Type v) const { return Rectangle{p0 + v, p1 + v}; }

	Rectangle Join(Vector2Type p) const { return Rectangle{Math::Min(p0, p), Math::Max(p1, p)}; }
	Rectangle Join(Rectangle other) const { return Rectangle{Math::Min(p0, other.p0), Math::Max(p1, other.p1)}; }

	Rectangle Intersect(Rectangle other) const
	{
		RMLUI_ASSERT(Valid() && other.Valid());
		Rectangle result{Math::Max(p0, other.p0), Math::Min(p1, other.p1)};
		result.p1 = Math::Max(result.p0, result.p1);
		return result;
	}
	Rectangle IntersectIfValid(Rectangle other) const
	{
		if (!Valid() || !other.Valid())
			return *this;
		return Intersect(other);
	}

	bool Intersects(Rectangle other) const { return p0.x < other.p1.x && p1.x > other.p0.x && p0.y < other.p1.y && p1.y > other.p0.y; }
	bool Contains(Vector2Type point) const { return point.x >= p0.x && point.x <= p1.x && point.y >= p0.y && point.y <= p1.y; }

	bool Valid() const { return p0.x <= p1.x && p0.y <= p1.y; }

	bool operator==(Rectangle rhs) const { return p0 == rhs.p0 && p1 == rhs.p1; }
	bool operator!=(Rectangle rhs) const { return !(*this == rhs); }

	template <typename U>
	explicit operator Rectangle<U>() const
	{
		return Rectangle<U>::FromCorners(static_cast<Vector2<U>>(p0), static_cast<Vector2<U>>(p1));
	}

	Vector2Type p0; // Minimum coordinates for a valid rectangle.
	Vector2Type p1; // Maximum coordinates for a valid rectangle.

private:
	Rectangle(Vector2Type p0, Vector2Type p1) : p0(p0), p1(p1) {}
};

} // namespace Rml

#endif
