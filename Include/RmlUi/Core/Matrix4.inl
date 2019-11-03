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

namespace Rml {
namespace Core {

// Initialising constructor.
template< typename Component, class Storage >
Matrix4< Component, Storage >::Matrix4(
	const typename Matrix4< Component, Storage >::VectorType& vec0,
	const typename Matrix4< Component, Storage >::VectorType& vec1,
	const typename Matrix4< Component, Storage >::VectorType& vec2,
	const typename Matrix4< Component, Storage >::VectorType& vec3
) noexcept
{
	vectors[0] = vec0;
	vectors[1] = vec1;
	vectors[2] = vec2;
	vectors[3] = vec3;
}

// Default constructor.
template< typename Component, class Storage >
Matrix4< Component, Storage >::Matrix4() noexcept
	: vectors{ VectorType{0}, VectorType{0}, VectorType{0}, VectorType{0} }
{
}

// Initialising, copy constructor.
template< typename Component, class Storage >
Matrix4< Component, Storage >::Matrix4(const typename Matrix4< Component, Storage >::ThisType& other) noexcept
{
	for (int i = 0; i < 4; ++i)
	{
		vectors[i] = other.vectors[i];
	}
}

template< typename Component, class Storage >
Matrix4< Component, Storage >::Matrix4(const typename Matrix4< Component, Storage >::TransposeType& other) noexcept
{
	Rows rows(vectors);
	typename Matrix4< Component, Storage >::TransposeType::ConstRows other_rows(other.vectors);
	for (int i = 0; i < 4; ++i)
	{
		rows[i] = other_rows[i];
	}
}

// Assignment operator
template< typename Component, class Storage >
const typename Matrix4< Component, Storage >::ThisType& Matrix4< Component, Storage >::operator=(const typename Matrix4< Component, Storage >::ThisType& other) noexcept
{
	for (int i = 0; i < 4; ++i)
	{
		vectors[i] = other.vectors[i];
	}
	return *this;
}

template< typename Component, class Storage >
const typename Matrix4< Component, Storage >::ThisType& Matrix4< Component, Storage >::operator=(const typename Matrix4< Component, Storage >::TransposeType& other) noexcept
{
	Rows rows(vectors);
	typename Matrix4< Component, Storage >::TransposeType::Rows other_rows(other.vectors);
	for (int i = 0; i < 4; ++i)
	{
		rows[i] = other_rows[i];
	}
	return *this;
}

// Construct from row vectors.
template< typename Component, class Storage >
const typename Matrix4< Component, Storage >::ThisType Matrix4< Component, Storage >::FromRows(
	const typename Matrix4< Component, Storage >::VectorType& vec0,
	const typename Matrix4< Component, Storage >::VectorType& vec1,
	const typename Matrix4< Component, Storage >::VectorType& vec2,
	const typename Matrix4< Component, Storage >::VectorType& vec3
) noexcept
{
	typename Matrix4< Component, Storage >::ThisType result;
	result.SetRows(vec0, vec1, vec2, vec3);
	return result;
}

// Construct from column vectors.
template< typename Component, class Storage >
const typename Matrix4< Component, Storage >::ThisType Matrix4< Component, Storage >::FromColumns(
	const typename Matrix4< Component, Storage >::VectorType& vec0,
	const typename Matrix4< Component, Storage >::VectorType& vec1,
	const typename Matrix4< Component, Storage >::VectorType& vec2,
	const typename Matrix4< Component, Storage >::VectorType& vec3
) noexcept
{
	typename Matrix4< Component, Storage >::ThisType result;
	result.SetColumns(vec0, vec1, vec2, vec3);
	return result;
}

// Construct from components
template< typename Component, class Storage >
const typename Matrix4< Component, Storage >::ThisType Matrix4< Component, Storage >::FromRowMajor(const Component* components) noexcept
{
	Matrix4< Component, Storage >::ThisType result;
	Matrix4< Component, Storage >::Rows rows(result.vectors);
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			rows[i][j] = components[i*4 + j];
		}
	}
	return result;
}
template< typename Component, class Storage >
const typename Matrix4< Component, Storage >::ThisType Matrix4< Component, Storage >::FromColumnMajor(const Component* components) noexcept
{
	Matrix4< Component, Storage >::ThisType result;
	Matrix4< Component, Storage >::Columns columns(result.vectors);
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			columns[i][j] = components[i*4 + j];
		}
	}
	return result;
}

