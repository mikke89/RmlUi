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

#ifndef RMLUI_CORE_COLOUR_H
#define RMLUI_CORE_COLOUR_H

#include "Header.h"

namespace Rml {

using byte = unsigned char;

/**
    Templated class for a four-component RGBA colour.

    @author Peter Curry
 */

template <typename ColourType, int AlphaDefault, bool PremultipliedAlpha>
class Colour {
public:
	/// Initialising constructor.
	/// @param[in] rgb Initial red, green and blue value of the colour.
	/// @param[in] alpha Initial alpha value of the colour.
	inline Colour(ColourType rgb = ColourType{0}, ColourType alpha = ColourType{AlphaDefault});
	/// Initialising constructor.
	/// @param[in] red Initial red value of the colour.
	/// @param[in] green Initial green value of the colour.
	/// @param[in] blue Initial blue value of the colour.
	/// @param[in] alpha Initial alpha value of the colour.
	inline Colour(ColourType red, ColourType green, ColourType blue, ColourType alpha = ColourType{AlphaDefault});

	/// Returns the sum of this colour and another. This does not saturate the channels.
	/// @param[in] rhs The colour to add this to.
	/// @return The sum of the two colours.
	inline Colour operator+(Colour rhs) const;
	/// Returns the result of subtracting another colour from this colour.
	/// @param[in] rhs The colour to subtract from this colour.
	/// @return The result of the subtraction.
	inline Colour operator-(Colour rhs) const;
	/// Returns the result of multiplying this colour component-wise by a scalar.
	/// @param[in] rhs The scalar value to multiply by.
	/// @return The result of the scale.
	inline Colour operator*(float rhs) const;
	/// Returns the result of dividing this colour component-wise by a scalar.
	/// @param[in] rhs The scalar value to divide by.
	/// @return The result of the scale.
	inline Colour operator/(float rhs) const;

	/// Adds another colour to this in-place. This does not saturate the channels.
	/// @param[in] rhs The colour to add.
	inline void operator+=(Colour rhs);
	/// Subtracts another colour from this in-place.
	/// @param[in] rhs The colour to subtract.
	inline void operator-=(Colour rhs);
	/// Scales this colour component-wise in-place.
	/// @param[in] rhs The value to scale this colours's components by.
	inline void operator*=(float rhs);
	/// Scales this colour component-wise in-place by the inverse of a value.
	/// @param[in] rhs The value to divide this colour's components by.
	inline void operator/=(float rhs);

	/// Equality operator.
	/// @param[in] rhs The colour to compare this against.
	/// @return True if the two colours are equal, false otherwise.
	inline bool operator==(Colour rhs) const { return red == rhs.red && green == rhs.green && blue == rhs.blue && alpha == rhs.alpha; }
	/// Inequality operator.
	/// @param[in] rhs The colour to compare this against.
	/// @return True if the two colours are not equal, false otherwise.
	inline bool operator!=(Colour rhs) const { return !(*this == rhs); }

	/// Auto-cast operator.
	/// @return A pointer to the first value.
	inline operator const ColourType*() const { return &red; }
	/// Constant auto-cast operator.
	/// @return A constant pointer to the first value.
	inline operator ColourType*() { return &red; }

	// Convert color to premultiplied alpha.
	template <typename IsPremultiplied = std::integral_constant<bool, PremultipliedAlpha>,
		typename = typename std::enable_if_t<!IsPremultiplied::value && std::is_same<ColourType, byte>::value>>
	inline Colour<ColourType, AlphaDefault, true> ToPremultiplied() const
	{
		return {
			ColourType((red * alpha) / 255),
			ColourType((green * alpha) / 255),
			ColourType((blue * alpha) / 255),
			alpha,
		};
	}
	// Convert color to premultiplied alpha, after multiplying alpha by opacity.
	template <typename IsPremultiplied = std::integral_constant<bool, PremultipliedAlpha>,
		typename = typename std::enable_if_t<!IsPremultiplied::value && std::is_same<ColourType, byte>::value>>
	inline Colour<ColourType, AlphaDefault, true> ToPremultiplied(float opacity) const
	{
		const float new_alpha = alpha * opacity;
		return {
			ColourType(red * (new_alpha / 255.f)),
			ColourType(green * (new_alpha / 255.f)),
			ColourType(blue * (new_alpha / 255.f)),
			ColourType(new_alpha),
		};
	}

	// Convert color to non-premultiplied alpha.
	template <typename IsPremultiplied = std::integral_constant<bool, PremultipliedAlpha>,
		typename = typename std::enable_if_t<IsPremultiplied::value && std::is_same<ColourType, byte>::value>>
	inline Colour<ColourType, AlphaDefault, false> ToNonPremultiplied() const
	{
		return {
			ColourType(alpha > 0 ? (red * 255) / alpha : 0),
			ColourType(alpha > 0 ? (green * 255) / alpha : 0),
			ColourType(alpha > 0 ? (blue * 255) / alpha : 0),
			ColourType(alpha),
		};
	}

	ColourType red, green, blue, alpha;

#if defined(RMLUI_COLOUR_USER_EXTRA)
	RMLUI_COLOUR_USER_EXTRA
#elif defined(RMLUI_COLOUR_USER_INCLUDE)
	#include RMLUI_COLOUR_USER_INCLUDE
#endif
};

} // namespace Rml

#include "Colour.inl"

#endif
