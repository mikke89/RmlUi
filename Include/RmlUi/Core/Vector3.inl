/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2014 Markus Schöngart
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

#include <type_traits>

namespace Rml {
namespace Core {

// Initialising constructor.
template < typename Type >
Vector3< Type >::Vector3(Type v) : x(v), y(v), z(v)
{
}

// Initialising constructor.
template < typename Type >
Vector3< Type >::Vector3(Type x, Type y, Type z) : x(x), y(y), z(z)
{
}

// Returns the magnitude of the vector.
template < typename Type >
float Vector3< Type >::Magnitude() const
{
	float squared_magnitude = (float)SquaredMagnitude();
	if (Math::IsZero(squared_magnitude))
		return 0;

	return Math::SquareRoot(squared_magnitude);
}

// Returns the squared magnitude of the vector.
template < typename Type >
Type Vector3< Type >::SquaredMagnitude() const
{
	return x * x + y * y + z * z;
}

// Generates a normalised vector from this vector.
template < typename Type >
Vector3< Type > Vector3< Type >::Normalise() const
{
	static_assert(std::is_floating_point< Type >::value, "Invalid operation");
	return *this;
}

template <>
inline Vector3< float > Vector3< float >::Normalise() const
{
	float magnitude = Magnitude();
	if (Math::IsZero(magnitude))
		return *this;

	return *this / magnitude;
}

// Computes the dot-product between this vector and another.
template < typename Type >
Type Vector3< Type >::DotProduct(const Vector3< Type >& rhs) const
{
	return x * rhs.x + y * rhs.y + z * rhs.z;
}

// Computes the cross-product between this vector and another.
template < typename Type >
Vector3< Type> Vector3< Type >::CrossProduct(const Vector3< Type >& rhs) const
{
	return Vector3< Type >(
		y * rhs.z - z * rhs.y,
		z * rhs.x - x * rhs.z,
		x * rhs.y - y * rhs.x
		);
}

// Returns the negation of this vector.
template < typename Type >
Vector3< Type > Vector3< Type >::operator-() const
{
	return Vector3(-x, -y, -z);
}

// Returns the sum of this vector and another.
template < typename Type >
Vector3< Type > Vector3< Type >::operator+(const Vector3< Type > & rhs) const
{
	return Vector3< Type >(x + rhs.x, y + rhs.y, z + rhs.z);
}

// Returns the result of subtracting another vector from this vector.
template < typename Type >
Vector3< Type > Vector3< Type >::operator-(const Vector3< Type > & rhs) const
{
	return Vector3(x - rhs.x, y - rhs.y, z - rhs.z);
}

// Returns the result of multiplying this vector by a scalar.
template < typename Type >
Vector3< Type > Vector3< Type >::operator*(Type rhs) const
{
	return Vector3(x * rhs, y * rhs, z * rhs);
}

// Returns the result of dividing this vector by a scalar.
template < typename Type >
Vector3< Type > Vector3< Type >::operator/(Type rhs) const
{
	return Vector3(x / rhs, y / rhs, z / rhs);
}

// Adds another vector to this in-place.
template < typename Type >
Vector3< Type >& Vector3< Type >::operator+=(const Vector3 & rhs)
{
	x += rhs.x;
	y += rhs.y;
	z += rhs.z;

	return *this;
}

// Subtracts another vector from this in-place.
template < typename Type >
Vector3< Type >& Vector3< Type >::operator-=(const Vector3 & rhs)
{
	x -= rhs.x;
	y -= rhs.y;
	z -= rhs.z;

	return *this;
}

// Scales this vector in-place.
template < typename Type >
Vector3< Type >& Vector3< Type >::operator*=(const Type & rhs)
{
	x *= rhs;
	y *= rhs;
	z *= rhs;

	return *this;
}

// Scales this vector in-place by the inverse of a value.
template < typename Type >
Vector3< Type >& Vector3< Type >::operator/=(const Type & rhs)
{
	x /= rhs;
	y /= rhs;
	z /= rhs;

	return *this;
}

// Equality operator.
template < typename Type >
bool Vector3< Type >::operator==(const Vector3 & rhs) const
{
	return (x == rhs.x && y == rhs.y && z == rhs.z);
}

// Inequality operator.
template < typename Type >
bool Vector3< Type >::operator!=(const Vector3 & rhs) const
{
	return (x != rhs.x || y != rhs.y || z != rhs.z);
}

// Constant auto-cast operator.
template < typename Type >
Vector3< Type >::operator const Type* () const
{
	return &x;
}

// Auto-cast operator.
template < typename Type >
Vector3< Type >::operator Type* ()
{
	return &x;
}

template < typename Type >
Vector3< Type >::operator Vector2< Type >() const
{
	return Vector2< Type >(x, y);
}

}
}
