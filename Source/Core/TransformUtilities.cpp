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

#include "TransformUtilities.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/TransformPrimitive.h"

namespace Rml {

using namespace Transforms;

static Vector3f Combine(const Vector3f a, const Vector3f b, float a_scale, float b_scale)
{
	Vector3f result;
	result.x = a_scale * a.x + b_scale * b.x;
	result.y = a_scale * a.y + b_scale * b.y;
	result.z = a_scale * a.z + b_scale * b.z;
	return result;
}

// Interpolate two quaternions a, b with weight alpha [0, 1]
static Vector4f QuaternionSlerp(const Vector4f a, const Vector4f b, float alpha)
{
	using namespace Math;

	const float eps = 0.9995f;

	float dot = a.DotProduct(b);
	dot = Clamp(dot, -1.f, 1.f);

	if (dot > eps)
		return a;

	float theta = ACos(dot);
	float w = Sin(alpha * theta) / SquareRoot(1.f - dot * dot);
	float a_scale = Cos(alpha * theta) - dot * w;

	Vector4f result;
	for (int i = 0; i < 4; i++)
	{
		result[i] = a[i] * a_scale + b[i] * w;
	}

	return result;
}

// Resolve a numeric property value with the element's width as relative base value.
static inline float ResolveWidth(NumericValue value, Element& e) noexcept
{
	if (value.unit == Unit::PX || value.unit == Unit::NUMBER)
		return value.number;
	return e.ResolveNumericValue(value, e.GetBox().GetSize(BoxArea::Border).x);
}

// Resolve a numeric property value with the element's height as relative base value.
static inline float ResolveHeight(NumericValue value, Element& e) noexcept
{
	if (value.unit == Unit::PX || value.unit == Unit::NUMBER)
		return value.number;
	return e.ResolveNumericValue(value, e.GetBox().GetSize(BoxArea::Border).y);
}

// Resolve a length numeric property value for the given element.
static inline float ResolveLength(NumericValue value, Element& e) noexcept
{
	if (value.unit == Unit::PX || value.unit == Unit::NUMBER)
		return value.number;
	return e.ResolveLength(value);
}

struct SetIdentityVisitor {
	template <size_t N>
	void operator()(Transforms::ResolvedPrimitive<N>& p)
	{
		for (auto& value : p.values)
			value = 0.0f;
	}
	template <size_t N>
	void operator()(Transforms::UnresolvedPrimitive<N>& p)
	{
		for (auto& value : p.values)
			value.number = 0.0f;
	}
	void operator()(Transforms::Matrix2D& p)
	{
		for (int i = 0; i < 6; i++)
			p.values[i] = ((i == 0 || i == 3) ? 1.0f : 0.0f);
	}
	void operator()(Transforms::Matrix3D& p)
	{
		for (int i = 0; i < 16; i++)
			p.values[i] = ((i % 5) == 0 ? 1.0f : 0.0f);
	}
	void operator()(Transforms::ScaleX& p) { p.values[0] = 1; }
	void operator()(Transforms::ScaleY& p) { p.values[0] = 1; }
	void operator()(Transforms::ScaleZ& p) { p.values[0] = 1; }
	void operator()(Transforms::Scale2D& p) { p.values[0] = p.values[1] = 1; }
	void operator()(Transforms::Scale3D& p) { p.values[0] = p.values[1] = p.values[2] = 1; }
	void operator()(Transforms::DecomposedMatrix4& p)
	{
		p.perspective = Vector4f(0, 0, 0, 1);
		p.quaternion = Vector4f(0, 0, 0, 1);
		p.translation = Vector3f(0, 0, 0);
		p.scale = Vector3f(1, 1, 1);
		p.skew = Vector3f(0, 0, 0);
	}

	void run(TransformPrimitive& primitive)
	{
		switch (primitive.type)
		{
		case TransformPrimitive::MATRIX2D: this->operator()(primitive.matrix_2d); break;
		case TransformPrimitive::MATRIX3D: this->operator()(primitive.matrix_3d); break;
		case TransformPrimitive::TRANSLATEX: this->operator()(primitive.translate_x); break;
		case TransformPrimitive::TRANSLATEY: this->operator()(primitive.translate_y); break;
		case TransformPrimitive::TRANSLATEZ: this->operator()(primitive.translate_z); break;
		case TransformPrimitive::TRANSLATE2D: this->operator()(primitive.translate_2d); break;
		case TransformPrimitive::TRANSLATE3D: this->operator()(primitive.translate_3d); break;
		case TransformPrimitive::SCALEX: this->operator()(primitive.scale_x); break;
		case TransformPrimitive::SCALEY: this->operator()(primitive.scale_y); break;
		case TransformPrimitive::SCALEZ: this->operator()(primitive.scale_z); break;
		case TransformPrimitive::SCALE2D: this->operator()(primitive.scale_2d); break;
		case TransformPrimitive::SCALE3D: this->operator()(primitive.scale_3d); break;
		case TransformPrimitive::ROTATEX: this->operator()(primitive.rotate_x); break;
		case TransformPrimitive::ROTATEY: this->operator()(primitive.rotate_y); break;
		case TransformPrimitive::ROTATEZ: this->operator()(primitive.rotate_z); break;
		case TransformPrimitive::ROTATE2D: this->operator()(primitive.rotate_2d); break;
		case TransformPrimitive::ROTATE3D: this->operator()(primitive.rotate_3d); break;
		case TransformPrimitive::SKEWX: this->operator()(primitive.skew_x); break;
		case TransformPrimitive::SKEWY: this->operator()(primitive.skew_y); break;
		case TransformPrimitive::SKEW2D: this->operator()(primitive.skew_2d); break;
		case TransformPrimitive::PERSPECTIVE: this->operator()(primitive.perspective); break;
		case TransformPrimitive::DECOMPOSEDMATRIX4: this->operator()(primitive.decomposed_matrix_4); break;
		}
	}
};

void TransformUtilities::SetIdentity(TransformPrimitive& p) noexcept
{
	SetIdentityVisitor{}.run(p);
}

struct ResolveTransformVisitor {
	Matrix4f& m;
	Element& e;

