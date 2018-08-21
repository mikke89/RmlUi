/*
 * This source file is part of libRocket, the HTML/CSS Interface Middleware
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

#include "precompiled.h"
#include "../../Include/Rocket/Core/TransformPrimitive.h"
#include <iostream>
#include <unordered_map>

namespace Rocket {
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

float NumericValue::Resolve(Element& e, float base) const noexcept
{
	Property prop;
	prop.value = Variant(number);
	prop.unit = unit;
	return e.ResolveProperty(&prop, base);
}

float NumericValue::ResolveWidth(Element& e) const noexcept
{
	if(unit & (Property::PX | Property::NUMBER)) return number;
	return Resolve(e, e.GetBox().GetSize().x);
}

float NumericValue::ResolveHeight(Element& e) const noexcept
{
	if (unit & (Property::PX | Property::NUMBER)) return number;
	return Resolve(e, e.GetBox().GetSize().y);
}

float NumericValue::ResolveDepth(Element& e) const noexcept
{
	if (unit & (Property::PX | Property::NUMBER)) return number;
	Vector2f size = e.GetBox().GetSize();
	return Resolve(e, Math::Max(size.x, size.y));
}

float NumericValue::ResolveAbsoluteUnit(Property::Unit base_unit) const noexcept
{
	switch (base_unit)
	{
	case Property::RAD:
	{
		switch (unit)
		{
		case Property::NUMBER:
		case Property::DEG:
			return Math::DegreesToRadians(number);
		case Property::RAD:
			return number;
		case Property::PERCENT:
			return number * 0.01f * 2.0f * Math::ROCKET_PI;
			break;
		}
	}
	}
	return number;
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
		m = Matrix4f::FromRows(
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
		return false;
	}

};





bool Primitive::ResolveTransform(Matrix4f & m, Element & e) const noexcept
{
	ResolveTransformVisitor visitor{ m, e };

	bool result = std::visit(visitor, primitive);

	return result;
}

bool Primitive::ResolvePerspective(float & p, Element & e) const noexcept
{
	bool result = false;

	if (const Perspective* perspective = std::get_if<Perspective>(&primitive))
	{
		p = perspective->values[0].ResolveDepth(e);
		result = true;
	}

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
};


void Primitive::SetIdentity() noexcept
{
	std::visit(SetIdentityVisitor{}, primitive);
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
		// Rotate3D must be resolved to a full matrix for interpolation. 
		// There is an exception in CSS specs when the two interpolating rotation vectors are in the same direction, but for simplicity we ignore this optimization.
		return false;
	}
	bool operator()(Matrix3D& p)
	{
		// Matrices must be decomposed for interpolatino
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
};


bool Primitive::PrepareForInterpolation(Element & e) noexcept
{
	return std::visit(PrepareVisitor{ e }, primitive);
}



enum class GenericType { None, Scale3D, Translate3D };

struct GetGenericTypeVisitor
{
	GenericType common_type = GenericType::None;

	GenericType operator()(const TranslateX& p) { return GenericType::Translate3D; }
	GenericType operator()(const TranslateY& p) { return GenericType::Translate3D; }
	GenericType operator()(const TranslateZ& p) { return GenericType::Translate3D; }
	GenericType operator()(const Translate2D& p) { return GenericType::Translate3D; }
	GenericType operator()(const ScaleX& p) { return GenericType::Scale3D; }
	GenericType operator()(const ScaleY& p) { return GenericType::Scale3D; }
	GenericType operator()(const ScaleZ& p) { return GenericType::Scale3D; }
	GenericType operator()(const Scale2D& p) { return GenericType::Scale3D; }

	template <typename T>
	GenericType operator()(const T& p) { return GenericType::None; }
};


struct ConvertToGenericTypeVisitor
{
	PrimitiveVariant operator()(const TranslateX& p) { return Translate3D{ p.values[0], {0.0f, Property::PX}, {0.0f, Property::PX} }; }
	PrimitiveVariant operator()(const TranslateY& p) { return Translate3D{ {0.0f, Property::PX}, p.values[0], {0.0f, Property::PX} }; }
	PrimitiveVariant operator()(const TranslateZ& p) { return Translate3D{ {0.0f, Property::PX}, {0.0f, Property::PX}, p.values[0] }; }
	PrimitiveVariant operator()(const Translate2D& p) { return Translate3D{ p.values[0], p.values[1], {0.0f, Property::PX} }; }
	PrimitiveVariant operator()(const ScaleX& p) { return Scale3D{ p.values[0], 1.0f, 1.0f }; }
	PrimitiveVariant operator()(const ScaleY& p) { return Scale3D{  1.0f, p.values[0], 1.0f }; }
	PrimitiveVariant operator()(const ScaleZ& p) { return Scale3D{  1.0f, 1.0f, p.values[0] }; }
	PrimitiveVariant operator()(const Scale2D& p) { return Scale3D{ p.values[0], p.values[1], 1.0f }; }

	template <typename T>
	PrimitiveVariant operator()(const T& p) { ROCKET_ERROR; return p; }
};



bool Primitive::TryConvertToMatchingGenericType(Primitive & p0, Primitive & p1) noexcept
{
	if (p0.primitive.index() == p1.primitive.index())
		return true;

	GenericType c0 = std::visit(GetGenericTypeVisitor{}, p0.primitive);
	GenericType c1 = std::visit(GetGenericTypeVisitor{}, p1.primitive);

	if (c0 == c1 && c0 != GenericType::None)
	{
		p0.primitive = std::visit(ConvertToGenericTypeVisitor{}, p0.primitive);
		p1.primitive = std::visit(ConvertToGenericTypeVisitor{}, p1.primitive);
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
		// Currently, we promote Rotate3D to decomposed matrices in PrepareForInterpolation(), thus, it is an error if we get here. Make sure primitives are prepared and decomposed as necessary.
		// We may change this later by assuming that the underlying direction vectors are equivalent (else, need to do full matrix interpolation)
		// If we change this later: p0.values[3] = p0.values[3] * (1.0f - alpha) + p1.values[3] * alpha;
		return false;
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

	template <typename T>
	bool operator()(T& p0)
	{
		auto& p1 = std::get<T>(other_variant);
		return Interpolate(p0, p1);
	}
};


bool Primitive::InterpolateWith(const Primitive & other, float alpha) noexcept
{
	if (primitive.index() != other.primitive.index())
		return false;

	bool result = std::visit(InterpolateVisitor{ other.primitive, alpha }, primitive);

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