// Set all rows
template< typename Component, class Storage >
void Matrix4< Component, Storage >::SetRows(const VectorType& vec0, const VectorType& vec1, const VectorType& vec2, const VectorType& vec3) noexcept
{
	Rows rows(vectors);
	rows[0] = vec0;
	rows[1] = vec1;
	rows[2] = vec2;
	rows[3] = vec3;
}

// Set all columns
template< typename Component, class Storage >
void Matrix4< Component, Storage >::SetColumns(const VectorType& vec0, const VectorType& vec1, const VectorType& vec2, const VectorType& vec3) noexcept
{
	Columns columns(vectors);
	columns[0] = vec0;
	columns[1] = vec1;
	columns[2] = vec2;
	columns[3] = vec3;
}

// Inverts this matrix in place.
// This is from the MESA implementation of the GLU library.
template< typename Component, class Storage >
bool Matrix4< Component, Storage >::Invert() noexcept
{
	Matrix4< Component, Storage >::ThisType result;
	Component *dst = result.data();
	const Component *src = data();

	dst[0] = src[5]  * src[10] * src[15] -
		src[5]  * src[11] * src[14] -
		src[9]  * src[6]  * src[15] +
		src[9]  * src[7]  * src[14] +
		src[13] * src[6]  * src[11] -
		src[13] * src[7]  * src[10];

	dst[4] = -src[4]  * src[10] * src[15] +
		src[4]  * src[11] * src[14] +
		src[8]  * src[6]  * src[15] -
		src[8]  * src[7]  * src[14] -
		src[12] * src[6]  * src[11] +
		src[12] * src[7]  * src[10];

	dst[8] = src[4]  * src[9] * src[15] -
		src[4]  * src[11] * src[13] -
		src[8]  * src[5] * src[15] +
		src[8]  * src[7] * src[13] +
		src[12] * src[5] * src[11] -
		src[12] * src[7] * src[9];

	dst[12] = -src[4]  * src[9] * src[14] +
		src[4]  * src[10] * src[13] +
		src[8]  * src[5] * src[14] -
		src[8]  * src[6] * src[13] -
		src[12] * src[5] * src[10] +
		src[12] * src[6] * src[9];

	dst[1] = -src[1]  * src[10] * src[15] +
		src[1]  * src[11] * src[14] +
		src[9]  * src[2] * src[15] -
		src[9]  * src[3] * src[14] -
		src[13] * src[2] * src[11] +
		src[13] * src[3] * src[10];

	dst[5] = src[0]  * src[10] * src[15] -
		src[0]  * src[11] * src[14] -
		src[8]  * src[2] * src[15] +
		src[8]  * src[3] * src[14] +
		src[12] * src[2] * src[11] -
		src[12] * src[3] * src[10];

	dst[9] = -src[0]  * src[9] * src[15] +
		src[0]  * src[11] * src[13] +
		src[8]  * src[1] * src[15] -
		src[8]  * src[3] * src[13] -
		src[12] * src[1] * src[11] +
		src[12] * src[3] * src[9];

	dst[13] = src[0]  * src[9] * src[14] -
		src[0]  * src[10] * src[13] -
		src[8]  * src[1] * src[14] +
		src[8]  * src[2] * src[13] +
		src[12] * src[1] * src[10] -
		src[12] * src[2] * src[9];

	dst[2] = src[1]  * src[6] * src[15] -
		src[1]  * src[7] * src[14] -
		src[5]  * src[2] * src[15] +
		src[5]  * src[3] * src[14] +
		src[13] * src[2] * src[7] -
		src[13] * src[3] * src[6];

	dst[6] = -src[0]  * src[6] * src[15] +
		src[0]  * src[7] * src[14] +
		src[4]  * src[2] * src[15] -
		src[4]  * src[3] * src[14] -
		src[12] * src[2] * src[7] +
		src[12] * src[3] * src[6];

	dst[10] = src[0]  * src[5] * src[15] -
		src[0]  * src[7] * src[13] -
		src[4]  * src[1] * src[15] +
		src[4]  * src[3] * src[13] +
		src[12] * src[1] * src[7] -
		src[12] * src[3] * src[5];

	dst[14] = -src[0]  * src[5] * src[14] +
		src[0]  * src[6] * src[13] +
		src[4]  * src[1] * src[14] -
		src[4]  * src[2] * src[13] -
		src[12] * src[1] * src[6] +
		src[12] * src[2] * src[5];

	dst[3] = -src[1] * src[6] * src[11] +
		src[1] * src[7] * src[10] +
		src[5] * src[2] * src[11] -
		src[5] * src[3] * src[10] -
		src[9] * src[2] * src[7] +
		src[9] * src[3] * src[6];

	dst[7] = src[0] * src[6] * src[11] -
		src[0] * src[7] * src[10] -
		src[4] * src[2] * src[11] +
		src[4] * src[3] * src[10] +
		src[8] * src[2] * src[7] -
		src[8] * src[3] * src[6];

	dst[11] = -src[0] * src[5] * src[11] +
		src[0] * src[7] * src[9] +
		src[4] * src[1] * src[11] -
		src[4] * src[3] * src[9] -
		src[8] * src[1] * src[7] +
		src[8] * src[3] * src[5];

	dst[15] = src[0] * src[5] * src[10] -
		src[0] * src[6] * src[9] -
		src[4] * src[1] * src[10] +
		src[4] * src[2] * src[9] +
		src[8] * src[1] * src[6] -
		src[8] * src[2] * src[5];

	float det = src[0] * dst[0] + \
		src[1] * dst[4] + \
		src[2] * dst[8] + \
		src[3] * dst[12];

	if (det == 0)
	{
		return false;
	}

	*this = result * (1 / det);
	return true;
}




