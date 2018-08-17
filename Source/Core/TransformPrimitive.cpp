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

namespace Rocket {
namespace Core {
namespace Transforms {

NumericValue::NumericValue() throw()
	: number(), unit(Property::UNKNOWN)
{
}

NumericValue::NumericValue(float number, Property::Unit unit) throw()
	: number(number), unit(unit)
{
}

float NumericValue::Resolve(Element& e, float base) const throw()
{
	Property prop;
	prop.value = Variant(number);
	prop.unit = unit;
	return e.ResolveProperty(&prop, base);
}

float NumericValue::ResolveWidth(Element& e) const throw()
{
	return Resolve(e, e.GetBox().GetSize().x);
}

float NumericValue::ResolveHeight(Element& e) const throw()
{
	return Resolve(e, e.GetBox().GetSize().y);
}

float NumericValue::ResolveDepth(Element& e) const throw()
{
	Vector2f size = e.GetBox().GetSize();
	return Resolve(e, Math::Max(size.x, size.y));
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

	bool operator()(const Perspective& p)
	{
		return false;
	}

};



struct SetIdentityVisitor
{
	template <size_t N>
	void operator()(ResolvedValuesPrimitive<N>& p)
	{
		for (auto& value : p.values)
			value = 0.0f;
	}
	template <size_t N>
	void operator()(UnresolvedValuesPrimitive<N>& p)
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
};


void Primitive::SetIdentity() throw()
{
	std::visit(SetIdentityVisitor{}, primitive);
}


bool Primitive::ResolveTransform(Matrix4f & m, Element & e) const throw()
{
	ResolveTransformVisitor visitor{ m, e };

	bool result = std::visit(visitor, primitive);

	return result;
}

bool Primitive::ResolvePerspective(float & p, Element & e) const throw()
{
	bool result = false;

	if (const Perspective* perspective = std::get_if<Perspective>(&primitive))
	{
		p = perspective->values[0].ResolveDepth(e);
		result = true;
	}

	return result;
}



struct InterpolateVisitor
{
	const PrimitiveVariant& other_variant;
	float alpha;

	template <size_t N>
	void Interpolate(ResolvedValuesPrimitive<N>& p0, const ResolvedValuesPrimitive<N>& p1)
	{
		for (size_t i = 0; i < N; i++)
			p0.values[i] = p0.values[i] * (1.0f - alpha) + p1.values[i] * alpha;
	}
	template <size_t N>
	void Interpolate(UnresolvedValuesPrimitive<N>& p0, const UnresolvedValuesPrimitive<N>& p1)
	{
		// TODO: While we have the same type and dimension, we may have different units. Should convert?
		for (size_t i = 0; i < N; i++)
			p0.values[i].number = p0.values[i].number*(1.0f - alpha) + p1.values[i].number * alpha;
	}
	//void Interpolate(Matrix3D& p0, Matrix3D& p1)
	//{
	//	// Special interpolation for full matrices TODO
	//}

	template <typename T>
	void operator()(T& p0)
	{
		auto& p1 = std::get<T>(other_variant);
		Interpolate(p0, p1);
	}
};



bool Primitive::InterpolateWith(const Primitive & other, float alpha) throw()
{
	if (primitive.index() != other.primitive.index())
		return false;

	std::visit(InterpolateVisitor{ other.primitive, alpha }, primitive);

	return true;
}



}
}
}
