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

#include "../../Include/RmlUi/Core/TransformPrimitive.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/TypeConverter.h"

namespace Rml {
namespace Core {
namespace Transforms {

static Vector3f Combine(const Vector3f& a, const Vector3f& b, float a_scale, float b_scale)
{
	Vector3f result;
	result.x = a_scale * a.x + b_scale * b.x;
	result.y = a_scale * a.y + b_scale * b.y;
	result.z = a_scale * a.z + b_scale * b.z;
	return result;
}


// Interpolate two quaternions a, b with weight alpha [0, 1]
static Vector4f QuaternionSlerp(const Vector4f& a, const Vector4f& b, float alpha)
{
	using namespace Math;

	const float eps = 0.9995f;

	float dot = a.DotProduct(b);
	dot = Clamp(dot, -1.f, 1.f);

	if (dot > eps)
		return a;

	float theta = ACos(dot);
	float w = Sin(alpha * theta) / SquareRoot(1.f - dot * dot);
	float a_scale = Cos(alpha*theta) - dot * w;

	Vector4f result;
	for (int i = 0; i < 4; i++)
	{
		result[i] = a[i] * a_scale + b[i] * w;
	}

	return result;
}



NumericValue::NumericValue() noexcept
	: number(), unit(Property::UNKNOWN)
{
}

NumericValue::NumericValue(float number, Property::Unit unit) noexcept
	: number(number), unit(unit)
{
}

float NumericValue::ResolveLengthPercentage(Element& e, float base) const noexcept
{
	Property prop;
	prop.value = Variant(number);
	prop.unit = unit;
	return e.ResolveNumericProperty(&prop, base);
}

float NumericValue::ResolveWidth(Element& e) const noexcept
{
	if(unit & (Property::PX | Property::NUMBER)) return number;
	return ResolveLengthPercentage(e, e.GetBox().GetSize(Box::BORDER).x);
}

float NumericValue::ResolveHeight(Element& e) const noexcept
{
	if (unit & (Property::PX | Property::NUMBER)) return number;
	return ResolveLengthPercentage(e, e.GetBox().GetSize(Box::BORDER).y);
}

float NumericValue::ResolveDepth(Element& e) const noexcept
{
	if (unit & (Property::PX | Property::NUMBER)) return number;
	Vector2f size = e.GetBox().GetSize(Box::BORDER);
	return ResolveLengthPercentage(e, Math::Max(size.x, size.y));
}

float NumericValue::ResolveAbsoluteUnit(Property::Unit base_unit) const noexcept
{
	if(base_unit == Property::RAD)
	{
		switch (unit)
		{
		case Property::NUMBER:
		case Property::DEG:
			return Math::DegreesToRadians(number);
		case Property::RAD:
			return number;
		case Property::PERCENT:
			return number * 0.01f * 2.0f * Math::RMLUI_PI;
		default:
			break;
		}
	}
	return number;
}

String NumericValue::ToString() const noexcept
{
	Property prop;
	prop.value = Variant(number);
	prop.unit = unit;
	return prop.ToString();
}










struct ResolveTransformVisitor
{
	Matrix4f& m;
	Element& e;

	bool operator()(const Matrix2D& p)
	{
		m = Matrix4f::FromRows(
			Vector4f(p.values[0], p.values[2], 0, p.values[4]),
			Vector4f(p.values[1], p.values[3], 0, p.values[5]),
			Vector4f(0, 0, 1, 0),
			Vector4f(0, 0, 0, 1)
		);
		return true;
	}

	bool operator()(const Matrix3D& p)
	{
		m = Matrix4f::FromColumns(
			Vector4f(p.values[0], p.values[1], p.values[2], p.values[3]),
			Vector4f(p.values[4], p.values[5], p.values[6], p.values[7]),
			Vector4f(p.values[8], p.values[9], p.values[10], p.values[11]),
			Vector4f(p.values[12], p.values[13], p.values[14], p.values[15])
		);
		return true;
	}

	bool operator()(const TranslateX& p)
	{
		m = Matrix4f::TranslateX(p.values[0].ResolveWidth(e));
		return true;
	}

	bool operator()(const TranslateY& p)
	{
		m = Matrix4f::TranslateY(p.values[0].ResolveHeight(e));
		return true;
	}

	bool operator()(const TranslateZ& p)
	{
		m = Matrix4f::TranslateZ(p.values[0].ResolveDepth(e));
		return true;
	}

	bool operator()(const Translate2D& p)
	{
		m = Matrix4f::Translate(
			p.values[0].ResolveWidth(e),
			p.values[1].ResolveHeight(e),
			0
		);
		return true;
	}

	bool operator()(const Translate3D& p)
	{
		m = Matrix4f::Translate(
			p.values[0].ResolveWidth(e),
			p.values[1].ResolveHeight(e),
			p.values[2].ResolveDepth(e)
		);
		return true;
	}

	bool operator()(const ScaleX& p)
	{
		m = Matrix4f::ScaleX(p.values[0]);
		return true;
	}

