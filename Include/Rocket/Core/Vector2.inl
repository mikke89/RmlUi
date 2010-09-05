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

// Default constructor.
template < typename Type >
Vector2< Type >::Vector2()
{
}

// Initialising constructor.
template < typename Type >
Vector2< Type >::Vector2(Type _x, Type _y)
{
	x = _x;
	y = _y;
}

// Returns the magnitude of the vector.
template < typename Type >
float Vector2< Type >::Magnitude() const
{
	float squared_magnitude = (float) SquaredMagnitude();
	if (Math::IsZero(squared_magnitude))
		return 0;

	return Math::SquareRoot(squared_magnitude);
}

// Returns the squared magnitude of the vector.
template < typename Type >
Type Vector2< Type >::SquaredMagnitude() const
{
	return x * x +
		   y * y;
}

// Generates a normalised vector from this vector.
template < typename Type >
Vector2< Type > Vector2< Type >::Normalise() const
{
	EMP_STATIC_ASSERT(false, Invalid_Operation);
	return *this;
}

template <>
ROCKETCORE_API Vector2< float > Vector2< float >::Normalise() const;

// Computes the dot-product between this vector and another.
template < typename Type >
Type Vector2< Type >::DotProduct(const Vector2< Type >& rhs) const
{
	return x * rhs.x +
		   y * rhs.y;
}

// Returns this vector rotated around the origin.
template < typename Type >
Vector2< Type > Vector2< Type >::Rotate(float theta) const
{
	EMP_STATIC_ASSERT(false, Invalid_Operation);
	return *this;
}

template <>
ROCKETCORE_API Vector2< float > Vector2< float >::Rotate(float) const;

// Returns the negation of this vector.
template < typename Type >
Vector2< Type > Vector2< Type >::operator-() const
{
	return Vector2(-x, -y);
}

// Returns the sum of this vector and another.
template < typename Type >
Vector2< Type > Vector2< Type >::operator+(const Vector2< Type >& rhs) const
{
	return Vector2< Type >(x + rhs.x, y + rhs.y);
}

// Returns the result of subtracting another vector from this vector.
template < typename Type >
Vector2< Type > Vector2< Type >::operator-(const Vector2< Type >& rhs) const
{
	return Vector2(x - rhs.x, y - rhs.y);
}

// Returns the result of multiplying this vector by a scalar.
template < typename Type >
Vector2< Type > Vector2< Type >::operator*(Type rhs) const
{
	return Vector2(x * rhs, y * rhs);
}

// Returns the result of dividing this vector by a scalar.
template < typename Type >
Vector2< Type > Vector2< Type >::operator/(Type rhs) const
{
	return Vector2(x / rhs, y / rhs);
}

// Adds another vector to this in-place.
template < typename Type >
Vector2< Type >& Vector2< Type >::operator+=(const Vector2& rhs)
{
	x += rhs.x;
	y += rhs.y;

	return *this;
}

// Subtracts another vector from this in-place.
template < typename Type >
Vector2< Type >& Vector2< Type >::operator-=(const Vector2& rhs)
{
	x -= rhs.x;
	y -= rhs.y;

	return *this;
}

// Scales this vector in-place.
template < typename Type >
Vector2< Type >& Vector2< Type >::operator*=(const Type& rhs)
{
	x *= rhs;
	y *= rhs;

	return *this;
}

// Scales this vector in-place by the inverse of a value.
template < typename Type >
Vector2< Type >& Vector2< Type >::operator/=(const Type& rhs)
{
	x /= rhs;
	y /= rhs;

	return *this;
}

// Equality operator.
template < typename Type >
bool Vector2< Type >::operator==(const Vector2& rhs) const
{
	return (x == rhs.x && y == rhs.y);
}

// Inequality operator.
template < typename Type >
bool Vector2< Type >::operator!=(const Vector2& rhs) const
{
	return (x != rhs.x || y != rhs.y);
}

// Auto-cast operator.
template < typename Type >
Vector2< Type >::operator const Type*() const
{
	return &x;
}

// Constant auto-cast operator.
template < typename Type >
Vector2< Type >::operator Type*()
{
	return &x;
}