	void operator()(const Transforms::Matrix2D& p)
	{
		m = Matrix4f::FromRows(Vector4f(p.values[0], p.values[2], 0, p.values[4]), Vector4f(p.values[1], p.values[3], 0, p.values[5]),
			Vector4f(0, 0, 1, 0), Vector4f(0, 0, 0, 1));
	}
	void operator()(const Transforms::Matrix3D& p)
	{
		m = Matrix4f::FromColumns(Vector4f(p.values[0], p.values[1], p.values[2], p.values[3]),
			Vector4f(p.values[4], p.values[5], p.values[6], p.values[7]), Vector4f(p.values[8], p.values[9], p.values[10], p.values[11]),
			Vector4f(p.values[12], p.values[13], p.values[14], p.values[15]));
	}

	void operator()(const Transforms::TranslateX& p) { m = Matrix4f::TranslateX(ResolveWidth(p.values[0], e)); }
	void operator()(const Transforms::TranslateY& p) { m = Matrix4f::TranslateY(ResolveHeight(p.values[0], e)); }
	void operator()(const Transforms::TranslateZ& p) { m = Matrix4f::TranslateZ(ResolveLength(p.values[0], e)); }
	void operator()(const Transforms::Translate2D& p) { m = Matrix4f::Translate(ResolveWidth(p.values[0], e), ResolveHeight(p.values[1], e), 0); }
	void operator()(const Transforms::Translate3D& p)
	{
		m = Matrix4f::Translate(ResolveWidth(p.values[0], e), ResolveHeight(p.values[1], e), ResolveLength(p.values[2], e));
	}

	void operator()(const Transforms::ScaleX& p) { m = Matrix4f::ScaleX(p.values[0]); }
	void operator()(const Transforms::ScaleY& p) { m = Matrix4f::ScaleY(p.values[0]); }
	void operator()(const Transforms::ScaleZ& p) { m = Matrix4f::ScaleZ(p.values[0]); }
	void operator()(const Transforms::Scale2D& p) { m = Matrix4f::Scale(p.values[0], p.values[1], 1); }
	void operator()(const Transforms::Scale3D& p) { m = Matrix4f::Scale(p.values[0], p.values[1], p.values[2]); }

	void operator()(const Transforms::RotateX& p) { m = Matrix4f::RotateX(p.values[0]); }
	void operator()(const Transforms::RotateY& p) { m = Matrix4f::RotateY(p.values[0]); }
	void operator()(const Transforms::RotateZ& p) { m = Matrix4f::RotateZ(p.values[0]); }
	void operator()(const Transforms::Rotate2D& p) { m = Matrix4f::RotateZ(p.values[0]); }
	void operator()(const Transforms::Rotate3D& p) { m = Matrix4f::Rotate(Vector3f(p.values[0], p.values[1], p.values[2]), p.values[3]); }

	void operator()(const Transforms::SkewX& p) { m = Matrix4f::SkewX(p.values[0]); }
	void operator()(const Transforms::SkewY& p) { m = Matrix4f::SkewY(p.values[0]); }
	void operator()(const Transforms::Skew2D& p) { m = Matrix4f::Skew(p.values[0], p.values[1]); }

	void operator()(const Transforms::DecomposedMatrix4& p) { m = Matrix4f::Compose(p.translation, p.scale, p.skew, p.perspective, p.quaternion); }
	void operator()(const Transforms::Perspective& p) { m = Matrix4f::Perspective(ResolveLength(p.values[0], e)); }

