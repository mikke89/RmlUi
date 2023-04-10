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

#ifndef RMLUI_CORE_MATRIX4_H
#define RMLUI_CORE_MATRIX4_H

#include "Debug.h"
#include "Math.h"
#include "Vector4.h"

namespace Rml {

/**
    Templated class that acts as base strategy for vectors access patterns of matrices.
    @author Markus Schöngart
 */
template <typename Component>
struct MatrixStorageBase {
	typedef Component ComponentType;
	typedef Vector4<ComponentType> VectorType;
	typedef VectorType VectorsType[4];

	class StrideVector {
		VectorsType& vectors;
		int idx;

	public:
		inline StrideVector(VectorsType& vectors, int idx) noexcept : vectors(vectors), idx(idx) {}
		inline ComponentType& operator[](int i) noexcept { return vectors[i][idx]; }
		inline StrideVector& operator=(const VectorType& vec) noexcept
		{
			(*this)[0] = vec[0];
			(*this)[1] = vec[1];
			(*this)[2] = vec[2];
			(*this)[3] = vec[3];
			return *this;
		}
		operator const VectorType() const noexcept { return VectorType((*this)[0], (*this)[1], (*this)[2], (*this)[3]); }
	};
	class StrideAccess {
		VectorsType& vectors;

	public:
		inline StrideAccess(VectorsType& vectors) noexcept : vectors(vectors) {}
		inline StrideVector operator[](int i) noexcept { return StrideVector(vectors, i); }
	};
	class ConstStrideVector {
		const VectorsType& vectors;
		int idx;

	public:
		inline ConstStrideVector(const VectorsType& vectors, int idx) noexcept : vectors(vectors), idx(idx) {}
		inline const ComponentType& operator[](int i) const noexcept { return vectors[i][idx]; }
		inline operator const VectorType() const noexcept { return VectorType((*this)[0], (*this)[1], (*this)[2], (*this)[3]); }
	};
	class ConstStrideAccess {
		const VectorsType& vectors;

	public:
		inline ConstStrideAccess(const VectorsType& vectors) noexcept : vectors(vectors) {}
		inline ConstStrideVector operator[](int i) noexcept { return ConstStrideVector(vectors, i); }
	};

	class PackedVector {
		VectorType& vector;

	public:
		inline PackedVector(VectorType& vector) noexcept : vector(vector) {}
		inline ComponentType& operator[](int i) noexcept { return vector[i]; }
		inline PackedVector& operator=(const VectorType& vec) noexcept
		{
			vector = vec;
			return *this;
		}
		inline PackedVector& operator=(StrideVector& vec) noexcept
		{
			vector[0] = vec[0];
			vector[1] = vec[1];
			vector[2] = vec[2];
			vector[3] = vec[3];
			return *this;
		}
		inline PackedVector& operator=(ConstStrideVector& vec) noexcept
		{
			vector[0] = vec[0];
			vector[1] = vec[1];
			vector[2] = vec[2];
			vector[3] = vec[3];
			return *this;
		}
		inline operator VectorType&() noexcept { return vector; }
	};
	class PackedAccess {
		VectorsType& vectors;

	public:
		inline PackedAccess(VectorsType& vectors) noexcept : vectors(vectors) {}
		inline PackedVector operator[](int i) noexcept { return PackedVector(vectors[i]); }
	};

	class ConstPackedAccess {
		const VectorsType& vectors;

	public:
		inline ConstPackedAccess(const VectorsType& vectors) noexcept : vectors(vectors) {}
		inline const VectorType& operator[](int i) noexcept { return vectors[i]; }
	};
};

template <typename Component>
struct RowMajorStorage;
template <typename Component>
struct ColumnMajorStorage;

/**
    Templated class that defines the vectors access pattern for row-major matrices.
    @author Markus Schöngart
 */
template <typename Component>
struct RowMajorStorage : public MatrixStorageBase<Component> {
	typedef Component ComponentType;
	typedef Vector4<ComponentType> VectorType;
	typedef RowMajorStorage<ComponentType> ThisType;
	typedef ColumnMajorStorage<ComponentType> TransposeType;

