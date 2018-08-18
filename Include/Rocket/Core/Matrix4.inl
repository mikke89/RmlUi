/*
 * This source file is part of rocket, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://www.librocket.com
 *
 * Copyright (c) 2014 Markus Schöngart
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

namespace Rocket {
namespace Core {

// Initialising constructor.
template< typename Component, class Storage >
Matrix4< Component, Storage >::Matrix4(
	const typename Matrix4< Component, Storage >::VectorType& vec0,
	const typename Matrix4< Component, Storage >::VectorType& vec1,
	const typename Matrix4< Component, Storage >::VectorType& vec2,
	const typename Matrix4< Component, Storage >::VectorType& vec3
) throw()
{
	vectors[0] = vec0;
	vectors[1] = vec1;
	vectors[2] = vec2;
	vectors[3] = vec3;
}

// Default constructor.
template< typename Component, class Storage >
Matrix4< Component, Storage >::Matrix4() throw()
{
}

// Initialising, copy constructor.
template< typename Component, class Storage >
Matrix4< Component, Storage >::Matrix4(const typename Matrix4< Component, Storage >::ThisType& other) throw()
{
	for (int i = 0; i < 4; ++i)
	{
		vectors[i] = other.vectors[i];
	}
}

template< typename Component, class Storage >
Matrix4< Component, Storage >::Matrix4(const typename Matrix4< Component, Storage >::TransposeType& other) throw()
{
	Rows rows(vectors);
	typename Matrix4< Component, Storage >::TransposeType::ConstRows other_rows(other.vectors);
	for (int i = 0; i < 4; ++i)
	{
		rows[i] = other_rows[i];
	}
	return *this;
}

// Assignment operator
template< typename Component, class Storage >
const typename Matrix4< Component, Storage >::ThisType& Matrix4< Component, Storage >::operator=(const typename Matrix4< Component, Storage >::ThisType& other) throw()
{
	for (int i = 0; i < 4; ++i)
	{
		vectors[i] = other.vectors[i];
	}
	return *this;
}

template< typename Component, class Storage >
const typename Matrix4< Component, Storage >::ThisType& Matrix4< Component, Storage >::operator=(const typename Matrix4< Component, Storage >::TransposeType& other) throw()
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
) throw()
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
) throw()
{
	typename Matrix4< Component, Storage >::ThisType result;
	result.SetColumns(vec0, vec1, vec2, vec3);
	return result;
}

// Construct from components
template< typename Component, class Storage >
const typename Matrix4< Component, Storage >::ThisType Matrix4< Component, Storage >::FromRowMajor(const Component* components) throw()
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
const typename Matrix4< Component, Storage >::ThisType Matrix4< Component, Storage >::FromColumnMajor(const Component* components) throw()
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
void Matrix4< Component, Storage >::SetRows(const VectorType& vec0, const VectorType& vec1, const VectorType& vec2, const VectorType& vec3) throw()
{
	Rows rows(vectors);
	rows[0] = vec0;
	rows[1] = vec1;
	rows[2] = vec2;
	rows[3] = vec3;
}

// Set all columns
template< typename Component, class Storage >
void Matrix4< Component, Storage >::SetColumns(const VectorType& vec0, const VectorType& vec1, const VectorType& vec2, const VectorType& vec3) throw()
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
bool Matrix4< Component, Storage >::Invert() throw()
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

// Returns the negation of this matrix.
template< typename Component, class Storage >
typename Matrix4< Component, Storage >::ThisType Matrix4< Component, Storage >::operator-() const throw()
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
const typename Matrix4< Component, Storage >::ThisType& Matrix4< Component, Storage >::operator+=(const typename Matrix4< Component, Storage >::ThisType& other) throw()
{
	for (int i = 0; i < 4; ++i)
	{
		vectors[i] += other.vectors[i];
	}
	return *this;
}
template< typename Component, class Storage>
const typename Matrix4< Component, Storage >::ThisType& Matrix4< Component, Storage >::operator+=(const typename Matrix4< Component, Storage >::TransposeType& other) throw()
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
const typename Matrix4< Component, Storage >::ThisType& Matrix4< Component, Storage >::operator-=(const typename Matrix4< Component, Storage >::ThisType& other) throw()
{
	for (int i = 0; i < 4; ++i)
	{
		vectors[i] -= other.vectors[i];
	}
	return *this;
}
template< typename Component, class Storage>
const typename Matrix4< Component, Storage >::ThisType& Matrix4< Component, Storage >::operator-=(const typename Matrix4< Component, Storage >::TransposeType& other) throw()
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
const typename Matrix4< Component, Storage >::ThisType& Matrix4< Component, Storage >::operator*=(Component s) throw()
{
	for (int i = 0; i < 4; ++i)
	{
		vectors[i] *= s;
	}
	return *this;
}

// Scales this matrix in-place by the inverse of a value.
template< typename Component, class Storage>
const typename Matrix4< Component, Storage >::ThisType& Matrix4< Component, Storage >::operator/=(Component s) throw()
{
	for (int i = 0; i < 4; ++i)
	{
		vectors[i] /= s;
	}
	return *this;
}

// Equality operator.
template< typename Component, class Storage>
bool Matrix4< Component, Storage >::operator==(const typename Matrix4< Component, Storage >::ThisType& other) const throw()
{
	typename Matrix4< Component, Storage >::ConstRows rows(vectors);
	typename Matrix4< Component, Storage >::ConstRows other_rows(other.vectors);
	return vectors[0] == other.vectors[0]
	   && vectors[1] == other.vectors[1]
	   && vectors[2] == other.vectors[2]
	   && vectors[3] == other.vectors[3];
}
template< typename Component, class Storage>
bool Matrix4< Component, Storage >::operator==(const typename Matrix4< Component, Storage >::TransposeType& other) const throw()
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
bool Matrix4< Component, Storage >::operator!=(const typename Matrix4< Component, Storage >::ThisType& other) const throw()
{
	return vectors[0] != other.vectors[0]
	    || vectors[1] != other.vectors[1]
	    || vectors[2] != other.vectors[2]
	    || vectors[3] != other.vectors[3];
}
template< typename Component, class Storage>
bool Matrix4< Component, Storage >::operator!=(const typename Matrix4< Component, Storage >::TransposeType& other) const throw()
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
const Matrix4< Component, Storage >& Matrix4< Component, Storage >::Identity() throw()
{
	static Matrix4< Component, Storage > identity(Diag(1, 1, 1, 1));
	return identity;
}

// Return a diagonal matrix.
template< typename Component, class Storage>
Matrix4< Component, Storage > Matrix4< Component, Storage >::Diag(Component a, Component b, Component c, Component d) throw()
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
Matrix4< Component, Storage > Matrix4< Component, Storage >::ProjectOrtho(Component l, Component r, Component b, Component t, Component n, Component f) throw()
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
Matrix4< Component, Storage > Matrix4< Component, Storage >::ProjectPerspective(Component l, Component r, Component b, Component t, Component n, Component f) throw()
{
	return Matrix4< Component, Storage >::FromRows(
		Matrix4< Component, Storage >::VectorType(2 * n / (r - l), 0, (r + l)/(r - l), 0),
		Matrix4< Component, Storage >::VectorType(0, 2 * n / (t - b), (t + b)/(t - b), 0),
		Matrix4< Component, Storage >::VectorType(0, 0, -(f + n)/(f - n), -(2 * f * n)/(f - n)),
		Matrix4< Component, Storage >::VectorType(0, 0, -1, 0)
	);
}

// Return a translation matrix.
template< typename Component, class Storage>
Matrix4< Component, Storage > Matrix4< Component, Storage >::Translate(const Vector3< Component >& v) throw()
{
	return Translate(v.x, v.y, v.z);
}

template< typename Component, class Storage>
Matrix4< Component, Storage > Matrix4< Component, Storage >::Translate(Component x, Component y, Component z) throw()
{
	return Matrix4< Component, Storage >::FromRows(
		Matrix4< Component, Storage >::VectorType(1, 0, 0, x),
		Matrix4< Component, Storage >::VectorType(0, 1, 0, y),
		Matrix4< Component, Storage >::VectorType(0, 0, 1, z),
		Matrix4< Component, Storage >::VectorType(0, 0, 0, 1)
	);
}

template< typename Component, class Storage>
Matrix4< Component, Storage > Matrix4< Component, Storage >::TranslateX(Component x) throw()
{
	return Translate(Vector3< Component >(x, 0, 0));
}

template< typename Component, class Storage>
Matrix4< Component, Storage > Matrix4< Component, Storage >::TranslateY(Component y) throw()
{
	return Translate(Vector3< Component >(0, y, 0));
}

template< typename Component, class Storage>
Matrix4< Component, Storage > Matrix4< Component, Storage >::TranslateZ(Component z) throw()
{
	return Translate(Vector3< Component >(0, 0, z));
}

// Return a scaling matrix.
template< typename Component, class Storage>
Matrix4< Component, Storage > Matrix4< Component, Storage >::Scale(Component x, Component y, Component z) throw()
{
	return Matrix4::Diag(x, y, z, 1);
}

template< typename Component, class Storage>
Matrix4< Component, Storage > Matrix4< Component, Storage >::ScaleX(Component x) throw()
{
	return Scale(x, 1, 1);
}

template< typename Component, class Storage>
Matrix4< Component, Storage > Matrix4< Component, Storage >::ScaleY(Component y) throw()
{
	return Scale(1, y, 1);
}

template< typename Component, class Storage>
Matrix4< Component, Storage > Matrix4< Component, Storage >::ScaleZ(Component z) throw()
{
	return Scale(1, 1, z);
}

// Return a rotation matrix.
template< typename Component, class Storage>
Matrix4< Component, Storage > Matrix4< Component, Storage >::Rotate(const Vector3< Component >& v, Component angle) throw()
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
Matrix4< Component, Storage > Matrix4< Component, Storage >::RotateX(Component angle) throw()
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
Matrix4< Component, Storage > Matrix4< Component, Storage >::RotateY(Component angle) throw()
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
Matrix4< Component, Storage > Matrix4< Component, Storage >::RotateZ(Component angle) throw()
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
Matrix4< Component, Storage > Matrix4< Component, Storage >::Skew(Component angle_x, Component angle_y) throw()
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
Matrix4< Component, Storage > Matrix4< Component, Storage >::SkewX(Component angle) throw()
{
	return Skew(angle, 0);
}

template< typename Component, class Storage>
Matrix4< Component, Storage > Matrix4< Component, Storage >::SkewY(Component angle) throw()
{
	return Skew(0, angle);
}

template< typename Component, class Storage >
template< typename _Component >
struct Matrix4< Component, Storage >::VectorMultiplier< _Component, RowMajorStorage< _Component > >
{
	typedef _Component ComponentType;
	typedef RowMajorStorage< ComponentType > StorageAType;
	typedef Matrix4< ComponentType, StorageAType > MatrixAType;
	typedef Vector4< ComponentType > VectorType;

	static const VectorType Multiply(const MatrixAType& lhs, const VectorType& rhs) throw()
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

	static const VectorType Multiply(const MatrixAType& lhs, const VectorType& rhs) throw()
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

	static const MatrixAType Multiply(const MatrixAType& lhs, const MatrixBType& rhs) throw()
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

	static const MatrixAType Multiply(const MatrixAType& lhs, const MatrixBType& rhs) throw()
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

	static const MatrixAType Multiply(const MatrixAType& lhs, const MatrixBType& rhs) throw()
	{
		return lhs * MatrixAType(rhs);
	}
};

}
}