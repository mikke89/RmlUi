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
#include <Rocket/Core/TransformPrimitive.h>
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

bool Matrix2D::ResolveTransform(Matrix4f& m, Element& e) const throw()
{
	m = Matrix4f::FromRows(
		Vector4f(values[0], values[1], 0, values[2]),
		Vector4f(values[3], values[4], 0, values[5]),
		Vector4f(        0,         0, 1,         0),
		Vector4f(        0,         0, 0,         1)
	);
	return true;
}

bool Matrix3D::ResolveTransform(Matrix4f& m, Element& e) const throw()
{
	m = Matrix4f::FromRows(
		Vector4f(values[ 0], values[ 1], values[ 2], values[ 3]),
		Vector4f(values[ 4], values[ 5], values[ 6], values[ 7]),
		Vector4f(values[ 8], values[ 9], values[10], values[11]),
		Vector4f(values[12], values[13], values[14], values[15])
	);
	return true;
}

bool TranslateX::ResolveTransform(Matrix4f& m, Element& e) const throw()
{
	m = Matrix4f::TranslateX(values[0].ResolveWidth(e));
	return true;
}

bool TranslateY::ResolveTransform(Matrix4f& m, Element& e) const throw()
{
	m = Matrix4f::TranslateY(values[0].ResolveHeight(e));
	return true;
}

bool TranslateZ::ResolveTransform(Matrix4f& m, Element& e) const throw()
{
	m = Matrix4f::TranslateZ(values[0].ResolveDepth(e));
	return true;
}

bool Translate2D::ResolveTransform(Matrix4f& m, Element& e) const throw()
{
	m = Matrix4f::Translate(
		values[0].ResolveWidth(e),
		values[1].ResolveHeight(e),
		0
	);
	return true;
}

bool Translate3D::ResolveTransform(Matrix4f& m, Element& e) const throw()
{
	m = Matrix4f::Translate(
		values[0].ResolveWidth(e),
		values[1].ResolveHeight(e),
		values[2].ResolveDepth(e)
	);
	return true;
}

bool ScaleX::ResolveTransform(Matrix4f& m, Element& e) const throw()
{
	m = Matrix4f::ScaleX(values[0]);
	return true;
}

bool ScaleY::ResolveTransform(Matrix4f& m, Element& e) const throw()
{
	m = Matrix4f::ScaleY(values[0]);
	return true;
}

bool ScaleZ::ResolveTransform(Matrix4f& m, Element& e) const throw()
{
	m = Matrix4f::ScaleZ(values[0]);
	return true;
}

bool Scale2D::ResolveTransform(Matrix4f& m, Element& e) const throw()
{
	m = Matrix4f::Scale(values[0], values[1], 1);
	return true;
}

bool Scale3D::ResolveTransform(Matrix4f& m, Element& e) const throw()
{
	m = Matrix4f::Scale(values[0], values[1], values[2]);
	return true;
}

bool RotateX::ResolveTransform(Matrix4f& m, Element& e) const throw()
{
	m = Matrix4f::RotateX(values[0]);
	return true;
}

bool RotateY::ResolveTransform(Matrix4f& m, Element& e) const throw()
{
	m = Matrix4f::RotateY(values[0]);
	return true;
}

bool RotateZ::ResolveTransform(Matrix4f& m, Element& e) const throw()
{
	m = Matrix4f::RotateZ(values[0]);
	return true;
}

bool Rotate2D::ResolveTransform(Matrix4f& m, Element& e) const throw()
{
	m = Matrix4f::RotateZ(values[0]);
	return true;
}

bool Rotate3D::ResolveTransform(Matrix4f& m, Element& e) const throw()
{
	m = Matrix4f::Rotate(Vector3f(values[0], values[1], values[2]), values[3]);
	return true;
}

bool SkewX::ResolveTransform(Matrix4f& m, Element& e) const throw()
{
	std::cout << "foo" << std::endl;
	m = Matrix4f::SkewX(values[0]);
	return true;
}

bool SkewY::ResolveTransform(Matrix4f& m, Element& e) const throw()
{
	m = Matrix4f::SkewY(values[0]);
	return true;
}

bool Skew2D::ResolveTransform(Matrix4f& m, Element& e) const throw()
{
	m = Matrix4f::Skew(values[0], values[1]);
	return true;
}

bool Perspective::ResolvePerspective(float& p, Element& e) const throw()
{
	p = values[0].ResolveDepth(e);
	return true;
}

}
}
}
