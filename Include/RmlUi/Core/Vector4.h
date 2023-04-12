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

#ifndef RMLUI_CORE_VECTOR4_H
#define RMLUI_CORE_VECTOR4_H

#include "Debug.h"
#include "Math.h"
#include "Vector3.h"

namespace Rml {

/**
    Templated class for a generic four-component vector.
    @author Markus Schöngart
 */

template <typename Type>
class Vector4 {
public:
	/// Default constructor.
	inline Vector4();
	/// Initialising constructor.
	/// @param[in] v Initial value of each element in the vector.
	explicit inline Vector4(Type v);
	/// Initialising constructor.
	/// @param[in] x Initial x-value of the vector.
	/// @param[in] y Initial y-value of the vector.
	/// @param[in] z Initial z-value of the vector.
	/// @param[in] w Initial omega-value of the vector.
	inline Vector4(Type x, Type y, Type z, Type w);
	/// Implicit conversion from a 3D Vector.
	inline Vector4(Vector3<Type> const& v, Type w);

	/// Returns the magnitude of the vector.
	/// @return The computed magnitude.
	inline float Magnitude() const;
	/// Returns the squared magnitude of the vector.
	/// @return The computed squared magnitude.
	inline Type SquaredMagnitude() const;
	/// Generates a normalised vector from this vector.
	/// @return The normalised vector.
	inline Vector4 Normalise() const;

	/// Computes the dot-product between this vector and another.
	/// @param[in] rhs The other vector to use in the dot-product.
	/// @return The computed dot-product between the two vectors.
	inline Type DotProduct(const Vector4& rhs) const;

	/// Return a Vector3 after perspective divide
	inline Vector3<Type> PerspectiveDivide() const;

	/// Returns the negation of this vector.
	/// @return The negation of this vector.
	inline Vector4 operator-() const;

	/// Returns the sum of this vector and another.
	/// @param[in] rhs The vector to add this to.
	/// @return The sum of the two vectors.
	inline Vector4 operator+(const Vector4& rhs) const;
	/// Returns the result of subtracting another vector from this vector.
	/// @param[in] rhs The vector to subtract from this vector.
	/// @return The result of the subtraction.
	inline Vector4 operator-(const Vector4& rhs) const;
	/// Returns the result of multiplying this vector by a scalar.
	/// @param[in] rhs The scalar value to multiply by.
	/// @return The result of the scale.
	inline Vector4 operator*(Type rhs) const;
	/// Returns the result of dividing this vector by a scalar.
	/// @param[in] rhs The scalar value to divide by.
	/// @return The result of the scale.
	inline Vector4 operator/(Type rhs) const;

	/// Adds another vector to this in-place.
	/// @param[in] rhs The vector to add.
	/// @return This vector, post-operation.
	inline Vector4& operator+=(const Vector4& rhs);
	/// Subtracts another vector from this in-place.
	/// @param[in] rhs The vector to subtract.
	/// @return This vector, post-operation.
	inline Vector4& operator-=(const Vector4& rhs);
	/// Scales this vector in-place.
	/// @param[in] rhs The value to scale this vector's components by.
	/// @return This vector, post-operation.
	inline Vector4& operator*=(const Type& rhs);
	/// Scales this vector in-place by the inverse of a value.
	/// @param[in] rhs The value to divide this vector's components by.
	/// @return This vector, post-operation.
	inline Vector4& operator/=(const Type& rhs);

	/// Equality operator.
	/// @param[in] rhs The vector to compare this against.
	/// @return True if the two vectors are equal, false otherwise.
	inline bool operator==(const Vector4& rhs) const;
	/// Inequality operator.
	/// @param[in] rhs The vector to compare this against.
	/// @return True if the two vectors are not equal, false otherwise.
	inline bool operator!=(const Vector4& rhs) const;

	/// Auto-cast operator to array of components.
	/// @return A pointer to the first value.
	inline operator const Type*() const;
	/// Constant auto-cast operator to array of components.
	/// @return A constant pointer to the first value.
	inline operator Type*();

	/// Underlying type-cast operator.
	/// @return A copy of the vector with another underlying type.
	template <typename U>
	explicit inline operator Vector4<U>() const;

	/// Cast to Vector3
	explicit inline operator Vector3<Type>() const;
	/// Cast to Vector2
	explicit inline operator Vector2<Type>() const;

	// The components of the vector.
	Type x;
	Type y;
	Type z;
	Type w;

#if defined(RMLUI_VECTOR4_USER_EXTRA)
	RMLUI_VECTOR4_USER_EXTRA
#elif defined(RMLUI_VECTOR4_USER_INCLUDE)
	#include RMLUI_VECTOR4_USER_INCLUDE
#endif
};

} // namespace Rml

#include "Vector4.inl"

#endif
