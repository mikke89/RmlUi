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
Vector4< Type >::Vector4(Type v) 
	: x(v), y(v), z(v), w(v)
{
}

// Initialising constructor.
template < typename Type >
Vector4< Type >::Vector4(Type x, Type y, Type z, Type w)
	: x(x), y(y), z(z), w(w)
{
}

// Implicit conversion from a 3D Vector.
template < typename Type >
Vector4< Type >::Vector4(Vector3< Type > const& v, Type w)
	: x(v.x), y(v.y), z(v.z), w(w)
{
}

// Returns the magnitude of the vector.
template < typename Type >
float Vector4< Type >::Magnitude() const
{
	float squared_magnitude = (float)SquaredMagnitude();
	if (Math::IsZero(squared_magnitude))
		return 0;

	return Math::SquareRoot(squared_magnitude);
}

// Returns the squared magnitude of the vector.
template < typename Type >
Type Vector4< Type >::SquaredMagnitude() const
{
	return x * x + y * y + z * z + w * w;
}

// Generates a normalised vector from this vector.
template < typename Type >
Vector4< Type > Vector4< Type >::Normalise() const
{
	static_assert(std::is_floating_point< Type >::value, "Invalid operation");
	return *this;
}

template <>
RMLUICORE_API Vector4< float > Vector4< float >::Normalise() const;

// Computes the dot-product between this vector and another.
template < typename Type >
Type Vector4< Type >::DotProduct(const Vector4< Type >& rhs) const
{
	return x * rhs.x + y * rhs.y + z * rhs.z + w * rhs.w;
}

// Returns the negation of this vector.
template < typename Type >
Vector4< Type > Vector4< Type >::operator-() const
{
	return Vector4(-x, -y, -z, -w);
}

// Returns the sum of this vector and another.
template < typename Type >
Vector4< Type > Vector4< Type >::operator+(const Vector4< Type > & rhs) const
{
	return Vector4< Type >(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w);
}

// Returns the result of subtracting another vector from this vector.
template < typename Type >
Vector4< Type > Vector4< Type >::operator-(const Vector4< Type > & rhs) const
{
	return Vector4(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w);
}

// Returns the result of multiplying this vector by a scalar.
template < typename Type >
Vector4< Type > Vector4< Type >::operator*(Type rhs) const
{
	return Vector4(x * rhs, y * rhs, z * rhs, w * rhs);
}

// Returns the result of dividing this vector by a scalar.
template < typename Type >
Vector4< Type > Vector4< Type >::operator/(Type rhs) const
{
	return Vector4(x / rhs, y / rhs, z / rhs, w / rhs);
}

// Adds another vector to this in-place.
template < typename Type >
Vector4< Type >& Vector4< Type >::operator+=(const Vector4 & rhs)
{
	x += rhs.x;
	y += rhs.y;
	z += rhs.z;
	w += rhs.w;

	return *this;
}

// Subtracts another vector from this in-place.
template < typename Type >
Vector4< Type >& Vector4< Type >::operator-=(const Vector4 & rhs)
{
	x -= rhs.x;
	y -= rhs.y;
	z -= rhs.z;
	w -= rhs.w;

	return *this;
}

// Scales this vector in-place.
template < typename Type >
Vector4< Type >& Vector4< Type >::operator*=(const Type & rhs)
{
	x *= rhs;
	y *= rhs;
	z *= rhs;
	w *= rhs;

	return *this;
}

// Scales this vector in-place by the inverse of a value.
template < typename Type >
Vector4< Type >& Vector4< Type >::operator/=(const Type & rhs)
{
	x /= rhs;
	y /= rhs;
	z /= rhs;
	w /= rhs;

	return *this;
}

// Equality operator.
template < typename Type >
bool Vector4< Type >::operator==(const Vector4 & rhs) const
{
	return (x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w);
}

// Inequality operator.
template < typename Type >
bool Vector4< Type >::operator!=(const Vector4 & rhs) const
{
	return (x != rhs.x || y != rhs.y || z != rhs.z || w != rhs.w);
}

// Auto-cast operator.
template < typename Type >
Vector4< Type >::operator const Type* () const
{
	return &x;
}

// Constant auto-cast operator.
template < typename Type >
Vector4< Type >::operator Type* ()
{
	return &x;
}

template < typename Type >
Vector3< Type > Vector4< Type >::PerspectiveDivide() const
{
	return Vector3< Type >(x / w, y / w, z / w);
}

template < typename Type >
Vector4< Type >::operator Vector3< Type >() const
{
	return Vector3< Type >(x, y, z);
}

template < typename Type >
Vector4< Type >::operator Vector2< Type >() const
{
	return Vector2< Type >(x, y);
}

}
}
