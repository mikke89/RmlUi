/*
 * Copyright (c) 2006 - 2008
 * Wandering Monster Studios Limited
 *
 * Any use of this program is governed by the terms of Wandering Monster
 * Studios Limited's Licence Agreement included with this program, a copy
 * of which can be obtained by contacting Wandering Monster Studios
 * Limited at info@wanderingmonster.co.nz.
 *
 */

// Lightweight, non-initialising constructor.
template < typename ColourType, int AlphaDefault >
Colour< ColourType, AlphaDefault >::Colour()
{
}

// Initialising constructor.
template < typename ColourType, int AlphaDefault >
Colour< ColourType, AlphaDefault >::Colour(ColourType _red, ColourType _green, ColourType _blue, ColourType _alpha)
{
	red = _red;
	green = _green;
	blue = _blue;
	alpha = _alpha;
}

// Returns the sum of this colour and another. This does not saturate the channels.
template < typename ColourType, int AlphaDefault >
Colour< ColourType, AlphaDefault > Colour< ColourType, AlphaDefault >::operator+(const Colour< ColourType, AlphaDefault >& rhs) const
{
	return Colour< ColourType, AlphaDefault >(red + rhs.red, green + rhs.green, blue + rhs.blue, alpha + rhs.alpha);
}

// Returns the result of subtracting another colour from this colour.
template < typename ColourType, int AlphaDefault >
Colour< ColourType, AlphaDefault > Colour< ColourType, AlphaDefault >::operator-(const Colour< ColourType, AlphaDefault >& rhs) const
{
	return Colour< ColourType, AlphaDefault >(red - rhs.red, green - rhs.green, blue - rhs.blue, alpha - rhs.alpha);
}

// Returns the result of multiplying this colour component-wise by a scalar.
template < typename ColourType, int AlphaDefault >
Colour< ColourType, AlphaDefault > Colour< ColourType, AlphaDefault >::operator*(float rhs) const
{
	return Colour((ColourType) (red * rhs), (ColourType) (green * rhs), (ColourType) (blue * rhs), (ColourType) (alpha * rhs));
}

// Returns the result of dividing this colour component-wise by a scalar.
template < typename ColourType, int AlphaDefault >
Colour< ColourType, AlphaDefault > Colour< ColourType, AlphaDefault >::operator/(float rhs) const
{
	return Colour((ColourType) (red / rhs), (ColourType) (green / rhs), (ColourType) (blue / rhs), (ColourType) (alpha / rhs));
}

// Adds another colour to this in-place. This does not saturate the channels.
template < typename ColourType, int AlphaDefault >
void Colour< ColourType, AlphaDefault >::operator+=(const Colour& rhs)
{
	red += rhs.red;
	green += rhs.green;
	blue += rhs.blue;
	alpha += rhs.alpha;
}

// Subtracts another colour from this in-place.
template < typename ColourType, int AlphaDefault >
void Colour< ColourType, AlphaDefault >::operator-=(const Colour& rhs)
{
	red -= rhs.red;
	green -= rhs.green;
	blue -= rhs.blue;
	alpha -= rhs.alpha;
}

// Scales this colour component-wise in-place.
template < typename ColourType, int AlphaDefault >
void Colour< ColourType, AlphaDefault >::operator*=(float rhs)
{
	red = (ColourType)(red * rhs);
	green = (ColourType)(green * rhs);
	blue = (ColourType)(blue * rhs);
	alpha = (ColourType)(alpha * rhs);
}

// Scales this colour component-wise in-place by the inverse of a value.
template < typename ColourType, int AlphaDefault >
void Colour< ColourType, AlphaDefault >::operator/=(float rhs)
{
	*this *= (1.0f / rhs);
}

template < >
Colour< float, 1 > ROCKETCORE_API Colour< float, 1 >::operator*(const Colour< float, 1 >& rhs) const;

template < >
Colour< byte, 255 > ROCKETCORE_API Colour< byte, 255 >::operator*(const Colour< byte, 255 >& rhs) const;

template < >
void ROCKETCORE_API Colour< float, 1 >::operator*=(const Colour& rhs);

template < >
void ROCKETCORE_API Colour< byte, 255 >::operator*=(const Colour& rhs);