template<typename Component, class Storage>
inline float Rml::Core::Matrix4<Component, Storage>::Determinant() const noexcept
{
	const Component *src = data();
	float diag[4]; // Diagonal elements of the matrix inverse (see Invert)

	diag[0] = src[5] * src[10] * src[15] -
		src[5] * src[11] * src[14] -
		src[9] * src[6] * src[15] +
		src[9] * src[7] * src[14] +
		src[13] * src[6] * src[11] -
		src[13] * src[7] * src[10];

	diag[1] = -src[4] * src[10] * src[15] +
		src[4] * src[11] * src[14] +
		src[8] * src[6] * src[15] -
		src[8] * src[7] * src[14] -
		src[12] * src[6] * src[11] +
		src[12] * src[7] * src[10];

	diag[2] = src[4] * src[9] * src[15] -
		src[4] * src[11] * src[13] -
		src[8] * src[5] * src[15] +
		src[8] * src[7] * src[13] +
		src[12] * src[5] * src[11] -
		src[12] * src[7] * src[9];

	diag[3] = -src[4] * src[9] * src[14] +
		src[4] * src[10] * src[13] +
		src[8] * src[5] * src[14] -
		src[8] * src[6] * src[13] -
		src[12] * src[5] * src[10] +
		src[12] * src[6] * src[9];

	float det = src[0] * diag[0] + \
		src[1] * diag[1] + \
		src[2] * diag[2] + \
		src[3] * diag[3];

	return det;
}

// Returns the negation of this matrix.
template< typename Component, class Storage >
typename Matrix4< Component, Storage >::ThisType Matrix4< Component, Storage >::operator-() const noexcept
{
	return typename Matrix4< Component, Storage >::ThisType(
		-vectors[0],
		-vectors[1],
		-vectors[2],
		-vectors[3]
	);
}