	bool operator()(const ScaleY& p)
	{
		m = Matrix4f::ScaleY(p.values[0]);
		return true;
	}

	bool operator()(const ScaleZ& p)
	{
		m = Matrix4f::ScaleZ(p.values[0]);
		return true;
	}

	bool operator()(const Scale2D& p)
	{
		m = Matrix4f::Scale(p.values[0], p.values[1], 1);
		return true;
	}

	bool operator()(const Scale3D& p)
	{
		m = Matrix4f::Scale(p.values[0], p.values[1], p.values[2]);
		return true;
	}

	bool operator()(const RotateX& p)
	{
		m = Matrix4f::RotateX(p.values[0]);
		return true;
	}

	bool operator()(const RotateY& p)
	{
		m = Matrix4f::RotateY(p.values[0]);
		return true;
	}

	bool operator()(const RotateZ& p)
	{
		m = Matrix4f::RotateZ(p.values[0]);
		return true;
	}

	bool operator()(const Rotate2D& p)
	{
		m = Matrix4f::RotateZ(p.values[0]);
		return true;
	}

	bool operator()(const Rotate3D& p)
	{
		m = Matrix4f::Rotate(Vector3f(p.values[0], p.values[1], p.values[2]), p.values[3]);
		return true;
	}

	bool operator()(const SkewX& p)
	{
		m = Matrix4f::SkewX(p.values[0]);
		return true;
	}

	bool operator()(const SkewY& p)
	{
		m = Matrix4f::SkewY(p.values[0]);
		return true;
	}

	bool operator()(const Skew2D& p)
	{
		m = Matrix4f::Skew(p.values[0], p.values[1]);
		return true;
	}

	bool operator()(const DecomposedMatrix4& p)
	{
		m = Matrix4f::Compose(p.translation, p.scale, p.skew, p.perspective, p.quaternion);
		return true;
	}
	bool operator()(const Perspective& p)
	{
		m = Matrix4f::Perspective(p.values[0].ResolveDepth(e));
		return true;
	}


	bool run(const PrimitiveVariant& primitive)
	{
		switch (primitive.type)
		{
		case PrimitiveVariant::MATRIX2D: return this->operator()(primitive.matrix_2d);
		case PrimitiveVariant::MATRIX3D: return this->operator()(primitive.matrix_3d);
		case PrimitiveVariant::TRANSLATEX: return this->operator()(primitive.translate_x);
		case PrimitiveVariant::TRANSLATEY: return this->operator()(primitive.translate_y);
		case PrimitiveVariant::TRANSLATEZ: return this->operator()(primitive.translate_z);
		case PrimitiveVariant::TRANSLATE2D: return this->operator()(primitive.translate_2d);
		case PrimitiveVariant::TRANSLATE3D: return this->operator()(primitive.translate_3d);
		case PrimitiveVariant::SCALEX: return this->operator()(primitive.scale_x);
		case PrimitiveVariant::SCALEY: return this->operator()(primitive.scale_y);
		case PrimitiveVariant::SCALEZ: return this->operator()(primitive.scale_z);
		case PrimitiveVariant::SCALE2D: return this->operator()(primitive.scale_2d);
		case PrimitiveVariant::SCALE3D: return this->operator()(primitive.scale_3d);
		case PrimitiveVariant::ROTATEX: return this->operator()(primitive.rotate_x);
		case PrimitiveVariant::ROTATEY: return this->operator()(primitive.rotate_y);
		case PrimitiveVariant::ROTATEZ: return this->operator()(primitive.rotate_z);
		case PrimitiveVariant::ROTATE2D: return this->operator()(primitive.rotate_2d);
		case PrimitiveVariant::ROTATE3D: return this->operator()(primitive.rotate_3d);
		case PrimitiveVariant::SKEWX: return this->operator()(primitive.skew_x);
		case PrimitiveVariant::SKEWY: return this->operator()(primitive.skew_y);
		case PrimitiveVariant::SKEW2D: return this->operator()(primitive.skew_2d);
		case PrimitiveVariant::PERSPECTIVE: return this->operator()(primitive.perspective);
		case PrimitiveVariant::DECOMPOSEDMATRIX4: return this->operator()(primitive.decomposed_matrix_4);
		default:
			break;
		}
		RMLUI_ASSERT(false);
		return false;
	}
};





bool Primitive::ResolveTransform(Matrix4f & m, Element & e) const noexcept
{
	ResolveTransformVisitor visitor{ m, e };

	bool result = visitor.run(primitive);

	return result;
}

struct SetIdentityVisitor
{
	template <size_t N>
	void operator()(ResolvedPrimitive<N>& p)
	{
		for (auto& value : p.values)
			value = 0.0f;
	}
	template <size_t N>
	void operator()(UnresolvedPrimitive<N>& p)
	{
		for (auto& value : p.values)
			value.number = 0.0f;
	}
	void operator()(Matrix2D& p)
	{
		for (int i = 0; i < 6; i++)
			p.values[i] = ((i == 0 || i == 3) ? 1.0f : 0.0f);
	}
	void operator()(Matrix3D& p)
	{
		for (int i = 0; i < 16; i++)
			p.values[i] = ((i % 5) == 0 ? 1.0f : 0.0f);
	}
	void operator()(ScaleX& p)
	{
		p.values[0] = 1;
	}
	void operator()(ScaleY& p)
	{
		p.values[0] = 1;
	}
	void operator()(ScaleZ& p)
	{
		p.values[0] = 1;
	}
	void operator()(Scale2D& p)
	{
		p.values[0] = p.values[1] = 1;
	}
	void operator()(Scale3D& p)
	{
		p.values[0] = p.values[1] = p.values[2] = 1;
	}
	void operator()(DecomposedMatrix4& p)
	{
		p.perspective = Vector4f(0, 0, 0, 1);
		p.quaternion = Vector4f(0, 0, 0, 1);
		p.translation = Vector3f(0, 0, 0);
		p.scale = Vector3f(1, 1, 1);
		p.skew = Vector3f(0, 0, 0);
	}


