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

namespace Rml {

template <typename ColourType, int AlphaDefault, bool PremultipliedAlpha>
Colour<ColourType, AlphaDefault, PremultipliedAlpha>::Colour(ColourType rgb, ColourType alpha) : red(rgb), green(rgb), blue(rgb), alpha(alpha)
{}

template <typename ColourType, int AlphaDefault, bool PremultipliedAlpha>
Colour<ColourType, AlphaDefault, PremultipliedAlpha>::Colour(ColourType red, ColourType green, ColourType blue, ColourType alpha) :
	red(red), green(green), blue(blue), alpha(alpha)
{}

template <typename ColourType, int AlphaDefault, bool PremultipliedAlpha>
Colour<ColourType, AlphaDefault, PremultipliedAlpha> Colour<ColourType, AlphaDefault, PremultipliedAlpha>::operator+(
	const Colour<ColourType, AlphaDefault, PremultipliedAlpha> rhs) const
{
	return Colour<ColourType, AlphaDefault, PremultipliedAlpha>(red + rhs.red, green + rhs.green, blue + rhs.blue, alpha + rhs.alpha);
}

template <typename ColourType, int AlphaDefault, bool PremultipliedAlpha>
Colour<ColourType, AlphaDefault, PremultipliedAlpha> Colour<ColourType, AlphaDefault, PremultipliedAlpha>::operator-(
	const Colour<ColourType, AlphaDefault, PremultipliedAlpha> rhs) const
{
	return Colour<ColourType, AlphaDefault, PremultipliedAlpha>(red - rhs.red, green - rhs.green, blue - rhs.blue, alpha - rhs.alpha);
}

template <typename ColourType, int AlphaDefault, bool PremultipliedAlpha>
Colour<ColourType, AlphaDefault, PremultipliedAlpha> Colour<ColourType, AlphaDefault, PremultipliedAlpha>::operator*(float rhs) const
{
	return Colour((ColourType)(red * rhs), (ColourType)(green * rhs), (ColourType)(blue * rhs), (ColourType)(alpha * rhs));
}

template <typename ColourType, int AlphaDefault, bool PremultipliedAlpha>
Colour<ColourType, AlphaDefault, PremultipliedAlpha> Colour<ColourType, AlphaDefault, PremultipliedAlpha>::operator/(float rhs) const
{
	return Colour((ColourType)(red / rhs), (ColourType)(green / rhs), (ColourType)(blue / rhs), (ColourType)(alpha / rhs));
}

template <typename ColourType, int AlphaDefault, bool PremultipliedAlpha>
void Colour<ColourType, AlphaDefault, PremultipliedAlpha>::operator+=(const Colour rhs)
{
	red += rhs.red;
	green += rhs.green;
	blue += rhs.blue;
	alpha += rhs.alpha;
}

template <typename ColourType, int AlphaDefault, bool PremultipliedAlpha>
void Colour<ColourType, AlphaDefault, PremultipliedAlpha>::operator-=(const Colour rhs)
{
	red -= rhs.red;
	green -= rhs.green;
	blue -= rhs.blue;
	alpha -= rhs.alpha;
}

template <typename ColourType, int AlphaDefault, bool PremultipliedAlpha>
void Colour<ColourType, AlphaDefault, PremultipliedAlpha>::operator*=(float rhs)
{
	red = (ColourType)(red * rhs);
	green = (ColourType)(green * rhs);
	blue = (ColourType)(blue * rhs);
	alpha = (ColourType)(alpha * rhs);
}

template <typename ColourType, int AlphaDefault, bool PremultipliedAlpha>
void Colour<ColourType, AlphaDefault, PremultipliedAlpha>::operator/=(float rhs)
{
	*this *= (1.0f / rhs);
}

} // namespace Rml