	void run(const TransformPrimitive& primitive)
	{
		switch (primitive.type)
		{
		case TransformPrimitive::MATRIX2D: this->operator()(primitive.matrix_2d); break;
		case TransformPrimitive::MATRIX3D: this->operator()(primitive.matrix_3d); break;
		case TransformPrimitive::TRANSLATEX: this->operator()(primitive.translate_x); break;
		case TransformPrimitive::TRANSLATEY: this->operator()(primitive.translate_y); break;
		case TransformPrimitive::TRANSLATEZ: this->operator()(primitive.translate_z); break;
		case TransformPrimitive::TRANSLATE2D: this->operator()(primitive.translate_2d); break;
		case TransformPrimitive::TRANSLATE3D: this->operator()(primitive.translate_3d); break;
		case TransformPrimitive::SCALEX: this->operator()(primitive.scale_x); break;
		case TransformPrimitive::SCALEY: this->operator()(primitive.scale_y); break;
		case TransformPrimitive::SCALEZ: this->operator()(primitive.scale_z); break;
		case TransformPrimitive::SCALE2D: this->operator()(primitive.scale_2d); break;
		case TransformPrimitive::SCALE3D: this->operator()(primitive.scale_3d); break;
		case TransformPrimitive::ROTATEX: this->operator()(primitive.rotate_x); break;
		case TransformPrimitive::ROTATEY: this->operator()(primitive.rotate_y); break;
		case TransformPrimitive::ROTATEZ: this->operator()(primitive.rotate_z); break;
		case TransformPrimitive::ROTATE2D: this->operator()(primitive.rotate_2d); break;
		case TransformPrimitive::ROTATE3D: this->operator()(primitive.rotate_3d); break;
		case TransformPrimitive::SKEWX: this->operator()(primitive.skew_x); break;
		case TransformPrimitive::SKEWY: this->operator()(primitive.skew_y); break;
		case TransformPrimitive::SKEW2D: this->operator()(primitive.skew_2d); break;
		case TransformPrimitive::PERSPECTIVE: this->operator()(primitive.perspective); break;
		case TransformPrimitive::DECOMPOSEDMATRIX4: this->operator()(primitive.decomposed_matrix_4); break;
		}
	}
};

Matrix4f TransformUtilities::ResolveTransform(const TransformPrimitive& p, Element& e) noexcept
{
	Matrix4f m;
	ResolveTransformVisitor visitor{m, e};
	visitor.run(p);
	return m;
}

struct PrepareVisitor {
	Element& e;

	bool operator()(TranslateX& p)
	{
		p.values[0] = NumericValue{ResolveWidth(p.values[0], e), Unit::PX};
		return true;
	}
	bool operator()(TranslateY& p)
	{
		p.values[0] = NumericValue{ResolveHeight(p.values[0], e), Unit::PX};
		return true;
	}
	bool operator()(TranslateZ& p)
	{
		p.values[0] = NumericValue{ResolveLength(p.values[0], e), Unit::PX};
		return true;
	}
	bool operator()(Translate2D& p)
	{
		p.values[0] = NumericValue{ResolveWidth(p.values[0], e), Unit::PX};
		p.values[1] = NumericValue{ResolveHeight(p.values[1], e), Unit::PX};
		return true;
	}
	bool operator()(Translate3D& p)
	{
		p.values[0] = NumericValue{ResolveWidth(p.values[0], e), Unit::PX};
		p.values[1] = NumericValue{ResolveHeight(p.values[1], e), Unit::PX};
		p.values[2] = NumericValue{ResolveLength(p.values[2], e), Unit::PX};
		return true;
	}
	template <size_t N>
	bool operator()(ResolvedPrimitive<N>& /*p*/)
	{
		// No conversion needed for resolved transforms (with some exceptions below)
		return true;
	}
	bool operator()(DecomposedMatrix4& /*p*/) { return true; }
	bool operator()(Rotate3D& p)
	{
		// Rotate3D can be interpolated if and only if their rotation axes point in the same direction.
		// We normalize the rotation vector here for easy comparison, and return true here. Later on we make the
		// pair-wise check in 'TryConvertToMatchingGenericType' to see if we need to decompose.
		Vector3f vec = Vector3f(p.values[0], p.values[1], p.values[2]).Normalise();
		p.values[0] = vec.x;
		p.values[1] = vec.y;
		p.values[2] = vec.z;
		return true;
	}
	bool operator()(Matrix3D& /*p*/)
	{
		// Matrices must be decomposed for interpolation
		return false;
	}
	bool operator()(Matrix2D& /*p*/)
	{
		// Matrix2D can also be optimized for interpolation, but for now we decompose it to a full DecomposedMatrix4
		return false;
	}
	bool operator()(Perspective& /*p*/)
	{
		// Perspective must be decomposed
		return false;
	}