// Adds another matrix to this in-place.
template< typename Component, class Storage>
const typename Matrix4< Component, Storage >::ThisType& Matrix4< Component, Storage >::operator+=(const typename Matrix4< Component, Storage >::ThisType& other) noexcept
{
	for (int i = 0; i < 4; ++i)
	{
		vectors[i] += other.vectors[i];
	}
	return *this;
}
template< typename Component, class Storage>
const typename Matrix4< Component, Storage >::ThisType& Matrix4< Component, Storage >::operator+=(const typename Matrix4< Component, Storage >::TransposeType& other) noexcept
{
	Rows rows(vectors);
	typename Matrix4< Component, Storage >::TransposeType::ConstRows other_rows(other);
	for (int i = 0; i < 4; ++i)
	{
		rows[i] += other_rows[i];
	}
	return *this;
}

// Subtracts another matrix from this in-place.
template< typename Component, class Storage>
const typename Matrix4< Component, Storage >::ThisType& Matrix4< Component, Storage >::operator-=(const typename Matrix4< Component, Storage >::ThisType& other) noexcept
{
	for (int i = 0; i < 4; ++i)
	{
		vectors[i] -= other.vectors[i];
	}
	return *this;
}
template< typename Component, class Storage>
const typename Matrix4< Component, Storage >::ThisType& Matrix4< Component, Storage >::operator-=(const typename Matrix4< Component, Storage >::TransposeType& other) noexcept
{
	Rows rows(vectors);
	typename Matrix4< Component, Storage >::TransposeType::ConstRows other_rows(other);
	for (int i = 0; i < 4; ++i)
	{
		rows[i] -= other_rows[i];
	}
	return *this;
}

// Scales this matrix in-place.
template< typename Component, class Storage>
const typename Matrix4< Component, Storage >::ThisType& Matrix4< Component, Storage >::operator*=(Component s) noexcept
{
	for (int i = 0; i < 4; ++i)
	{
		vectors[i] *= s;
	}
	return *this;
}

// Scales this matrix in-place by the inverse of a value.
template< typename Component, class Storage>
const typename Matrix4< Component, Storage >::ThisType& Matrix4< Component, Storage >::operator/=(Component s) noexcept
{
	for (int i = 0; i < 4; ++i)
	{
		vectors[i] /= s;
	}
	return *this;
}

// Equality operator.
template< typename Component, class Storage>
bool Matrix4< Component, Storage >::operator==(const typename Matrix4< Component, Storage >::ThisType& other) const noexcept
{
	typename Matrix4< Component, Storage >::ConstRows rows(vectors);
	typename Matrix4< Component, Storage >::ConstRows other_rows(other.vectors);
	return vectors[0] == other.vectors[0]
	   && vectors[1] == other.vectors[1]
	   && vectors[2] == other.vectors[2]
	   && vectors[3] == other.vectors[3];
}
template< typename Component, class Storage>
bool Matrix4< Component, Storage >::operator==(const typename Matrix4< Component, Storage >::TransposeType& other) const noexcept
{
	typename Matrix4< Component, Storage >::ConstRows rows(vectors);
	typename Matrix4< Component, Storage >::ConstRows other_rows(other.vectors);
	return rows[0] == other_rows[0]
	   && rows[1] == other_rows[1]
	   && rows[2] == other_rows[2]
	   && rows[3] == other_rows[3];
}

// Inequality operator.
template< typename Component, class Storage>
bool Matrix4< Component, Storage >::operator!=(const typename Matrix4< Component, Storage >::ThisType& other) const noexcept
{
	return vectors[0] != other.vectors[0]
	    || vectors[1] != other.vectors[1]
	    || vectors[2] != other.vectors[2]
	    || vectors[3] != other.vectors[3];
}
template< typename Component, class Storage>
bool Matrix4< Component, Storage >::operator!=(const typename Matrix4< Component, Storage >::TransposeType& other) const noexcept
{
	typename Matrix4< Component, Storage >::ConstRows rows(vectors);
	typename Matrix4< Component, Storage >::ConstRows other_rows(other.vectors);
	return rows[0] != other_rows[0]
	    || rows[1] != other_rows[1]
	    || rows[2] != other_rows[2]
	    || rows[3] != other_rows[3];
}