	void run(PrimitiveVariant& primitive)
	{
		switch (primitive.type)
		{
		case PrimitiveVariant::MATRIX2D: this->operator()(primitive.matrix_2d); break;
		case PrimitiveVariant::MATRIX3D: this->operator()(primitive.matrix_3d); break;
		case PrimitiveVariant::TRANSLATEX: this->operator()(primitive.translate_x); break;
		case PrimitiveVariant::TRANSLATEY: this->operator()(primitive.translate_y); break;
		case PrimitiveVariant::TRANSLATEZ: this->operator()(primitive.translate_z); break;
		case PrimitiveVariant::TRANSLATE2D: this->operator()(primitive.translate_2d); break;
		case PrimitiveVariant::TRANSLATE3D: this->operator()(primitive.translate_3d); break;
		case PrimitiveVariant::SCALEX: this->operator()(primitive.scale_x); break;
		case PrimitiveVariant::SCALEY: this->operator()(primitive.scale_y); break;
		case PrimitiveVariant::SCALEZ: this->operator()(primitive.scale_z); break;
		case PrimitiveVariant::SCALE2D: this->operator()(primitive.scale_2d); break;
		case PrimitiveVariant::SCALE3D: this->operator()(primitive.scale_3d); break;
		case PrimitiveVariant::ROTATEX: this->operator()(primitive.rotate_x); break;
		case PrimitiveVariant::ROTATEY: this->operator()(primitive.rotate_y); break;
		case PrimitiveVariant::ROTATEZ: this->operator()(primitive.rotate_z); break;
		case PrimitiveVariant::ROTATE2D: this->operator()(primitive.rotate_2d); break;
		case PrimitiveVariant::ROTATE3D: this->operator()(primitive.rotate_3d); break;
		case PrimitiveVariant::SKEWX: this->operator()(primitive.skew_x); break;
		case PrimitiveVariant::SKEWY: this->operator()(primitive.skew_y); break;
		case PrimitiveVariant::SKEW2D: this->operator()(primitive.skew_2d); break;
		case PrimitiveVariant::PERSPECTIVE: this->operator()(primitive.perspective); break;
		case PrimitiveVariant::DECOMPOSEDMATRIX4: this->operator()(primitive.decomposed_matrix_4); break;
		default:
			RMLUI_ASSERT(false);
			break;
		}
	}
};

void Primitive::SetIdentity() noexcept
{
	SetIdentityVisitor{}.run(primitive);
}



struct PrepareVisitor
{
	Element& e;

	bool operator()(TranslateX& p)
	{
		p.values[0] = NumericValue{ p.values[0].ResolveWidth(e), Property::PX };
		return true;
	}
	bool operator()(TranslateY& p)
	{
		p.values[0] = NumericValue{ p.values[0].ResolveHeight(e), Property::PX };
		return true;
	}
	bool operator()(TranslateZ& p)
	{
		p.values[0] = NumericValue{ p.values[0].ResolveDepth(e), Property::PX };
		return true;
	}
	bool operator()(Translate2D& p)
	{
		p.values[0] = NumericValue{ p.values[0].ResolveWidth(e), Property::PX };
		p.values[1] = NumericValue{ p.values[1].ResolveHeight(e), Property::PX };
		return true;
	}
	bool operator()(Translate3D& p)
	{
		p.values[0] = NumericValue{ p.values[0].ResolveWidth(e), Property::PX };
		p.values[1] = NumericValue{ p.values[1].ResolveHeight(e), Property::PX };
		p.values[2] = NumericValue{ p.values[2].ResolveDepth(e), Property::PX };
		return true;
	}
	template <size_t N>
	bool operator()(ResolvedPrimitive<N>& p)
	{
		// No conversion needed for resolved transforms (with some exceptions below)
		return true;
	}
	bool operator()(DecomposedMatrix4& p)
	{
		return true;
	}
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
	bool operator()(Matrix3D& p)
	{
		// Matrices must be decomposed for interpolation
		return false;
	}
	bool operator()(Matrix2D& p)
	{
		// Matrix2D can also be optimized for interpolation, but for now we decompose it to a full DecomposedMatrix4
		return false;
	}
	bool operator()(Perspective& p)
	{
		// Perspective must be decomposed
		return false;
	}