	typedef typename MatrixStorageBase<Component>::PackedVector Row;
	typedef typename MatrixStorageBase<Component>::PackedAccess Rows;
	typedef const typename MatrixStorageBase<Component>::VectorType& ConstRow;
	typedef typename MatrixStorageBase<Component>::ConstPackedAccess ConstRows;
	typedef typename MatrixStorageBase<Component>::StrideVector Column;
	typedef typename MatrixStorageBase<Component>::StrideAccess Columns;
	typedef typename MatrixStorageBase<Component>::ConstStrideVector ConstColumn;
	typedef typename MatrixStorageBase<Component>::ConstStrideAccess ConstColumns;
};

/**
    Templated class that defines the vectors access pattern for column-major matrices.
    @author Markus Schöngart
 */
template <typename Component>
struct ColumnMajorStorage {
	typedef Component ComponentType;
	typedef Vector4<ComponentType> VectorType;
	typedef ColumnMajorStorage<ComponentType> ThisType;
	typedef RowMajorStorage<ComponentType> TransposeType;

	typedef typename MatrixStorageBase<Component>::PackedVector Column;
	typedef typename MatrixStorageBase<Component>::PackedAccess Columns;
	typedef const typename MatrixStorageBase<Component>::VectorType& ConstColumn;
	typedef typename MatrixStorageBase<Component>::ConstPackedAccess ConstColumns;
	typedef typename MatrixStorageBase<Component>::StrideVector Row;
	typedef typename MatrixStorageBase<Component>::StrideAccess Rows;
	typedef typename MatrixStorageBase<Component>::ConstStrideVector ConstRow;
	typedef typename MatrixStorageBase<Component>::ConstStrideAccess ConstRows;
};

/**
    Templated class for a generic 4x4 matrix.
    @author Markus Schöngart
 */

template <typename Component, class Storage = ColumnMajorStorage<Component>>
class Matrix4 {
public:
	typedef Component ComponentType;
	typedef Vector4<ComponentType> VectorType;
	typedef Matrix4<ComponentType, Storage> ThisType;

	typedef Storage StorageType;
	typedef typename StorageType::Row Row;
	typedef typename StorageType::Rows Rows;
	typedef typename StorageType::ConstRow ConstRow;
	typedef typename StorageType::ConstRows ConstRows;
	typedef typename StorageType::Column Column;
	typedef typename StorageType::Columns Columns;
	typedef typename StorageType::ConstColumn ConstColumn;
	typedef typename StorageType::ConstColumns ConstColumns;

	typedef typename StorageType::TransposeType TransposeStorageType;
	typedef Matrix4<ComponentType, TransposeStorageType> TransposeType;
	friend class Rml::Matrix4<ComponentType, TransposeStorageType>;

private:
	// The components of the matrix.
	VectorType vectors[4];

	/// Initialising constructor.
	Matrix4(const VectorType& vec0, const VectorType& vec1, const VectorType& vec2, const VectorType& vec3) noexcept;

	template <typename _Component, class _StorageA>
	struct VectorMultiplier {
		typedef _Component ComponentType;
		typedef _StorageA StorageAType;
		typedef Matrix4<ComponentType, StorageAType> MatrixAType;
		typedef Vector4<ComponentType> VectorType;

		static const VectorType Multiply(const MatrixAType& lhs, const VectorType& rhs) noexcept;
	};

	template <typename _Component, class _StorageA, class _StorageB>
	struct MatrixMultiplier {
		typedef _Component ComponentType;
		typedef _StorageA StorageAType;
		typedef _StorageB StorageBType;
		typedef Matrix4<ComponentType, StorageAType> MatrixAType;
		typedef Matrix4<ComponentType, StorageBType> MatrixBType;

		static const VectorType Multiply(const MatrixAType& lhs, const VectorType& rhs);

		static const MatrixAType Multiply(const MatrixAType& lhs, const MatrixBType& rhs) noexcept;
	};

public:
	/// Zero-initialising default constructor.
	inline Matrix4() noexcept;

	/// Copy constructor.
	inline Matrix4(const ThisType& other) noexcept;
	Matrix4(const TransposeType& other) noexcept;

	/// Assignment operator
	const ThisType& operator=(const ThisType& other) noexcept;
	const ThisType& operator=(const TransposeType& other) noexcept;

	/// Construct from row vectors.
	static const ThisType FromRows(const VectorType& vec0, const VectorType& vec1, const VectorType& vec2, const VectorType& vec3) noexcept;

	/// Construct from column vectors.
	static const ThisType FromColumns(const VectorType& vec0, const VectorType& vec1, const VectorType& vec2, const VectorType& vec3) noexcept;

	/// Construct from components.
	static const ThisType FromRowMajor(const ComponentType* components) noexcept;
	static const ThisType FromColumnMajor(const ComponentType* components) noexcept;

	// Convert to raw values; keep the storage mode in mind.
	inline Component* data() noexcept { return &vectors[0][0]; }
	inline const Component* data() const noexcept { return &vectors[0][0]; }