// Return the identity matrix.
template< typename Component, class Storage>
const Matrix4< Component, Storage >& Matrix4< Component, Storage >::Identity() noexcept
{
	static Matrix4< Component, Storage > identity(Diag(1, 1, 1, 1));
	return identity;
}

// Return a diagonal matrix.
template< typename Component, class Storage>
Matrix4< Component, Storage > Matrix4< Component, Storage >::Diag(Component a, Component b, Component c, Component d) noexcept
{
	return Matrix4< Component, Storage >::FromRows(
		Matrix4< Component, Storage >::VectorType(a, 0, 0, 0),
		Matrix4< Component, Storage >::VectorType(0, b, 0, 0),
		Matrix4< Component, Storage >::VectorType(0, 0, c, 0),
		Matrix4< Component, Storage >::VectorType(0, 0, 0, d)
	);
}

// Create an orthographic projection matrix
template< typename Component, class Storage>
Matrix4< Component, Storage > Matrix4< Component, Storage >::ProjectOrtho(Component l, Component r, Component b, Component t, Component n, Component f) noexcept
{
	return Matrix4< Component, Storage >::FromRows(
		Matrix4< Component, Storage >::VectorType(2 / (r - l), 0, 0, -(r + l)/(r - l)),
		Matrix4< Component, Storage >::VectorType(0, 2 / (t - b), 0, -(t + b)/(t - b)),
		Matrix4< Component, Storage >::VectorType(0, 0, 2 / (f - n), -(f + n)/(f - n)),
		Matrix4< Component, Storage >::VectorType(0, 0, 0, 1)
	);
}

// Create a perspective projection matrix
template< typename Component, class Storage>
Matrix4< Component, Storage > Matrix4< Component, Storage >::ProjectPerspective(Component l, Component r, Component b, Component t, Component n, Component f) noexcept
{
	return Matrix4< Component, Storage >::FromRows(
		Matrix4< Component, Storage >::VectorType(2 * n / (r - l), 0, (r + l)/(r - l), 0),
		Matrix4< Component, Storage >::VectorType(0, 2 * n / (t - b), (t + b)/(t - b), 0),
		Matrix4< Component, Storage >::VectorType(0, 0, -(f + n)/(f - n), -(2 * f * n)/(f - n)),
		Matrix4< Component, Storage >::VectorType(0, 0, -1, 0)
	);
}

// Create a perspective projection matrix
template< typename Component, class Storage>
Matrix4< Component, Storage > Matrix4< Component, Storage >::Perspective(Component d) noexcept
{
	return Matrix4< Component, Storage >::FromRows(
		Matrix4< Component, Storage >::VectorType(1, 0, 0, 0),
		Matrix4< Component, Storage >::VectorType(0, 1, 0, 0),
		Matrix4< Component, Storage >::VectorType(0, 0, 1, 0),
		Matrix4< Component, Storage >::VectorType(0, 0, -static_cast<Component>(1)/d, 1)
	);
}

// Return a translation matrix.
template< typename Component, class Storage>
Matrix4< Component, Storage > Matrix4< Component, Storage >::Translate(const Vector3< Component >& v) noexcept
{
	return Translate(v.x, v.y, v.z);
}

template< typename Component, class Storage>
Matrix4< Component, Storage > Matrix4< Component, Storage >::Translate(Component x, Component y, Component z) noexcept
{
	return Matrix4< Component, Storage >::FromRows(
		Matrix4< Component, Storage >::VectorType(1, 0, 0, x),
		Matrix4< Component, Storage >::VectorType(0, 1, 0, y),
		Matrix4< Component, Storage >::VectorType(0, 0, 1, z),
		Matrix4< Component, Storage >::VectorType(0, 0, 0, 1)
	);
}