	bool run(PrimitiveVariant& primitive)
	{
		switch (primitive.type)
		{
		case PrimitiveVariant::MATRIX2D: return this->operator()(primitive.matrix_2d);
		case PrimitiveVariant::MATRIX3D: return this->operator()(primitive.matrix_3d);
		case PrimitiveVariant::TRANSLATEX: return this->operator()(primitive.translate_x);
		case PrimitiveVariant::TRANSLATEY: return this->operator()(primitive.translate_y);
		case PrimitiveVariant::TRANSLATEZ: return this->operator()(primitive.translate_z);
		case PrimitiveVariant::TRANSLATE2D: return this->operator()(primitive.translate_2d);
		case PrimitiveVariant::TRANSLATE3D: return this->operator()(primitive.translate_3d);
		case PrimitiveVariant::SCALEX: return this->operator()(primitive.scale_x);
		case PrimitiveVariant::SCALEY: return this->operator()(primitive.scale_y);
		case PrimitiveVariant::SCALEZ: return this->operator()(primitive.scale_z);
		case PrimitiveVariant::SCALE2D: return this->operator()(primitive.scale_2d);
		case PrimitiveVariant::SCALE3D: return this->operator()(primitive.scale_3d);
		case PrimitiveVariant::ROTATEX: return this->operator()(primitive.rotate_x);
		case PrimitiveVariant::ROTATEY: return this->operator()(primitive.rotate_y);
		case PrimitiveVariant::ROTATEZ: return this->operator()(primitive.rotate_z);
		case PrimitiveVariant::ROTATE2D: return this->operator()(primitive.rotate_2d);
		case PrimitiveVariant::ROTATE3D: return this->operator()(primitive.rotate_3d);
		case PrimitiveVariant::SKEWX: return this->operator()(primitive.skew_x);
		case PrimitiveVariant::SKEWY: return this->operator()(primitive.skew_y);
		case PrimitiveVariant::SKEW2D: return this->operator()(primitive.skew_2d);
		case PrimitiveVariant::PERSPECTIVE: return this->operator()(primitive.perspective);
		case PrimitiveVariant::DECOMPOSEDMATRIX4: return this->operator()(primitive.decomposed_matrix_4);
		default:
			break;
		}
		RMLUI_ASSERT(false);
		return false;
	}
};


bool Primitive::PrepareForInterpolation(Element & e) noexcept
{
	return PrepareVisitor{ e }.run(primitive);
}



enum class GenericType { None, Scale3D, Translate3D, Rotate3D };

struct GetGenericTypeVisitor
{
	GenericType run(const PrimitiveVariant& primitive)
	{
		switch (primitive.type)
		{
		case PrimitiveVariant::TRANSLATEX:  return GenericType::Translate3D;
		case PrimitiveVariant::TRANSLATEY:  return GenericType::Translate3D;
		case PrimitiveVariant::TRANSLATEZ:  return GenericType::Translate3D;
		case PrimitiveVariant::TRANSLATE2D: return GenericType::Translate3D;
		case PrimitiveVariant::TRANSLATE3D: return GenericType::Translate3D;
		case PrimitiveVariant::SCALEX:      return GenericType::Scale3D;
		case PrimitiveVariant::SCALEY:      return GenericType::Scale3D;
		case PrimitiveVariant::SCALEZ:      return GenericType::Scale3D;
		case PrimitiveVariant::SCALE2D:     return GenericType::Scale3D;
		case PrimitiveVariant::SCALE3D:     return GenericType::Scale3D;
		case PrimitiveVariant::ROTATEX:     return GenericType::Rotate3D;
		case PrimitiveVariant::ROTATEY:     return GenericType::Rotate3D;
		case PrimitiveVariant::ROTATEZ:     return GenericType::Rotate3D;
		case PrimitiveVariant::ROTATE2D:    return GenericType::Rotate3D;
		case PrimitiveVariant::ROTATE3D:    return GenericType::Rotate3D;
		default:
			break;
		}
		return GenericType::None;
	}
};


struct ConvertToGenericTypeVisitor
{
	Translate3D operator()(const TranslateX& p) { return Translate3D{ p.values[0], {0.0f, Property::PX}, {0.0f, Property::PX} }; }
	Translate3D operator()(const TranslateY& p) { return Translate3D{ {0.0f, Property::PX}, p.values[0], {0.0f, Property::PX} }; }
	Translate3D operator()(const TranslateZ& p) { return Translate3D{ {0.0f, Property::PX}, {0.0f, Property::PX}, p.values[0] }; }
	Translate3D operator()(const Translate2D& p) { return Translate3D{ p.values[0], p.values[1], {0.0f, Property::PX} }; }
	Scale3D operator()(const ScaleX& p) { return Scale3D{ p.values[0], 1.0f, 1.0f }; }
	Scale3D operator()(const ScaleY& p) { return Scale3D{  1.0f, p.values[0], 1.0f }; }
	Scale3D operator()(const ScaleZ& p) { return Scale3D{  1.0f, 1.0f, p.values[0] }; }
	Scale3D operator()(const Scale2D& p) { return Scale3D{ p.values[0], p.values[1], 1.0f }; }
	Rotate3D operator()(const RotateX& p)  { return Rotate3D{ 1, 0, 0, p.values[0], Property::RAD }; }
	Rotate3D operator()(const RotateY& p)  { return Rotate3D{ 0, 1, 0, p.values[0], Property::RAD }; }
	Rotate3D operator()(const RotateZ& p)  { return Rotate3D{ 0, 0, 1, p.values[0], Property::RAD }; }
	Rotate3D operator()(const Rotate2D& p) { return Rotate3D{ 0, 0, 1, p.values[0], Property::RAD }; }

