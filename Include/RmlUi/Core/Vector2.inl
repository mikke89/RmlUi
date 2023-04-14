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

template <typename Type>
Vector2<Type>::Vector2() : x{}, y{}
{}

template <typename Type>
Vector2<Type>::Vector2(Type v) : x(v), y(v)
{}

template <typename Type>
Vector2<Type>::Vector2(Type x, Type y) : x(x), y(y)
{}

template <typename Type>
float Vector2<Type>::Magnitude() const
{
	return Math::SquareRoot(static_cast<float>(SquaredMagnitude()));
}

template <typename Type>
Type Vector2<Type>::SquaredMagnitude() const
{
	return x * x + y * y;
}

template <typename Type>
inline Vector2<Type> Vector2<Type>::Normalise() const
{
	static_assert(std::is_same<Type, float>::value, "Normalise only implemented for Vector<float>.");
	return *this;
}

template <>
inline Vector2<float> Vector2<float>::Normalise() const
{
	const float magnitude = Magnitude();
	if (Math::IsCloseToZero(magnitude))
		return *this;

	return *this / magnitude;
}

template <>
inline Vector2<float> Vector2<float>::Round() const
{
	Vector2<float> result;
	result.x = Math::Round(x);
	result.y = Math::Round(y);
	return result;
}

template <>
inline Vector2<int> Vector2<int>::Round() const
{
	return *this;
}

template <typename Type>
Type Vector2<Type>::DotProduct(Vector2 rhs) const
{
	return x * rhs.x + y * rhs.y;
}

template <typename Type>
Vector2<Type> Vector2<Type>::Rotate(float theta) const
{
	float cos_theta = Math::Cos(theta);
	float sin_theta = Math::Sin(theta);

	return Vector2<Type>(((Type)(cos_theta * x - sin_theta * y)), ((Type)(sin_theta * x + cos_theta * y)));
}

template <typename Type>
Vector2<Type> Vector2<Type>::operator-() const
{
	return Vector2(-x, -y);
}

template <typename Type>
Vector2<Type> Vector2<Type>::operator+(Vector2 rhs) const
{
	return Vector2<Type>(x + rhs.x, y + rhs.y);
}

template <typename Type>
Vector2<Type> Vector2<Type>::operator-(Vector2 rhs) const
{
	return Vector2(x - rhs.x, y - rhs.y);
}

template <typename Type>
Vector2<Type> Vector2<Type>::operator*(Type rhs) const
{
	return Vector2(x * rhs, y * rhs);
}

template <typename Type>
Vector2<Type> Vector2<Type>::operator*(Vector2 rhs) const
{
	return Vector2(x * rhs.x, y * rhs.y);
}

template <typename Type>
Vector2<Type> Vector2<Type>::operator/(Type rhs) const
{
	return Vector2(x / rhs, y / rhs);
}

template <typename Type>
Vector2<Type> Vector2<Type>::operator/(Vector2 rhs) const
{
	return Vector2(x / rhs.x, y / rhs.y);
}

template <typename Type>
Vector2<Type>& Vector2<Type>::operator+=(Vector2 rhs)
{
	x += rhs.x;
	y += rhs.y;

	return *this;
}

template <typename Type>
Vector2<Type>& Vector2<Type>::operator-=(Vector2 rhs)
{
	x -= rhs.x;
	y -= rhs.y;

	return *this;
}

template <typename Type>
Vector2<Type>& Vector2<Type>::operator*=(Type rhs)
{
	x *= rhs;
	y *= rhs;

	return *this;
}

template <typename Type>
Vector2<Type>& Vector2<Type>::operator*=(Vector2 rhs)
{
	x *= rhs.x;
	y *= rhs.y;

	return *this;
}

template <typename Type>
Vector2<Type>& Vector2<Type>::operator/=(Type rhs)
{
	x /= rhs;
	y /= rhs;

	return *this;
}

template <typename Type>
Vector2<Type>& Vector2<Type>::operator/=(Vector2 rhs)
{
	x /= rhs.x;
	y /= rhs.y;
	return *this;
}

template <typename Type>
bool Vector2<Type>::operator==(Vector2 rhs) const
{
	return (x == rhs.x && y == rhs.y);
}

template <typename Type>
bool Vector2<Type>::operator!=(Vector2 rhs) const
{
	return (x != rhs.x || y != rhs.y);
}

template <typename Type>
Vector2<Type>::operator const Type*() const
{
	return &x;
}

template <typename Type>
Vector2<Type>::operator Type*()
{
	return &x;
}

template <typename Type>
template <typename U>
inline Vector2<Type>::operator Vector2<U>() const
{
	return Vector2<U>(static_cast<U>(x), static_cast<U>(y));
}

template <typename Type>
inline Vector2<Type> operator*(Type lhs, Vector2<Type> rhs)
{
	return rhs * lhs;
}

} // namespace Rml
