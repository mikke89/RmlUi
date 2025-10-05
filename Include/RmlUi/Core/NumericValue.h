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

#ifndef RMLUI_CORE_NUMERICVALUE_H
#define RMLUI_CORE_NUMERICVALUE_H

#include "Unit.h"

namespace Rml {

/**
    A numeric value is a number combined with a unit.
 */
struct NumericValue {
	NumericValue() noexcept : number(0.f), unit(Unit::UNKNOWN) {}
	NumericValue(float number, Unit unit) noexcept : number(number), unit(unit) {}
	float number;
	Unit unit;
};
inline bool operator==(const NumericValue& a, const NumericValue& b)
{
	return a.number == b.number && a.unit == b.unit;
}
inline bool operator!=(const NumericValue& a, const NumericValue& b)
{
	return !(a == b);
}

} // namespace Rml
namespace std {
template <>
struct hash<::Rml::NumericValue> {
	size_t operator()(const ::Rml::NumericValue& v) const noexcept
	{
		using namespace ::Rml;
		size_t h1 = hash<float>{}(v.number);
		size_t h2 = hash<Unit>{}(v.unit);
		return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
	}
};
} // namespace std
#endif