	bool run(TransformPrimitive& primitive)
	{
		switch (primitive.type)
		{
		case TransformPrimitive::MATRIX2D: return this->operator()(primitive.matrix_2d);
		case TransformPrimitive::MATRIX3D: return this->operator()(primitive.matrix_3d);
		case TransformPrimitive::TRANSLATEX: return this->operator()(primitive.translate_x);
		case TransformPrimitive::TRANSLATEY: return this->operator()(primitive.translate_y);
		case TransformPrimitive::TRANSLATEZ: return this->operator()(primitive.translate_z);
		case TransformPrimitive::TRANSLATE2D: return this->operator()(primitive.translate_2d);
		case TransformPrimitive::TRANSLATE3D: return this->operator()(primitive.translate_3d);
		case TransformPrimitive::SCALEX: return this->operator()(primitive.scale_x);
		case TransformPrimitive::SCALEY: return this->operator()(primitive.scale_y);
		case TransformPrimitive::SCALEZ: return this->operator()(primitive.scale_z);
		case TransformPrimitive::SCALE2D: return this->operator()(primitive.scale_2d);
		case TransformPrimitive::SCALE3D: return this->operator()(primitive.scale_3d);
		case TransformPrimitive::ROTATEX: return this->operator()(primitive.rotate_x);
		case TransformPrimitive::ROTATEY: return this->operator()(primitive.rotate_y);
		case TransformPrimitive::ROTATEZ: return this->operator()(primitive.rotate_z);
		case TransformPrimitive::ROTATE2D: return this->operator()(primitive.rotate_2d);
		case TransformPrimitive::ROTATE3D: return this->operator()(primitive.rotate_3d);
		case TransformPrimitive::SKEWX: return this->operator()(primitive.skew_x);
		case TransformPrimitive::SKEWY: return this->operator()(primitive.skew_y);
		case TransformPrimitive::SKEW2D: return this->operator()(primitive.skew_2d);
		case TransformPrimitive::PERSPECTIVE: return this->operator()(primitive.perspective);
		case TransformPrimitive::DECOMPOSEDMATRIX4: return this->operator()(primitive.decomposed_matrix_4);
		}
		RMLUI_ASSERT(false);
		return false;
	}
};

bool TransformUtilities::PrepareForInterpolation(TransformPrimitive& p, Element& e) noexcept
{
	return PrepareVisitor{e}.run(p);
}

enum class GenericType { None, Scale3D, Translate3D, Rotate3D };

struct GetGenericTypeVisitor {
	GenericType run(const TransformPrimitive& primitive)
	{
		switch (primitive.type)
		{
		case TransformPrimitive::TRANSLATEX:
		case TransformPrimitive::TRANSLATEY:
		case TransformPrimitive::TRANSLATEZ:
		case TransformPrimitive::TRANSLATE2D:
		case TransformPrimitive::TRANSLATE3D: return GenericType::Translate3D;

		case TransformPrimitive::SCALEX:
		case TransformPrimitive::SCALEY:
		case TransformPrimitive::SCALEZ:
		case TransformPrimitive::SCALE2D:
		case TransformPrimitive::SCALE3D: return GenericType::Scale3D;

		case TransformPrimitive::ROTATEX:
		case TransformPrimitive::ROTATEY:
		case TransformPrimitive::ROTATEZ:
		case TransformPrimitive::ROTATE2D:
		case TransformPrimitive::ROTATE3D: return GenericType::Rotate3D;

		case TransformPrimitive::MATRIX2D:
		case TransformPrimitive::MATRIX3D:
		case TransformPrimitive::SKEWX:
		case TransformPrimitive::SKEWY:
		case TransformPrimitive::SKEW2D:
		case TransformPrimitive::PERSPECTIVE:
		case TransformPrimitive::DECOMPOSEDMATRIX4: return GenericType::None;
		}
		RMLUI_ASSERT(false);
		return GenericType::None;
	}
};

struct ConvertToGenericTypeVisitor {
	Translate3D operator()(const TranslateX& p) { return Translate3D{p.values[0], {0.0f, Unit::PX}, {0.0f, Unit::PX}}; }
	Translate3D operator()(const TranslateY& p) { return Translate3D{{0.0f, Unit::PX}, p.values[0], {0.0f, Unit::PX}}; }
	Translate3D operator()(const TranslateZ& p) { return Translate3D{{0.0f, Unit::PX}, {0.0f, Unit::PX}, p.values[0]}; }
	Translate3D operator()(const Translate2D& p) { return Translate3D{p.values[0], p.values[1], {0.0f, Unit::PX}}; }
	Scale3D operator()(const ScaleX& p) { return Scale3D{p.values[0], 1.0f, 1.0f}; }
	Scale3D operator()(const ScaleY& p) { return Scale3D{1.0f, p.values[0], 1.0f}; }
	Scale3D operator()(const ScaleZ& p) { return Scale3D{1.0f, 1.0f, p.values[0]}; }
	Scale3D operator()(const Scale2D& p) { return Scale3D{p.values[0], p.values[1], 1.0f}; }
	Rotate3D operator()(const RotateX& p) { return Rotate3D{1, 0, 0, p.values[0], Unit::RAD}; }
	Rotate3D operator()(const RotateY& p) { return Rotate3D{0, 1, 0, p.values[0], Unit::RAD}; }
	Rotate3D operator()(const RotateZ& p) { return Rotate3D{0, 0, 1, p.values[0], Unit::RAD}; }
	Rotate3D operator()(const Rotate2D& p) { return Rotate3D{0, 0, 1, p.values[0], Unit::RAD}; }

	template <typename T>
	TransformPrimitive operator()(const T& p)
	{
		RMLUI_ERROR;
		return p;
	}

