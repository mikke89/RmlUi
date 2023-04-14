/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2014 Markus Schöngart
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

#include <type_traits>

namespace Rml {

template <typename Type>
Vector4<Type>::Vector4() : x{}, y{}, z{}, w{}
{}

template <typename Type>
Vector4<Type>::Vector4(Type v) : x(v), y(v), z(v), w(v)
{}

template <typename Type>
Vector4<Type>::Vector4(Type x, Type y, Type z, Type w) : x(x), y(y), z(z), w(w)
{}

template <typename Type>
Vector4<Type>::Vector4(Vector3<Type> const& v, Type w) : x(v.x), y(v.y), z(v.z), w(w)
{}

template <typename Type>
float Vector4<Type>::Magnitude() const
{
	return Math::SquareRoot(static_cast<float>(SquaredMagnitude()));
}

template <typename Type>
Type Vector4<Type>::SquaredMagnitude() const
{
	return x * x + y * y + z * z + w * w;
}

template <typename Type>
inline Vector4<Type> Vector4<Type>::Normalise() const
{
	static_assert(std::is_same<Type, float>::value, "Normalise only implemented for Vector<float>.");
	return *this;
}

template <>
inline Vector4<float> Vector4<float>::Normalise() const
{
	const float magnitude = Magnitude();
	if (Math::IsCloseToZero(magnitude))
		return *this;

	return *this / magnitude;
}

template <typename Type>
Type Vector4<Type>::DotProduct(const Vector4<Type>& rhs) const
{
	return x * rhs.x + y * rhs.y + z * rhs.z + w * rhs.w;
}

template <typename Type>
Vector3<Type> Vector4<Type>::PerspectiveDivide() const
{
	return Vector3<Type>(x / w, y / w, z / w);
}

template <typename Type>
Vector4<Type> Vector4<Type>::operator-() const
{
	return Vector4(-x, -y, -z, -w);
}

template <typename Type>
Vector4<Type> Vector4<Type>::operator+(const Vector4<Type>& rhs) const
{
	return Vector4<Type>(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w);
}

template <typename Type>
Vector4<Type> Vector4<Type>::operator-(const Vector4<Type>& rhs) const
{
	return Vector4(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w);
}

template <typename Type>
Vector4<Type> Vector4<Type>::operator*(Type rhs) const
{
	return Vector4(x * rhs, y * rhs, z * rhs, w * rhs);
}

template <typename Type>
Vector4<Type> Vector4<Type>::operator/(Type rhs) const
{
	return Vector4(x / rhs, y / rhs, z / rhs, w / rhs);
}

template <typename Type>
Vector4<Type>& Vector4<Type>::operator+=(const Vector4& rhs)
{
	x += rhs.x;
	y += rhs.y;
	z += rhs.z;
	w += rhs.w;

	return *this;
}

template <typename Type>
Vector4<Type>& Vector4<Type>::operator-=(const Vector4& rhs)
{
	x -= rhs.x;
	y -= rhs.y;
	z -= rhs.z;
	w -= rhs.w;

	return *this;
}

template <typename Type>
Vector4<Type>& Vector4<Type>::operator*=(const Type& rhs)
{
	x *= rhs;
	y *= rhs;
	z *= rhs;
	w *= rhs;

	return *this;
}

template <typename Type>
Vector4<Type>& Vector4<Type>::operator/=(const Type& rhs)
{
	x /= rhs;
	y /= rhs;
	z /= rhs;
	w /= rhs;

	return *this;
}

template <typename Type>
bool Vector4<Type>::operator==(const Vector4& rhs) const
{
	return (x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w);
}

template <typename Type>
bool Vector4<Type>::operator!=(const Vector4& rhs) const
{
	return (x != rhs.x || y != rhs.y || z != rhs.z || w != rhs.w);
}

template <typename Type>
Vector4<Type>::operator const Type*() const
{
	return &x;
}

template <typename Type>
Vector4<Type>::operator Type*()
{
	return &x;
}

template <typename Type>
template <typename U>
inline Vector4<Type>::operator Vector4<U>() const
{
	return Vector4<U>(static_cast<U>(x), static_cast<U>(y), static_cast<U>(z), static_cast<U>(w));
}

template <typename Type>
Vector4<Type>::operator Vector3<Type>() const
{
	return Vector3<Type>(x, y, z);
}

template <typename Type>
Vector4<Type>::operator Vector2<Type>() const
{
	return Vector2<Type>(x, y);
}

template <typename Type>
inline Vector4<Type> operator*(Type lhs, Vector4<Type> rhs)
{
	return rhs * lhs;
}

} // namespace Rml
