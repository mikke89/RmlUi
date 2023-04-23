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

#ifndef RMLUI_CORE_TRANSFORMPRIMITIVE_H
#define RMLUI_CORE_TRANSFORMPRIMITIVE_H

#include "Header.h"
#include "NumericValue.h"
#include "Types.h"
#include "Unit.h"

namespace Rml {
namespace Transforms {

	// A resolved primitive has values that are always independent of an element's properties or layout.
	template <size_t N>
	struct RMLUICORE_API ResolvedPrimitive {
		Array<float, N> values;

	protected:
		ResolvedPrimitive(const float* values) noexcept;
		ResolvedPrimitive(const NumericValue* values) noexcept;
		ResolvedPrimitive(const NumericValue* values, Array<Unit, N> base_units) noexcept;
		ResolvedPrimitive(Array<NumericValue, N> values, Array<Unit, N> base_units) noexcept;
		ResolvedPrimitive(Array<float, N> values) noexcept;
	};

	// An unresolved primitive may have values that depend on the final layout of a given element, such as its width.
	template <size_t N>
	struct RMLUICORE_API UnresolvedPrimitive {
		Array<NumericValue, N> values;

	protected:
		UnresolvedPrimitive(const NumericValue* values) noexcept;
		UnresolvedPrimitive(Array<NumericValue, N> values) noexcept;
	};

	struct RMLUICORE_API Matrix2D : public ResolvedPrimitive<6> {
		Matrix2D(const NumericValue* values) noexcept;
	};

	struct RMLUICORE_API Matrix3D : public ResolvedPrimitive<16> {
		Matrix3D(const NumericValue* values) noexcept;
		Matrix3D(const Matrix4f& matrix) noexcept;
	};

	struct RMLUICORE_API TranslateX : public UnresolvedPrimitive<1> {
		TranslateX(const NumericValue* values) noexcept;
		TranslateX(float x, Unit unit = Unit::PX) noexcept;
	};

	struct RMLUICORE_API TranslateY : public UnresolvedPrimitive<1> {
		TranslateY(const NumericValue* values) noexcept;
		TranslateY(float y, Unit unit = Unit::PX) noexcept;
	};

	struct RMLUICORE_API TranslateZ : public UnresolvedPrimitive<1> {
		TranslateZ(const NumericValue* values) noexcept;
		TranslateZ(float z, Unit unit = Unit::PX) noexcept;
	};

	struct RMLUICORE_API Translate2D : public UnresolvedPrimitive<2> {
		Translate2D(const NumericValue* values) noexcept;
		Translate2D(float x, float y, Unit units = Unit::PX) noexcept;
	};

	struct RMLUICORE_API Translate3D : public UnresolvedPrimitive<3> {
		Translate3D(const NumericValue* values) noexcept;
		Translate3D(NumericValue x, NumericValue y, NumericValue z) noexcept;
		Translate3D(float x, float y, float z, Unit units = Unit::PX) noexcept;
	};

	struct RMLUICORE_API ScaleX : public ResolvedPrimitive<1> {
		ScaleX(const NumericValue* values) noexcept;
		ScaleX(float value) noexcept;
	};

	struct RMLUICORE_API ScaleY : public ResolvedPrimitive<1> {
		ScaleY(const NumericValue* values) noexcept;
		ScaleY(float value) noexcept;
	};

	struct RMLUICORE_API ScaleZ : public ResolvedPrimitive<1> {
		ScaleZ(const NumericValue* values) noexcept;
		ScaleZ(float value) noexcept;
	};

	struct RMLUICORE_API Scale2D : public ResolvedPrimitive<2> {
		Scale2D(const NumericValue* values) noexcept;
		Scale2D(float xy) noexcept;
		Scale2D(float x, float y) noexcept;
	};

	struct RMLUICORE_API Scale3D : public ResolvedPrimitive<3> {
		Scale3D(const NumericValue* values) noexcept;
		Scale3D(float xyz) noexcept;
		Scale3D(float x, float y, float z) noexcept;
	};

	struct RMLUICORE_API RotateX : public ResolvedPrimitive<1> {
		RotateX(const NumericValue* values) noexcept;
		RotateX(float angle, Unit unit = Unit::DEG) noexcept;
	};

	struct RMLUICORE_API RotateY : public ResolvedPrimitive<1> {
		RotateY(const NumericValue* values) noexcept;
		RotateY(float angle, Unit unit = Unit::DEG) noexcept;
	};

	struct RMLUICORE_API RotateZ : public ResolvedPrimitive<1> {
		RotateZ(const NumericValue* values) noexcept;
		RotateZ(float angle, Unit unit = Unit::DEG) noexcept;
	};

	struct RMLUICORE_API Rotate2D : public ResolvedPrimitive<1> {
		Rotate2D(const NumericValue* values) noexcept;
		Rotate2D(float angle, Unit unit = Unit::DEG) noexcept;
	};

	struct RMLUICORE_API Rotate3D : public ResolvedPrimitive<4> {
		Rotate3D(const NumericValue* values) noexcept;
		Rotate3D(float x, float y, float z, float angle, Unit angle_unit = Unit::DEG) noexcept;
	};

	struct RMLUICORE_API SkewX : public ResolvedPrimitive<1> {
		SkewX(const NumericValue* values) noexcept;
		SkewX(float angle, Unit unit = Unit::DEG) noexcept;
	};