	TransformPrimitive run(const TransformPrimitive& primitive)
	{
		TransformPrimitive result = primitive;
		// clang-format off
		switch (primitive.type)
		{
		case TransformPrimitive::TRANSLATEX:  result.type = TransformPrimitive::TRANSLATE3D; result.translate_3d = this->operator()(primitive.translate_x);  break;
		case TransformPrimitive::TRANSLATEY:  result.type = TransformPrimitive::TRANSLATE3D; result.translate_3d = this->operator()(primitive.translate_y);  break;
		case TransformPrimitive::TRANSLATEZ:  result.type = TransformPrimitive::TRANSLATE3D; result.translate_3d = this->operator()(primitive.translate_z);  break;
		case TransformPrimitive::TRANSLATE2D: result.type = TransformPrimitive::TRANSLATE3D; result.translate_3d = this->operator()(primitive.translate_2d); break;
		case TransformPrimitive::TRANSLATE3D: break;
		case TransformPrimitive::SCALEX:      result.type = TransformPrimitive::SCALE3D;     result.scale_3d = this->operator()(primitive.scale_x);      break;
		case TransformPrimitive::SCALEY:      result.type = TransformPrimitive::SCALE3D;     result.scale_3d = this->operator()(primitive.scale_y);      break;
		case TransformPrimitive::SCALEZ:      result.type = TransformPrimitive::SCALE3D;     result.scale_3d = this->operator()(primitive.scale_z);      break;
		case TransformPrimitive::SCALE2D:     result.type = TransformPrimitive::SCALE3D;     result.scale_3d = this->operator()(primitive.scale_2d);     break;
		case TransformPrimitive::SCALE3D:     break;
		case TransformPrimitive::ROTATEX:     result.type = TransformPrimitive::ROTATE3D;    result.rotate_3d = this->operator()(primitive.rotate_x);     break;
		case TransformPrimitive::ROTATEY:     result.type = TransformPrimitive::ROTATE3D;    result.rotate_3d = this->operator()(primitive.rotate_y);     break;
		case TransformPrimitive::ROTATEZ:     result.type = TransformPrimitive::ROTATE3D;    result.rotate_3d = this->operator()(primitive.rotate_z);     break;
		case TransformPrimitive::ROTATE2D:    result.type = TransformPrimitive::ROTATE3D;    result.rotate_3d = this->operator()(primitive.rotate_2d);    break;
		case TransformPrimitive::ROTATE3D:    break;
		default: RMLUI_ASSERT(false); break;
		}
		// clang-format on
		return result;
	}
};

static bool CanInterpolateRotate3D(const Rotate3D& p0, const Rotate3D& p1)
{
	// Rotate3D can only be interpolated if and only if their rotation axes point in the same direction.
	// Assumes each rotation axis has already been normalized.
	auto& v0 = p0.values;
	auto& v1 = p1.values;
	return v0[0] == v1[0] && v0[1] == v1[1] && v0[2] == v1[2];
}

bool TransformUtilities::TryConvertToMatchingGenericType(TransformPrimitive& p0, TransformPrimitive& p1) noexcept
{
	if (p0.type == p1.type)
	{
		if (p0.type == TransformPrimitive::ROTATE3D && !CanInterpolateRotate3D(p0.rotate_3d, p1.rotate_3d))
			return false;

		return true;
	}

	GenericType c0 = GetGenericTypeVisitor{}.run(p0);
	GenericType c1 = GetGenericTypeVisitor{}.run(p1);

	if (c0 == c1 && c0 != GenericType::None)
	{
		TransformPrimitive new_p0 = ConvertToGenericTypeVisitor{}.run(p0);
		TransformPrimitive new_p1 = ConvertToGenericTypeVisitor{}.run(p1);

		RMLUI_ASSERT(new_p0.type == new_p1.type);

		if (new_p0.type == TransformPrimitive::ROTATE3D && !CanInterpolateRotate3D(new_p0.rotate_3d, new_p1.rotate_3d))
			return false;

		p0 = new_p0;
		p1 = new_p1;

		return true;
	}

	return false;
}

struct InterpolateVisitor {
	const TransformPrimitive& other_variant;
	float alpha;

	template <size_t N>
	bool Interpolate(ResolvedPrimitive<N>& p0, const ResolvedPrimitive<N>& p1)
	{
		for (size_t i = 0; i < N; i++)
			p0.values[i] = p0.values[i] * (1.0f - alpha) + p1.values[i] * alpha;
		return true;
	}
	template <size_t N>
	bool Interpolate(UnresolvedPrimitive<N>& p0, const UnresolvedPrimitive<N>& p1)
	{
		// Assumes that the underlying units have been resolved (e.g. to pixels)
		for (size_t i = 0; i < N; i++)
			p0.values[i].number = p0.values[i].number * (1.0f - alpha) + p1.values[i].number * alpha;
		return true;
	}
	bool Interpolate(Rotate3D& p0, const Rotate3D& p1)
	{
		RMLUI_ASSERT(CanInterpolateRotate3D(p0, p1));
		// We can only interpolate rotate3d if their rotation axes align. That should be the case if we get here,
		// otherwise the generic type matching should decompose them. Thus, we only need to interpolate
		// the angle value here.
		p0.values[3] = p0.values[3] * (1.0f - alpha) + p1.values[3] * alpha;
		return true;
	}
	bool Interpolate(Matrix2D& /*p0*/, const Matrix2D& /*p1*/)
	{
		RMLUI_ERROR;
		return false; /* Error if we get here, see PrepareForInterpolation() */
	}
	bool Interpolate(Matrix3D& /*p0*/, const Matrix3D& /*p1*/)
	{
		RMLUI_ERROR;
		return false; /* Error if we get here, see PrepareForInterpolation() */
	}
	bool Interpolate(Perspective& /*p0*/, const Perspective& /*p1*/)
	{
		RMLUI_ERROR;
		return false; /* Error if we get here, see PrepareForInterpolation() */
	}