	/// Get the i-th row
	inline Row GetRow(int i) noexcept
	{
		Rows rows(vectors);
		return rows[i];
	}
	/// Get the i-th row
	inline ConstRow GetRow(int i) const noexcept
	{
		ConstRows rows(vectors);
		return rows[i];
	}
	/// Set the i-th row
	inline void SetRow(int i, const VectorType& vec) noexcept
	{
		Rows rows(vectors);
		rows[i] = vec;
	}
	/// Set all rows
	void SetRows(const VectorType& vec0, const VectorType& vec1, const VectorType& vec2, const VectorType& vec3) noexcept;

	/// Get the i-th column
	inline Column GetColumn(int i) noexcept
	{
		Columns columns(vectors);
		return columns[i];
	}
	/// Get the i-th column
	inline ConstColumn GetColumn(int i) const noexcept
	{
		ConstColumns columns(vectors);
		return columns[i];
	}
	/// Set the i-th column
	inline void SetColumn(int i, const VectorType& vec) noexcept
	{
		Columns columns(vectors);
		columns[i] = vec;
	}
	/// Set all columns
	void SetColumns(const VectorType& vec0, const VectorType& vec1, const VectorType& vec2, const VectorType& vec3) noexcept;

	/// Returns the transpose of this matrix.
	/// @return The transpose matrix.
	inline const TransposeType& Transpose() const noexcept { return reinterpret_cast<const TransposeType&>(*this); }

	/// Inverts this matrix in place, if possible.
	/// @return true, if the inversion succeeded.
	bool Invert() noexcept;

	/// Inverts this matrix in place, if possible.
	/// @return true, if the inversion succeeded.
	float Determinant() const noexcept;

	/// Returns the negation of this matrix.
	/// @return The negation of this matrix.
	ThisType operator-() const noexcept;

	/// Adds another matrix to this in-place.
	/// @param[in] other The matrix to add.
	/// @return This matrix, post-operation.
	const ThisType& operator+=(const ThisType& other) noexcept;
	const ThisType& operator+=(const TransposeType& other) noexcept;
	/// Subtracts another matrix from this in-place.
	/// @param[in] other The matrix to subtract.
	/// @return This matrix, post-operation.
	const ThisType& operator-=(const ThisType& other) noexcept;
	const ThisType& operator-=(const TransposeType& other) noexcept;
	/// Scales this matrix in-place.
	/// @param[in] other The value to scale this matrix's components by.
	/// @return This matrix, post-operation.
	const ThisType& operator*=(Component other) noexcept;
	/// Scales this matrix in-place by the inverse of a value.
	/// @param[in] other The value to divide this matrix's components by.
	/// @return This matrix, post-operation.
	const ThisType& operator/=(Component other) noexcept;

	inline const VectorType& operator[](size_t i) const noexcept { return vectors[i]; }
	inline VectorType& operator[](size_t i) noexcept { return vectors[i]; }

	/// Returns the sum of this matrix and another.
	/// @param[in] other The matrix to add this to.
	/// @return The sum of the two matrices.
	inline const ThisType operator+(const ThisType& other) const noexcept
	{
		ThisType result(*this);
		result += other;
		return result;
	}
	inline const ThisType operator+(const TransposeType& other) const noexcept
	{
		ThisType result(*this);
		result += other;
		return result;
	}
	/// Returns the result of subtracting another matrix from this matrix.
	/// @param[in] other The matrix to subtract from this matrix.
	/// @return The result of the subtraction.
	inline const ThisType operator-(const ThisType& other) const noexcept
	{
		ThisType result(*this);
		result -= other;
		return result;
	}
	inline const ThisType operator-(const TransposeType& other) const noexcept
	{
		ThisType result(*this);
		result -= other;
		return result;
	}
	/// Returns the result of multiplying this matrix by a scalar.
	/// @param[in] other The scalar value to multiply by.
	/// @return The result of the scale.
	inline const ThisType operator*(Component other) const noexcept
	{
		ThisType result(*this);
		result *= other;
		return result;
	}
	/// Returns the result of dividing this matrix by a scalar.
	/// @param[in] other The scalar value to divide by.
	/// @return The result of the scale.
	inline const ThisType operator/(Component other) const noexcept
	{
		ThisType result(*this);
		result *= other;
		return result;
	}

	/// Returns the result of multiplying this matrix by a vector.
	/// @param[in] other The scalar value to multiply by.
	/// @return The result of the scale.
	const VectorType operator*(const VectorType& other) const noexcept { return VectorMultiplier<Component, Storage>::Multiply(*this, other); }