template< typename Component, class Storage>
Matrix4< Component, Storage > Matrix4< Component, Storage >::TranslateX(Component x) noexcept
{
	return Translate(Vector3< Component >(x, 0, 0));
}

template< typename Component, class Storage>
Matrix4< Component, Storage > Matrix4< Component, Storage >::TranslateY(Component y) noexcept
{
	return Translate(Vector3< Component >(0, y, 0));
}

template< typename Component, class Storage>
Matrix4< Component, Storage > Matrix4< Component, Storage >::TranslateZ(Component z) noexcept
{
	return Translate(Vector3< Component >(0, 0, z));
}

// Return a scaling matrix.
template< typename Component, class Storage>
Matrix4< Component, Storage > Matrix4< Component, Storage >::Scale(Component x, Component y, Component z) noexcept
{
	return Matrix4::Diag(x, y, z, 1);
}

template< typename Component, class Storage>
Matrix4< Component, Storage > Matrix4< Component, Storage >::ScaleX(Component x) noexcept
{
	return Scale(x, 1, 1);
}

template< typename Component, class Storage>
Matrix4< Component, Storage > Matrix4< Component, Storage >::ScaleY(Component y) noexcept
{
	return Scale(1, y, 1);
}

template< typename Component, class Storage>
Matrix4< Component, Storage > Matrix4< Component, Storage >::ScaleZ(Component z) noexcept
{
	return Scale(1, 1, z);
}

// Return a rotation matrix.
template< typename Component, class Storage>
Matrix4< Component, Storage > Matrix4< Component, Storage >::Rotate(const Vector3< Component >& v, Component angle) noexcept
{
	Vector3< Component > n = v.Normalise();
	Component Sin = Math::Sin(angle);
	Component Cos = Math::Cos(angle);
	return Matrix4< Component, Storage >::FromRows(
		Matrix4< Component, Storage >::VectorType(
			n.x * n.x * (1 - Cos) +       Cos,
			n.x * n.y * (1 - Cos) - n.z * Sin,
			n.x * n.z * (1 - Cos) + n.y * Sin,
			0
		),
		Matrix4< Component, Storage >::VectorType(
			n.y * n.x * (1 - Cos) + n.z * Sin,
			n.y * n.y * (1 - Cos) +       Cos,
			n.y * n.z * (1 - Cos) - n.x * Sin,
			0
		),
		Matrix4< Component, Storage >::VectorType(
			n.z * n.x * (1 - Cos) - n.y * Sin,
			n.z * n.y * (1 - Cos) + n.x * Sin,
			n.z * n.z * (1 - Cos) +       Cos,
			0
		),
		Matrix4< Component, Storage >::VectorType(0, 0, 0, 1)
	);
}

template< typename Component, class Storage>
Matrix4< Component, Storage > Matrix4< Component, Storage >::RotateX(Component angle) noexcept
{
	Component Sin = Math::Sin(angle);
	Component Cos = Math::Cos(angle);
	return Matrix4< Component, Storage >::FromRows(
		Matrix4< Component, Storage >::VectorType(1, 0,    0,   0),
		Matrix4< Component, Storage >::VectorType(0, Cos, -Sin, 0),
		Matrix4< Component, Storage >::VectorType(0, Sin,  Cos, 0),
		Matrix4< Component, Storage >::VectorType(0, 0,    0,   1)
	);
}

template< typename Component, class Storage>
Matrix4< Component, Storage > Matrix4< Component, Storage >::RotateY(Component angle) noexcept
{
	Component Sin = Math::Sin(angle);
	Component Cos = Math::Cos(angle);
	return Matrix4< Component, Storage >::FromRows(
		Matrix4< Component, Storage >::VectorType( Cos, 0, Sin, 0),
		Matrix4< Component, Storage >::VectorType( 0,   1, 0,   0),
		Matrix4< Component, Storage >::VectorType(-Sin, 0, Cos, 0),
		Matrix4< Component, Storage >::VectorType( 0,   0, 0,   1)
	);
}