	bool Interpolate(DecomposedMatrix4& p0, const DecomposedMatrix4& p1)
	{
		p0.perspective = p0.perspective * (1.0f - alpha) + p1.perspective * alpha;
		p0.quaternion = QuaternionSlerp(p0.quaternion, p1.quaternion, alpha);
		p0.translation = p0.translation * (1.0f - alpha) + p1.translation * alpha;
		p0.scale = p0.scale * (1.0f - alpha) + p1.scale * alpha;
		p0.skew = p0.skew * (1.0f - alpha) + p1.skew * alpha;
		return true;
	}

	bool run(TransformPrimitive& variant)
	{
		RMLUI_ASSERT(variant.type == other_variant.type);
		switch (variant.type)
		{
		case TransformPrimitive::MATRIX2D: return Interpolate(variant.matrix_2d, other_variant.matrix_2d);
		case TransformPrimitive::MATRIX3D: return Interpolate(variant.matrix_3d, other_variant.matrix_3d);
		case TransformPrimitive::TRANSLATEX: return Interpolate(variant.translate_x, other_variant.translate_x);
		case TransformPrimitive::TRANSLATEY: return Interpolate(variant.translate_y, other_variant.translate_y);
		case TransformPrimitive::TRANSLATEZ: return Interpolate(variant.translate_z, other_variant.translate_z);
		case TransformPrimitive::TRANSLATE2D: return Interpolate(variant.translate_2d, other_variant.translate_2d);
		case TransformPrimitive::TRANSLATE3D: return Interpolate(variant.translate_3d, other_variant.translate_3d);
		case TransformPrimitive::SCALEX: return Interpolate(variant.scale_x, other_variant.scale_x);
		case TransformPrimitive::SCALEY: return Interpolate(variant.scale_y, other_variant.scale_y);
		case TransformPrimitive::SCALEZ: return Interpolate(variant.scale_z, other_variant.scale_z);
		case TransformPrimitive::SCALE2D: return Interpolate(variant.scale_2d, other_variant.scale_2d);
		case TransformPrimitive::SCALE3D: return Interpolate(variant.scale_3d, other_variant.scale_3d);
		case TransformPrimitive::ROTATEX: return Interpolate(variant.rotate_x, other_variant.rotate_x);
		case TransformPrimitive::ROTATEY: return Interpolate(variant.rotate_y, other_variant.rotate_y);
		case TransformPrimitive::ROTATEZ: return Interpolate(variant.rotate_z, other_variant.rotate_z);
		case TransformPrimitive::ROTATE2D: return Interpolate(variant.rotate_2d, other_variant.rotate_2d);
		case TransformPrimitive::ROTATE3D: return Interpolate(variant.rotate_3d, other_variant.rotate_3d);
		case TransformPrimitive::SKEWX: return Interpolate(variant.skew_x, other_variant.skew_x);
		case TransformPrimitive::SKEWY: return Interpolate(variant.skew_y, other_variant.skew_y);
		case TransformPrimitive::SKEW2D: return Interpolate(variant.skew_2d, other_variant.skew_2d);
		case TransformPrimitive::PERSPECTIVE: return Interpolate(variant.perspective, other_variant.perspective);
		case TransformPrimitive::DECOMPOSEDMATRIX4: return Interpolate(variant.decomposed_matrix_4, other_variant.decomposed_matrix_4);
		}
		RMLUI_ASSERT(false);
		return false;
	}
};

bool TransformUtilities::InterpolateWith(TransformPrimitive& target, const TransformPrimitive& other, float alpha) noexcept
{
	if (target.type != other.type)
		return false;

	bool result = InterpolateVisitor{other, alpha}.run(target);
	return result;
}

template <size_t N>
static String ToString(const Transforms::ResolvedPrimitive<N>& p, const String& unit, bool rad_to_deg = false,
	bool only_unit_on_last_value = false) noexcept
{
	float multiplier = 1.0f;
	String tmp;
	String result = "(";
	for (size_t i = 0; i < N; i++)
	{
		if (only_unit_on_last_value && i < N - 1)
			multiplier = 1.0f;
		else if (rad_to_deg)
			multiplier = 180.f / Math::RMLUI_PI;

		if (TypeConverter<float, String>::Convert(p.values[i] * multiplier, tmp))
			result += tmp;

		if (!unit.empty() && (!only_unit_on_last_value || (i == N - 1)))
			result += unit;

		if (i < N - 1)
			result += ", ";
	}
	result += ")";
	return result;
}

static inline String ToString(NumericValue value) noexcept
{
	return ToString(value.number) + ToString(value.unit);
}

template <size_t N>
static inline String ToString(const Transforms::UnresolvedPrimitive<N>& p) noexcept
{
	String result = "(";
	for (size_t i = 0; i < N; i++)
	{
		result += ToString(p.values[i]);
		if (i != N - 1)
			result += ", ";
	}
	result += ")";
	return result;
}

static inline String ToString(const Transforms::DecomposedMatrix4& p) noexcept
{
	static const Transforms::DecomposedMatrix4 d{Vector4f(0, 0, 0, 1), Vector4f(0, 0, 0, 1), Vector3f(0, 0, 0), Vector3f(1, 1, 1), Vector3f(0, 0, 0)};
	String tmp;
	String result;

	if (p.perspective != d.perspective && TypeConverter<Vector4f, String>::Convert(p.perspective, tmp))
		result += "perspective(" + tmp + "), ";
	if (p.quaternion != d.quaternion && TypeConverter<Vector4f, String>::Convert(p.quaternion, tmp))
		result += "quaternion(" + tmp + "), ";
	if (p.translation != d.translation && TypeConverter<Vector3f, String>::Convert(p.translation, tmp))
		result += "translation(" + tmp + "), ";
	if (p.scale != d.scale && TypeConverter<Vector3f, String>::Convert(p.scale, tmp))
		result += "scale(" + tmp + "), ";
	if (p.skew != d.skew && TypeConverter<Vector3f, String>::Convert(p.skew, tmp))
		result += "skew(" + tmp + "), ";

	if (result.size() > 2)
		result.resize(result.size() - 2);

	result = "decomposedMatrix3d{ " + result + " }";

	return result;
}

// clang-format off
static inline String ToString(const Transforms::Matrix2D& p)    noexcept { return "matrix"      + ToString(static_cast<const Transforms::ResolvedPrimitive< 6 >&>(p), ""); }
static inline String ToString(const Transforms::Matrix3D& p)    noexcept { return "matrix3d"    + ToString(static_cast<const Transforms::ResolvedPrimitive< 16 >&>(p), ""); }
static inline String ToString(const Transforms::TranslateX& p)  noexcept { return "translateX"  + ToString(static_cast<const Transforms::UnresolvedPrimitive< 1 >&>(p)); }
static inline String ToString(const Transforms::TranslateY& p)  noexcept { return "translateY"  + ToString(static_cast<const Transforms::UnresolvedPrimitive< 1 >&>(p)); }
static inline String ToString(const Transforms::TranslateZ& p)  noexcept { return "translateZ"  + ToString(static_cast<const Transforms::UnresolvedPrimitive< 1 >&>(p)); }
static inline String ToString(const Transforms::Translate2D& p) noexcept { return "translate"   + ToString(static_cast<const Transforms::UnresolvedPrimitive< 2 >&>(p)); }
static inline String ToString(const Transforms::Translate3D& p) noexcept { return "translate3d" + ToString(static_cast<const Transforms::UnresolvedPrimitive< 3 >&>(p)); }
static inline String ToString(const Transforms::ScaleX& p)      noexcept { return "scaleX"      + ToString(static_cast<const Transforms::ResolvedPrimitive< 1 >&>(p), ""); }
static inline String ToString(const Transforms::ScaleY& p)      noexcept { return "scaleY"      + ToString(static_cast<const Transforms::ResolvedPrimitive< 1 >&>(p), ""); }
static inline String ToString(const Transforms::ScaleZ& p)      noexcept { return "scaleZ"      + ToString(static_cast<const Transforms::ResolvedPrimitive< 1 >&>(p), ""); }
static inline String ToString(const Transforms::Scale2D& p)     noexcept { return "scale"       + ToString(static_cast<const Transforms::ResolvedPrimitive< 2 >&>(p), ""); }
static inline String ToString(const Transforms::Scale3D& p)     noexcept { return "scale3d"     + ToString(static_cast<const Transforms::ResolvedPrimitive< 3 >&>(p), ""); }
static inline String ToString(const Transforms::RotateX& p)     noexcept { return "rotateX"     + ToString(static_cast<const Transforms::ResolvedPrimitive< 1 >&>(p), "deg", true); }
static inline String ToString(const Transforms::RotateY& p)     noexcept { return "rotateY"     + ToString(static_cast<const Transforms::ResolvedPrimitive< 1 >&>(p), "deg", true); }
static inline String ToString(const Transforms::RotateZ& p)     noexcept { return "rotateZ"     + ToString(static_cast<const Transforms::ResolvedPrimitive< 1 >&>(p), "deg", true); }
static inline String ToString(const Transforms::Rotate2D& p)    noexcept { return "rotate"      + ToString(static_cast<const Transforms::ResolvedPrimitive< 1 >&>(p), "deg", true); }
static inline String ToString(const Transforms::Rotate3D& p)    noexcept { return "rotate3d"    + ToString(static_cast<const Transforms::ResolvedPrimitive< 4 >&>(p), "deg", true, true); }
static inline String ToString(const Transforms::SkewX& p)       noexcept { return "skewX"       + ToString(static_cast<const Transforms::ResolvedPrimitive< 1 >&>(p), "deg", true); }
static inline String ToString(const Transforms::SkewY& p)       noexcept { return "skewY"       + ToString(static_cast<const Transforms::ResolvedPrimitive< 1 >&>(p), "deg", true); }
static inline String ToString(const Transforms::Skew2D& p)      noexcept { return "skew"        + ToString(static_cast<const Transforms::ResolvedPrimitive< 2 >&>(p), "deg", true); }
static inline String ToString(const Transforms::Perspective& p) noexcept { return "perspective" + ToString(static_cast<const Transforms::UnresolvedPrimitive< 1 >&>(p)); }
// clang-format on

struct ToStringVisitor {
	String run(const TransformPrimitive& variant)
	{
		switch (variant.type)
		{
		case TransformPrimitive::MATRIX2D: return ToString(variant.matrix_2d);
		case TransformPrimitive::MATRIX3D: return ToString(variant.matrix_3d);
		case TransformPrimitive::TRANSLATEX: return ToString(variant.translate_x);
		case TransformPrimitive::TRANSLATEY: return ToString(variant.translate_y);
		case TransformPrimitive::TRANSLATEZ: return ToString(variant.translate_z);
		case TransformPrimitive::TRANSLATE2D: return ToString(variant.translate_2d);
		case TransformPrimitive::TRANSLATE3D: return ToString(variant.translate_3d);
		case TransformPrimitive::SCALEX: return ToString(variant.scale_x);
		case TransformPrimitive::SCALEY: return ToString(variant.scale_y);
		case TransformPrimitive::SCALEZ: return ToString(variant.scale_z);
		case TransformPrimitive::SCALE2D: return ToString(variant.scale_2d);
		case TransformPrimitive::SCALE3D: return ToString(variant.scale_3d);
		case TransformPrimitive::ROTATEX: return ToString(variant.rotate_x);
		case TransformPrimitive::ROTATEY: return ToString(variant.rotate_y);
		case TransformPrimitive::ROTATEZ: return ToString(variant.rotate_z);
		case TransformPrimitive::ROTATE2D: return ToString(variant.rotate_2d);
		case TransformPrimitive::ROTATE3D: return ToString(variant.rotate_3d);
		case TransformPrimitive::SKEWX: return ToString(variant.skew_x);
		case TransformPrimitive::SKEWY: return ToString(variant.skew_y);
		case TransformPrimitive::SKEW2D: return ToString(variant.skew_2d);
		case TransformPrimitive::PERSPECTIVE: return ToString(variant.perspective);
		case TransformPrimitive::DECOMPOSEDMATRIX4: return ToString(variant.decomposed_matrix_4);
		}
		RMLUI_ASSERT(false);
		return String();
	}
};

String TransformUtilities::ToString(const TransformPrimitive& p) noexcept
{
	String result = ToStringVisitor{}.run(p);
	return result;
}

bool TransformUtilities::Decompose(Transforms::DecomposedMatrix4& d, const Matrix4f& m) noexcept
{
	// Follows the procedure given in https://drafts.csswg.org/css-transforms-2/#interpolation-of-3d-matrices

	const float eps = 0.0005f;

	if (Math::Absolute(m[3][3]) < eps)
		return false;

	// Perspective matrix
	Matrix4f p = m;

	for (int i = 0; i < 3; i++)
		p[i][3] = 0;
	p[3][3] = 1;

	if (Math::Absolute(p.Determinant()) < eps)
		return false;

	if (m[0][3] != 0 || m[1][3] != 0 || m[2][3] != 0)
	{
		auto rhs = m.GetColumn(3);
		Matrix4f p_inv = p;
		if (!p_inv.Invert())
			return false;
		auto& p_inv_trans = p.Transpose();
		d.perspective = p_inv_trans * rhs;
	}
	else
	{
		d.perspective[0] = d.perspective[1] = d.perspective[2] = 0;
		d.perspective[3] = 1;
	}

	for (int i = 0; i < 3; i++)
		d.translation[i] = m[3][i];

	Vector3f row[3];
	for (int i = 0; i < 3; i++)
	{
		row[i][0] = m[i][0];
		row[i][1] = m[i][1];
		row[i][2] = m[i][2];
	}

	d.scale[0] = row[0].Magnitude();
	row[0] = row[0].Normalise();

	d.skew[0] = row[0].DotProduct(row[1]);
	row[1] = Combine(row[1], row[0], 1, -d.skew[0]);

	d.scale[1] = row[1].Magnitude();
	row[1] = row[1].Normalise();
	d.skew[0] /= d.scale[1];

	d.skew[1] = row[0].DotProduct(row[2]);
	row[2] = Combine(row[2], row[0], 1, -d.skew[1]);
	d.skew[2] = row[1].DotProduct(row[2]);
	row[2] = Combine(row[2], row[1], 1, -d.skew[2]);

	d.scale[2] = row[2].Magnitude();
	row[2] = row[2].Normalise();
	d.skew[2] /= d.scale[2];
	d.skew[1] /= d.scale[2];

	// Check if we need to flip coordinate system
	auto pdum3 = row[1].CrossProduct(row[2]);
	if (row[0].DotProduct(pdum3) < 0.0f)
	{
		for (int i = 0; i < 3; i++)
		{
			d.scale[i] *= -1.f;
			row[i] *= -1.f;
		}
	}

	d.quaternion[0] = 0.5f * Math::SquareRoot(Math::Max(1.f + row[0][0] - row[1][1] - row[2][2], 0.0f));
	d.quaternion[1] = 0.5f * Math::SquareRoot(Math::Max(1.f - row[0][0] + row[1][1] - row[2][2], 0.0f));
	d.quaternion[2] = 0.5f * Math::SquareRoot(Math::Max(1.f - row[0][0] - row[1][1] + row[2][2], 0.0f));
	d.quaternion[3] = 0.5f * Math::SquareRoot(Math::Max(1.f + row[0][0] + row[1][1] + row[2][2], 0.0f));

	if (row[2][1] > row[1][2])
		d.quaternion[0] *= -1.f;
	if (row[0][2] > row[2][0])
		d.quaternion[1] *= -1.f;
	if (row[1][0] > row[0][1])
		d.quaternion[2] *= -1.f;

	return true;
}

} // namespace Rml