	/// Returns the result of multiplying this matrix by another matrix.
	/// @param[in] other The matrix value to multiply by.
	/// @return The result of the multiplication.
	template <class Storage2>
	const ThisType operator*(const Matrix4<Component, Storage2>& other) const noexcept
	{
		return MatrixMultiplier<Component, Storage, Storage2>::Multiply(*this, other);
	}

	/// Multiplies this matrix by another matrix in place.
	/// @return The result of the multiplication.
	inline const ThisType& operator*=(const ThisType& other) noexcept
	{
		*this = *this * other;
		return *this;
	}
	inline const ThisType& operator*=(const TransposeType& other) noexcept
	{
		*this = *this * other;
		return *this;
	}

	/// Equality operator.
	/// @param[in] other The matrix to compare this against.
	/// @return True if the two matrices are equal, false otherwise.
	bool operator==(const ThisType& other) const noexcept;
	bool operator==(const TransposeType& other) const noexcept;
	/// Inequality operator.
	/// @param[in] other The matrix to compare this against.
	/// @return True if the two matrices are not equal, false otherwise.
	bool operator!=(const ThisType& other) const noexcept;
	bool operator!=(const TransposeType& other) const noexcept;

	/// Return the identity matrix.
	/// @return The identity matrix.
	inline static const ThisType& Identity() noexcept;
	/// Return a diagonal matrix.
	/// @return A diagonal matrix.
	static ThisType Diag(Component a, Component b, Component c, Component d = 1) noexcept;

	/// Create an orthographic projection matrix
	/// @param l The horizontal coordinate of the left clipping plane
	/// @param r The horizontal coordinate of the right clipping plane
	/// @param b The vertical coordinate of the bottom clipping plane
	/// @param t The vertical coordinate of the top clipping plane
	/// @param n The depth coordinate of the near clipping plane
	/// @param f The depth coordinate of the far clipping plane
	/// @return The specified orthographic projection matrix.
	static ThisType ProjectOrtho(Component l, Component r, Component b, Component t, Component n, Component f) noexcept;
	/// Create a perspective projection matrix
	/// @param l The horizontal coordinate of the left clipping plane
	/// @param r The horizontal coordinate of the right clipping plane
	/// @param b The vertical coordinate of the bottom clipping plane
	/// @param t The vertical coordinate of the top clipping plane
	/// @param n The depth coordinate of the near clipping plane
	/// @param f The depth coordinate of the far clipping plane
	/// @return The specified perspective projection matrix.
	static ThisType ProjectPerspective(Component l, Component r, Component b, Component t, Component n, Component f) noexcept;
	/// Create a perspective projection matrix
	/// @param d The distance to the z-plane
	static ThisType Perspective(Component d) noexcept;

	/// Return a translation matrix.
	/// @return A translation matrix.
	static ThisType Translate(const Vector3<Component>& v) noexcept;
	static ThisType Translate(Component x, Component y, Component z) noexcept;
	static ThisType TranslateX(Component x) noexcept;
	static ThisType TranslateY(Component y) noexcept;
	static ThisType TranslateZ(Component z) noexcept;

	/// Return a scaling matrix.
	/// @return A scaling matrix.
	static ThisType Scale(Component x, Component y, Component z) noexcept;
	static ThisType ScaleX(Component x) noexcept;
	static ThisType ScaleY(Component y) noexcept;
	static ThisType ScaleZ(Component z) noexcept;

	/// Return a rotation matrix.
	/// @return A rotation matrix.
	static ThisType Rotate(const Vector3<Component>& v, Component angle) noexcept;
	static ThisType RotateX(Component angle) noexcept;
	static ThisType RotateY(Component angle) noexcept;
	static ThisType RotateZ(Component angle) noexcept;

	/// Return a skew/shearing matrix.
	/// @return A skew matrix.
	static ThisType Skew(Component angle_x, Component angle_y) noexcept;
	static ThisType SkewX(Component angle) noexcept;
	static ThisType SkewY(Component angle) noexcept;

	static ThisType Compose(const Vector3<Component>& translation, const Vector3<Component>& scale, const Vector3<Component>& skew,
		const Vector4<Component>& perspective, const Vector4<Component>& quaternion) noexcept;

#if defined(RMLUI_MATRIX4_USER_EXTRA)
	RMLUI_MATRIX4_USER_EXTRA
#elif defined(RMLUI_MATRIX4_USER_INCLUDE)
	#include RMLUI_MATRIX4_USER_INCLUDE
#endif
};

} // namespace Rml

#include "Matrix4.inl"

#endif