	template <typename T>
	PrimitiveVariant operator()(const T& p) { RMLUI_ERROR; return p; }

	PrimitiveVariant run(const PrimitiveVariant& primitive)
	{
		PrimitiveVariant result = primitive;
		switch (primitive.type)
		{
		case PrimitiveVariant::TRANSLATEX:  result.type = PrimitiveVariant::TRANSLATE3D; result.translate_3d = this->operator()(primitive.translate_x);  break;
		case PrimitiveVariant::TRANSLATEY:  result.type = PrimitiveVariant::TRANSLATE3D; result.translate_3d = this->operator()(primitive.translate_y);  break;
		case PrimitiveVariant::TRANSLATEZ:  result.type = PrimitiveVariant::TRANSLATE3D; result.translate_3d = this->operator()(primitive.translate_z);  break;
		case PrimitiveVariant::TRANSLATE2D: result.type = PrimitiveVariant::TRANSLATE3D; result.translate_3d = this->operator()(primitive.translate_2d); break;
		case PrimitiveVariant::TRANSLATE3D: break;
		case PrimitiveVariant::SCALEX:      result.type = PrimitiveVariant::SCALE3D;     result.scale_3d     = this->operator()(primitive.scale_x);      break;
		case PrimitiveVariant::SCALEY:      result.type = PrimitiveVariant::SCALE3D;     result.scale_3d     = this->operator()(primitive.scale_y);      break;
		case PrimitiveVariant::SCALEZ:      result.type = PrimitiveVariant::SCALE3D;     result.scale_3d     = this->operator()(primitive.scale_z);      break;
		case PrimitiveVariant::SCALE2D:     result.type = PrimitiveVariant::SCALE3D;     result.scale_3d     = this->operator()(primitive.scale_2d);     break;
		case PrimitiveVariant::SCALE3D:     break;
		case PrimitiveVariant::ROTATEX:     result.type = PrimitiveVariant::ROTATE3D;    result.rotate_3d    = this->operator()(primitive.rotate_x);     break;
		case PrimitiveVariant::ROTATEY:     result.type = PrimitiveVariant::ROTATE3D;    result.rotate_3d    = this->operator()(primitive.rotate_y);     break;
		case PrimitiveVariant::ROTATEZ:     result.type = PrimitiveVariant::ROTATE3D;    result.rotate_3d    = this->operator()(primitive.rotate_z);     break;
		case PrimitiveVariant::ROTATE2D:    result.type = PrimitiveVariant::ROTATE3D;    result.rotate_3d    = this->operator()(primitive.rotate_2d);    break;
		case PrimitiveVariant::ROTATE3D:    break;
		default:
			RMLUI_ASSERT(false);
			break;
		}
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


bool Primitive::TryConvertToMatchingGenericType(Primitive & p0, Primitive & p1) noexcept
{
	if (p0.primitive.type == p1.primitive.type)
	{
		if(p0.primitive.type == PrimitiveVariant::ROTATE3D && !CanInterpolateRotate3D(p0.primitive.rotate_3d, p1.primitive.rotate_3d))
			return false;

		return true;
	}

	GenericType c0 = GetGenericTypeVisitor{}.run(p0.primitive);
	GenericType c1 = GetGenericTypeVisitor{}.run(p1.primitive);

	if (c0 == c1 && c0 != GenericType::None)
	{
		PrimitiveVariant new_p0 = ConvertToGenericTypeVisitor{}.run(p0.primitive);
		PrimitiveVariant new_p1 = ConvertToGenericTypeVisitor{}.run(p1.primitive);
		
		RMLUI_ASSERT(new_p0.type == new_p1.type);
		
		if (new_p0.type == PrimitiveVariant::ROTATE3D && !CanInterpolateRotate3D(new_p0.rotate_3d, new_p1.rotate_3d))
			return false;

		p0.primitive = new_p0;
		p1.primitive = new_p1;

		return true;
	}

	return false;
}



struct InterpolateVisitor
{
	const PrimitiveVariant& other_variant;
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
			p0.values[i].number = p0.values[i].number*(1.0f - alpha) + p1.values[i].number * alpha;
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
	bool Interpolate(Matrix2D& p0, const Matrix2D& p1) { return false; /* Error if we get here, see PrepareForInterpolation() */ }
	bool Interpolate(Matrix3D& p0, const Matrix3D& p1) { return false; /* Error if we get here, see PrepareForInterpolation() */ }
	bool Interpolate(Perspective& p0, const Perspective& p1) { return false; /* Error if we get here, see PrepareForInterpolation() */ }

	bool Interpolate(DecomposedMatrix4& p0, const DecomposedMatrix4& p1)
	{
		p0.perspective = p0.perspective * (1.0f - alpha) + p1.perspective * alpha;
		p0.quaternion = QuaternionSlerp(p0.quaternion, p1.quaternion, alpha);
		p0.translation = p0.translation * (1.0f - alpha) + p1.translation * alpha;
		p0.scale = p0.scale* (1.0f - alpha) + p1.scale* alpha;
		p0.skew = p0.skew* (1.0f - alpha) + p1.skew* alpha;
		return true;
	}

	bool run(PrimitiveVariant& variant)
	{
		RMLUI_ASSERT(variant.type == other_variant.type);
		switch (variant.type)
		{
		case PrimitiveVariant::MATRIX2D: return Interpolate(variant.matrix_2d, other_variant.matrix_2d);
		case PrimitiveVariant::MATRIX3D: return Interpolate(variant.matrix_3d, other_variant.matrix_3d);
		case PrimitiveVariant::TRANSLATEX: return Interpolate(variant.translate_x, other_variant.translate_x);
		case PrimitiveVariant::TRANSLATEY: return Interpolate(variant.translate_y, other_variant.translate_y);
		case PrimitiveVariant::TRANSLATEZ: return Interpolate(variant.translate_z, other_variant.translate_z);
		case PrimitiveVariant::TRANSLATE2D: return Interpolate(variant.translate_2d, other_variant.translate_2d);
		case PrimitiveVariant::TRANSLATE3D: return Interpolate(variant.translate_3d, other_variant.translate_3d);
		case PrimitiveVariant::SCALEX: return Interpolate(variant.scale_x, other_variant.scale_x);
		case PrimitiveVariant::SCALEY: return Interpolate(variant.scale_y, other_variant.scale_y);
		case PrimitiveVariant::SCALEZ: return Interpolate(variant.scale_z, other_variant.scale_z);
		case PrimitiveVariant::SCALE2D: return Interpolate(variant.scale_2d, other_variant.scale_2d);
		case PrimitiveVariant::SCALE3D: return Interpolate(variant.scale_3d, other_variant.scale_3d);
		case PrimitiveVariant::ROTATEX: return Interpolate(variant.rotate_x, other_variant.rotate_x);
		case PrimitiveVariant::ROTATEY: return Interpolate(variant.rotate_y, other_variant.rotate_y);
		case PrimitiveVariant::ROTATEZ: return Interpolate(variant.rotate_z, other_variant.rotate_z);
		case PrimitiveVariant::ROTATE2D: return Interpolate(variant.rotate_2d, other_variant.rotate_2d);
		case PrimitiveVariant::ROTATE3D: return Interpolate(variant.rotate_3d, other_variant.rotate_3d);
		case PrimitiveVariant::SKEWX: return Interpolate(variant.skew_x, other_variant.skew_x);
		case PrimitiveVariant::SKEWY: return Interpolate(variant.skew_y, other_variant.skew_y);
		case PrimitiveVariant::SKEW2D: return Interpolate(variant.skew_2d, other_variant.skew_2d);
		case PrimitiveVariant::PERSPECTIVE: return Interpolate(variant.perspective, other_variant.perspective);
		case PrimitiveVariant::DECOMPOSEDMATRIX4: return Interpolate(variant.decomposed_matrix_4, other_variant.decomposed_matrix_4);
		default:
			break;
		}
		RMLUI_ASSERT(false);
		return false;
	}
};


bool Primitive::InterpolateWith(const Primitive & other, float alpha) noexcept
{
	if (primitive.type != other.primitive.type)
		return false;

	bool result = InterpolateVisitor{ other.primitive, alpha }.run(primitive);

	return result;
}





template<size_t N>
static inline String ToString(const Transforms::ResolvedPrimitive<N>& p, String unit, bool rad_to_deg = false, bool only_unit_on_last_value = false) noexcept {
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

template<size_t N>
static inline String ToString(const Transforms::UnresolvedPrimitive<N> & p) noexcept {
	String result = "(";
	for (size_t i = 0; i < N; i++) 
	{
		result += p.values[i].ToString();
		if (i != N - 1) 
			result += ", ";
	}
	result += ")";
	return result;
}

static inline String ToString(const Transforms::DecomposedMatrix4& p) noexcept {
	static const DecomposedMatrix4 d{
		Vector4f(0, 0, 0, 1),
		Vector4f(0, 0, 0, 1),
		Vector3f(0, 0, 0),
		Vector3f(1, 1, 1),
		Vector3f(0, 0, 0)
	}; 
	String tmp;
	String result;
	
	if(p.perspective != d.perspective && TypeConverter< Vector4f, String >::Convert(p.perspective, tmp))
		result += "perspective(" + tmp + "), ";
	if (p.quaternion != d.quaternion && TypeConverter< Vector4f, String >::Convert(p.quaternion, tmp))
		result += "quaternion(" + tmp + "), ";
	if (p.translation != d.translation && TypeConverter< Vector3f, String >::Convert(p.translation, tmp))
		result += "translation(" + tmp + "), ";
	if (p.scale != d.scale && TypeConverter< Vector3f, String >::Convert(p.scale, tmp))
		result += "scale(" + tmp + "), ";
	if (p.skew != d.skew && TypeConverter< Vector3f, String >::Convert(p.skew, tmp))
		result += "skew(" + tmp + "), ";

	if (result.size() > 2)
		result.resize(result.size() - 2);

	result = "decomposedMatrix3d{ " + result + " }";

	return result;
}


String ToString(const Transforms::Matrix2D & p) noexcept { return "matrix" + ToString(static_cast<const Transforms::ResolvedPrimitive< 6 >&>(p), ""); }
String ToString(const Transforms::Matrix3D & p) noexcept { return "matrix3d" + ToString(static_cast<const Transforms::ResolvedPrimitive< 16 >&>(p), ""); }
String ToString(const Transforms::TranslateX & p) noexcept { return "translateX" + ToString(static_cast<const Transforms::UnresolvedPrimitive< 1 >&>(p)); }
String ToString(const Transforms::TranslateY & p) noexcept { return "translateY" + ToString(static_cast<const Transforms::UnresolvedPrimitive< 1 >&>(p)); }
String ToString(const Transforms::TranslateZ & p) noexcept { return "translateZ" + ToString(static_cast<const Transforms::UnresolvedPrimitive< 1 >&>(p)); }
String ToString(const Transforms::Translate2D & p) noexcept { return "translate" + ToString(static_cast<const Transforms::UnresolvedPrimitive< 2 >&>(p)); }
String ToString(const Transforms::Translate3D & p) noexcept { return "translate3d" + ToString(static_cast<const Transforms::UnresolvedPrimitive< 3 >&>(p)); }
String ToString(const Transforms::ScaleX & p) noexcept { return "scaleX" + ToString(static_cast<const Transforms::ResolvedPrimitive< 1 >&>(p), ""); }
String ToString(const Transforms::ScaleY & p) noexcept { return "scaleY" + ToString(static_cast<const Transforms::ResolvedPrimitive< 1 >&>(p), ""); }
String ToString(const Transforms::ScaleZ & p) noexcept { return "scaleZ" + ToString(static_cast<const Transforms::ResolvedPrimitive< 1 >&>(p), ""); }
String ToString(const Transforms::Scale2D & p) noexcept { return "scale" + ToString(static_cast<const Transforms::ResolvedPrimitive< 2 >&>(p), ""); }
String ToString(const Transforms::Scale3D & p) noexcept { return "scale3d" + ToString(static_cast<const Transforms::ResolvedPrimitive< 3 >&>(p), ""); }
String ToString(const Transforms::RotateX & p) noexcept { return "rotateX" + ToString(static_cast<const Transforms::ResolvedPrimitive< 1 >&>(p), "deg", true); }
String ToString(const Transforms::RotateY & p) noexcept { return "rotateY" + ToString(static_cast<const Transforms::ResolvedPrimitive< 1 >&>(p), "deg", true); }
String ToString(const Transforms::RotateZ & p) noexcept { return "rotateZ" + ToString(static_cast<const Transforms::ResolvedPrimitive< 1 >&>(p), "deg", true); }
String ToString(const Transforms::Rotate2D & p) noexcept { return "rotate" + ToString(static_cast<const Transforms::ResolvedPrimitive< 1 >&>(p), "deg", true); }
String ToString(const Transforms::Rotate3D & p) noexcept { return "rotate3d" + ToString(static_cast<const Transforms::ResolvedPrimitive< 4 >&>(p), "deg", true, true); }
String ToString(const Transforms::SkewX & p) noexcept { return "skewX" + ToString(static_cast<const Transforms::ResolvedPrimitive< 1 >&>(p), "deg", true); }
String ToString(const Transforms::SkewY & p) noexcept { return "skewY" + ToString(static_cast<const Transforms::ResolvedPrimitive< 1 >&>(p), "deg", true); }
String ToString(const Transforms::Skew2D & p) noexcept { return "skew" + ToString(static_cast<const Transforms::ResolvedPrimitive< 2 >&>(p), "deg", true); }
String ToString(const Transforms::Perspective & p) noexcept { return "perspective" + ToString(static_cast<const Transforms::UnresolvedPrimitive< 1 >&>(p)); }


struct ToStringVisitor
{
	String run(const PrimitiveVariant& variant)
	{
		switch (variant.type)
		{
		case PrimitiveVariant::MATRIX2D: return ToString(variant.matrix_2d);
		case PrimitiveVariant::MATRIX3D: return ToString(variant.matrix_3d);
		case PrimitiveVariant::TRANSLATEX: return ToString(variant.translate_x);
		case PrimitiveVariant::TRANSLATEY: return ToString(variant.translate_y);
		case PrimitiveVariant::TRANSLATEZ: return ToString(variant.translate_z);
		case PrimitiveVariant::TRANSLATE2D: return ToString(variant.translate_2d);
		case PrimitiveVariant::TRANSLATE3D: return ToString(variant.translate_3d);
		case PrimitiveVariant::SCALEX: return ToString(variant.scale_x);
		case PrimitiveVariant::SCALEY: return ToString(variant.scale_y);
		case PrimitiveVariant::SCALEZ: return ToString(variant.scale_z);
		case PrimitiveVariant::SCALE2D: return ToString(variant.scale_2d);
		case PrimitiveVariant::SCALE3D: return ToString(variant.scale_3d);
		case PrimitiveVariant::ROTATEX: return ToString(variant.rotate_x);
		case PrimitiveVariant::ROTATEY: return ToString(variant.rotate_y);
		case PrimitiveVariant::ROTATEZ: return ToString(variant.rotate_z);
		case PrimitiveVariant::ROTATE2D: return ToString(variant.rotate_2d);
		case PrimitiveVariant::ROTATE3D: return ToString(variant.rotate_3d);
		case PrimitiveVariant::SKEWX: return ToString(variant.skew_x);
		case PrimitiveVariant::SKEWY: return ToString(variant.skew_y);
		case PrimitiveVariant::SKEW2D: return ToString(variant.skew_2d);
		case PrimitiveVariant::PERSPECTIVE: return ToString(variant.perspective);
		case PrimitiveVariant::DECOMPOSEDMATRIX4: return ToString(variant.decomposed_matrix_4);
		default:
			break;
		}
		RMLUI_ASSERT(false);
		return String();
	}
};

String Primitive::ToString() const noexcept
{
	String result = ToStringVisitor{}.run(primitive);
	return result;
}


bool DecomposedMatrix4::Decompose(const Matrix4f & m)
{
	// Follows the procedure given in https://drafts.csswg.org/css-transforms-2/#interpolation-of-3d-matrices

	const float eps = 0.0005f;

	if (Math::AbsoluteValue(m[3][3]) < eps)
		return false;


	// Perspective matrix
	Matrix4f p = m;

	for (int i = 0; i < 3; i++)
		p[i][3] = 0;
	p[3][3] = 1;

	if (Math::AbsoluteValue(p.Determinant()) < eps)
		return false;

	if (m[0][3] != 0 || m[1][3] != 0 || m[2][3] != 0)
	{
		auto rhs = m.GetColumn(3);
		Matrix4f p_inv = p;
		if (!p_inv.Invert())
			return false;
		auto& p_inv_trans = p.Transpose();
		perspective = p_inv_trans * rhs;
	}
	else
	{
		perspective[0] = perspective[1] = perspective[2] = 0;
		perspective[3] = 1;
	}

	for (int i = 0; i < 3; i++)
		translation[i] = m[3][i];

	Vector3f row[3];
	for (int i = 0; i < 3; i++)
	{
		row[i][0] = m[i][0];
		row[i][1] = m[i][1];
		row[i][2] = m[i][2];
	}

	scale[0] = row[0].Magnitude();
	row[0] = row[0].Normalise();

	skew[0] = row[0].DotProduct(row[1]);
	row[1] = Combine(row[1], row[0], 1, -skew[0]);

	scale[1] = row[1].Magnitude();
	row[1] = row[1].Normalise();
	skew[0] /= scale[1];

	skew[1] = row[0].DotProduct(row[2]);
	row[2] = Combine(row[2], row[0], 1, -skew[1]);
	skew[2] = row[1].DotProduct(row[2]);
	row[2] = Combine(row[2], row[1], 1, -skew[2]);

	scale[2] = row[2].Magnitude();
	row[2] = row[2].Normalise();
	skew[1] /= scale[2];
	skew[2] /= scale[2];

	// Check if we need to flip coordinate system
	auto pdum3 = row[1].CrossProduct(row[2]);
	if (row[0].DotProduct(pdum3) < 0.0f)
	{
		for (int i = 0; i < 3; i++)
		{
			scale[i] *= -1.f;
			row[i] *= -1.f;
		}
	}

	quaternion[0] = 0.5f * Math::SquareRoot(Math::Max(1.f + row[0][0] - row[1][1] - row[2][2], 0.0f));
	quaternion[1] = 0.5f * Math::SquareRoot(Math::Max(1.f - row[0][0] + row[1][1] - row[2][2], 0.0f));
	quaternion[2] = 0.5f * Math::SquareRoot(Math::Max(1.f - row[0][0] - row[1][1] + row[2][2], 0.0f));
	quaternion[3] = 0.5f * Math::SquareRoot(Math::Max(1.f + row[0][0] + row[1][1] + row[2][2], 0.0f));

	if (row[2][1] > row[1][2])
		quaternion[0] *= -1.f;
	if (row[0][2] > row[2][0])
		quaternion[1] *= -1.f;
	if (row[1][0] > row[0][1])
		quaternion[2] *= -1.f;

	return true;
}

}
}
}