template< typename Component, class Storage>
Matrix4< Component, Storage > Matrix4< Component, Storage >::RotateZ(Component angle) noexcept
{
	Component Sin = Math::Sin(angle);
	Component Cos = Math::Cos(angle);
	return Matrix4< Component, Storage >::FromRows(
		Matrix4< Component, Storage >::VectorType(Cos, -Sin, 0, 0),
		Matrix4< Component, Storage >::VectorType(Sin,  Cos, 0, 0),
		Matrix4< Component, Storage >::VectorType( 0,   0,   1, 0),
		Matrix4< Component, Storage >::VectorType( 0,   0,   0, 1)
	);
}
// Return a skew/shearing matrix.
// @return A skew matrix.
template< typename Component, class Storage>
Matrix4< Component, Storage > Matrix4< Component, Storage >::Skew(Component angle_x, Component angle_y) noexcept
{
	Component SkewX = Math::Tan(angle_x);
	Component SkewY = Math::Tan(angle_y);
	return Matrix4< Component, Storage >::FromRows(
		Matrix4< Component, Storage >::VectorType(1,     SkewX, 0, 0),
		Matrix4< Component, Storage >::VectorType(SkewY, 1,     0, 0),
		Matrix4< Component, Storage >::VectorType( 0,    0,     1, 0),
		Matrix4< Component, Storage >::VectorType( 0,    0,     0, 1)
	);
}

template< typename Component, class Storage>
Matrix4< Component, Storage > Matrix4< Component, Storage >::SkewX(Component angle) noexcept
{
	return Skew(angle, 0);
}

template< typename Component, class Storage>
Matrix4< Component, Storage > Matrix4< Component, Storage >::SkewY(Component angle) noexcept
{
	return Skew(0, angle);
}

template<typename Component, class Storage>
Matrix4< Component, Storage > Rml::Core::Matrix4<Component, Storage>::Compose(const Vector3<Component>& translation,
	const Vector3<Component>& scale, const Vector3<Component>& skew, const Vector4<Component>& perspective,
	const Vector4<Component>& quaternion) noexcept
{
	ThisType matrix = ThisType::Identity();

	for (int i = 0; i < 4; i++)
		matrix[i][3] = perspective[i];

	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 3; j++)
			matrix[3][i] += translation[j] * matrix[j][i];
	
	float x = quaternion.x;
	float y = quaternion.y;
	float z = quaternion.z;
	float w = quaternion.w;

	ThisType rotation = Matrix4< Component, Storage >::FromRows(
		VectorType(1.f - 2.f * (y*y + z * z), 2.f*(x*y - z * w), 2.f*(x*z + y * w), 0.f),
		VectorType(2.f * (x * y + z * w), 1.f - 2.f * (x * x + z * z), 2.f * (y * z - x * w), 0.f),
		VectorType(2.f * (x * z - y * w), 2.f * (y * z + x * w), 1.f - 2.f * (x * x + y * y), 0.f),
		VectorType(0, 0, 0, 1)
	);

	matrix *= rotation;

	ThisType temp = ThisType::Identity();
	if(skew[2])
	{
		temp[2][1] = skew[2];
		matrix *= temp;
	}
	if (skew[1])
	{
		temp[2][1] = 0;
		temp[2][0] = skew[1];
		matrix *= temp;
	}
	if (skew[0])
	{
		temp[2][0] = 0;
		temp[1][0] = skew[0];
		matrix *= temp;
	}

	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 4; j++)
			matrix[i][j] *= scale[i];

	return matrix;
}

template< typename Component, class Storage >
template< typename _Component >
struct Matrix4< Component, Storage >::VectorMultiplier< _Component, RowMajorStorage< _Component > >
{
	typedef _Component ComponentType;
	typedef RowMajorStorage< ComponentType > StorageAType;
	typedef Matrix4< ComponentType, StorageAType > MatrixAType;
	typedef Vector4< ComponentType > VectorType;