	struct RMLUICORE_API SkewY : public ResolvedPrimitive<1> {
		SkewY(const NumericValue* values) noexcept;
		SkewY(float angle, Unit unit = Unit::DEG) noexcept;
	};

	struct RMLUICORE_API Skew2D : public ResolvedPrimitive<2> {
		Skew2D(const NumericValue* values) noexcept;
		Skew2D(float x, float y, Unit unit = Unit::DEG) noexcept;
	};

	struct RMLUICORE_API Perspective : public UnresolvedPrimitive<1> {
		Perspective(const NumericValue* values) noexcept;
	};

	struct RMLUICORE_API DecomposedMatrix4 {
		Vector4f perspective;
		Vector4f quaternion;
		Vector3f translation;
		Vector3f scale;
		Vector3f skew;
	};

} // namespace Transforms

/**
    The TransformPrimitive struct is the base struct of geometric transforms such as rotations, scalings and translations.
    Instances of this struct are added to Rml::Transform during parsing of the 'transform' property.

    @author Markus Schöngart
    @see Rml::Transform
    @see Rml::PropertyParserTransform
 */
struct RMLUICORE_API TransformPrimitive {
	enum Type {
		MATRIX2D,
		MATRIX3D,
		TRANSLATEX,
		TRANSLATEY,
		TRANSLATEZ,
		TRANSLATE2D,
		TRANSLATE3D,
		SCALEX,
		SCALEY,
		SCALEZ,
		SCALE2D,
		SCALE3D,
		ROTATEX,
		ROTATEY,
		ROTATEZ,
		ROTATE2D,
		ROTATE3D,
		SKEWX,
		SKEWY,
		SKEW2D,
		PERSPECTIVE,
		DECOMPOSEDMATRIX4
	};

	TransformPrimitive(Transforms::Matrix2D p) : type(MATRIX2D) { matrix_2d = p; }
	TransformPrimitive(Transforms::Matrix3D p) : type(MATRIX3D) { matrix_3d = p; }
	TransformPrimitive(Transforms::TranslateX p) : type(TRANSLATEX) { translate_x = p; }
	TransformPrimitive(Transforms::TranslateY p) : type(TRANSLATEY) { translate_y = p; }
	TransformPrimitive(Transforms::TranslateZ p) : type(TRANSLATEZ) { translate_z = p; }
	TransformPrimitive(Transforms::Translate2D p) : type(TRANSLATE2D) { translate_2d = p; }
	TransformPrimitive(Transforms::Translate3D p) : type(TRANSLATE3D) { translate_3d = p; }
	TransformPrimitive(Transforms::ScaleX p) : type(SCALEX) { scale_x = p; }
	TransformPrimitive(Transforms::ScaleY p) : type(SCALEY) { scale_y = p; }
	TransformPrimitive(Transforms::ScaleZ p) : type(SCALEZ) { scale_z = p; }
	TransformPrimitive(Transforms::Scale2D p) : type(SCALE2D) { scale_2d = p; }
	TransformPrimitive(Transforms::Scale3D p) : type(SCALE3D) { scale_3d = p; }
	TransformPrimitive(Transforms::RotateX p) : type(ROTATEX) { rotate_x = p; }
	TransformPrimitive(Transforms::RotateY p) : type(ROTATEY) { rotate_y = p; }
	TransformPrimitive(Transforms::RotateZ p) : type(ROTATEZ) { rotate_z = p; }
	TransformPrimitive(Transforms::Rotate2D p) : type(ROTATE2D) { rotate_2d = p; }
	TransformPrimitive(Transforms::Rotate3D p) : type(ROTATE3D) { rotate_3d = p; }
	TransformPrimitive(Transforms::SkewX p) : type(SKEWX) { skew_x = p; }
	TransformPrimitive(Transforms::SkewY p) : type(SKEWY) { skew_y = p; }
	TransformPrimitive(Transforms::Skew2D p) : type(SKEW2D) { skew_2d = p; }
	TransformPrimitive(Transforms::Perspective p) : type(PERSPECTIVE) { perspective = p; }
	TransformPrimitive(Transforms::DecomposedMatrix4 p) : type(DECOMPOSEDMATRIX4) { decomposed_matrix_4 = p; }

	Type type;

	union {
		Transforms::Matrix2D matrix_2d;
		Transforms::Matrix3D matrix_3d;
		Transforms::TranslateX translate_x;
		Transforms::TranslateY translate_y;
		Transforms::TranslateZ translate_z;
		Transforms::Translate2D translate_2d;
		Transforms::Translate3D translate_3d;
		Transforms::ScaleX scale_x;
		Transforms::ScaleY scale_y;
		Transforms::ScaleZ scale_z;
		Transforms::Scale2D scale_2d;
		Transforms::Scale3D scale_3d;
		Transforms::RotateX rotate_x;
		Transforms::RotateY rotate_y;
		Transforms::RotateZ rotate_z;
		Transforms::Rotate2D rotate_2d;
		Transforms::Rotate3D rotate_3d;
		Transforms::SkewX skew_x;
		Transforms::SkewY skew_y;
		Transforms::Skew2D skew_2d;
		Transforms::Perspective perspective;
		Transforms::DecomposedMatrix4 decomposed_matrix_4;
	};
};

} // namespace Rml
#endif