	static const VectorType Multiply(const MatrixAType& lhs, const VectorType& rhs) noexcept
	{
		typename MatrixAType::ConstRows rows(lhs.vectors);
		return VectorType(
			rhs.DotProduct(rows[0]),
			rhs.DotProduct(rows[1]),
			rhs.DotProduct(rows[2]),
			rhs.DotProduct(rows[3])
		);
	}
};

template< typename Component, class Storage >
template< typename _Component >
struct Matrix4< Component, Storage >::VectorMultiplier< _Component, ColumnMajorStorage< _Component > >
{
	typedef _Component ComponentType;
	typedef ColumnMajorStorage< ComponentType > StorageAType;
	typedef Matrix4< ComponentType, StorageAType > MatrixAType;
	typedef Vector4< ComponentType > VectorType;

	static const VectorType Multiply(const MatrixAType& lhs, const VectorType& rhs) noexcept
	{
		typename MatrixAType::ConstRows rows(lhs.vectors);
		return VectorType(
			rhs.DotProduct(rows[0]),
			rhs.DotProduct(rows[1]),
			rhs.DotProduct(rows[2]),
			rhs.DotProduct(rows[3])
		);
	}
};

template< typename Component, class Storage >
template< typename _Component, class _StorageB >
struct Matrix4< Component, Storage >::MatrixMultiplier< _Component, RowMajorStorage< _Component >, _StorageB >
{
	typedef _Component ComponentType;
	typedef RowMajorStorage< ComponentType > StorageAType;
	typedef _StorageB StorageBType;
	typedef Matrix4< ComponentType, StorageAType > MatrixAType;
	typedef Matrix4< ComponentType, StorageBType > MatrixBType;

	static const MatrixAType Multiply(const MatrixAType& lhs, const MatrixBType& rhs) noexcept
	{
		typename MatrixAType::ThisType result;
		typename MatrixAType::Rows result_rows(result.vectors);
		typename MatrixAType::ConstRows lhs_rows(lhs.vectors);
		typename MatrixBType::ConstColumns rhs_columns(rhs.vectors);
		for (int i = 0; i < 4; ++i)
		{
			for (int j = 0; j < 4; ++j)
			{
				result_rows[i][j] = lhs_rows[i].DotProduct(rhs_columns[j]);
			}
		}
		return result;
	}
};

template< typename Component, class Storage >
template< typename _Component >
struct Matrix4< Component, Storage >::MatrixMultiplier< _Component, ColumnMajorStorage< _Component >, ColumnMajorStorage< _Component > >
{
	typedef _Component ComponentType;
	typedef ColumnMajorStorage< ComponentType > StorageAType;
	typedef ColumnMajorStorage< ComponentType > StorageBType;
	typedef Matrix4< ComponentType, StorageAType > MatrixAType;
	typedef Matrix4< ComponentType, StorageBType > MatrixBType;

	static const MatrixAType Multiply(const MatrixAType& lhs, const MatrixBType& rhs) noexcept
	{
		typename MatrixAType::ThisType result;
		typename MatrixAType::Rows result_rows(result.vectors);
		typename MatrixAType::ConstRows lhs_rows(lhs.vectors);
		typename MatrixBType::ConstColumns rhs_columns(rhs.vectors);
		for (int i = 0; i < 4; ++i)
		{
			for (int j = 0; j < 4; ++j)
			{
				result_rows[i][j] = rhs_columns[j].DotProduct(lhs_rows[i]);
			}
		}
		return result;
	}
};

template< typename Component, class Storage >
template< typename _Component >
struct Matrix4< Component, Storage >::MatrixMultiplier< _Component, ColumnMajorStorage< _Component >, RowMajorStorage< _Component > >
{
	typedef _Component ComponentType;
	typedef ColumnMajorStorage< ComponentType > StorageAType;
	typedef RowMajorStorage< ComponentType > StorageBType;
	typedef Matrix4< ComponentType, StorageAType > MatrixAType;
	typedef Matrix4< ComponentType, StorageBType > MatrixBType;

	static const MatrixAType Multiply(const MatrixAType& lhs, const MatrixBType& rhs) noexcept
	{
		return lhs * MatrixAType(rhs);
	}
};

}
}