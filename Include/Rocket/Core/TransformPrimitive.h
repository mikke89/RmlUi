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

#ifndef ROCKETCORETRANSFORMPRIMITIVE_H
#define ROCKETCORETRANSFORMPRIMITIVE_H

#include "Header.h"
#include "Types.h"
#include "Property.h"
#include <variant>

namespace Rocket {
namespace Core {
namespace Transforms {
	

struct NumericValue
{
	/// Non-initializing constructor.
	NumericValue() throw();
	/// Construct from a float and a Unit.
	NumericValue(float number, Property::Unit unit) throw();

	/// Resolve a numeric property value for an element.
	float Resolve(Element& e, float base) const throw();
	/// Resolve a numeric property value with the element's width as relative base value.
	float ResolveWidth(Element& e) const throw();
	/// Resolve a numeric property value with the element's height as relative base value.
	float ResolveHeight(Element& e) const throw();
	/// Resolve a numeric property value with the element's depth as relative base value.
	float ResolveDepth(Element& e) const throw();

	float number;
	Rocket::Core::Property::Unit unit;
};



template< size_t N >
struct ResolvedValuesPrimitive
{
	ResolvedValuesPrimitive(const float* values) throw()
	{
		for (size_t i = 0; i < N; ++i)
			this->values[i] = values[i];
	}
	ResolvedValuesPrimitive(const NumericValue* values) throw()
	{
		for (size_t i = 0; i < N; ++i) 
			this->values[i] = values[i].number;
	}
	
	float values[N];
};

template< size_t N >
struct UnresolvedValuesPrimitive
{
	UnresolvedValuesPrimitive(const NumericValue* values) throw()
	{
		memcpy(this->values, values, sizeof(this->values));
	}

	NumericValue values[N];
};





struct Matrix2D : public ResolvedValuesPrimitive< 6 >
{
	Matrix2D(const NumericValue* values) throw() : ResolvedValuesPrimitive(values) { }
};

struct Matrix3D : public ResolvedValuesPrimitive< 16 >
{
	Matrix3D(const Matrix4f& matrix) throw() : ResolvedValuesPrimitive(matrix.data()) { }
	Matrix3D(const NumericValue* values) throw() : ResolvedValuesPrimitive(values) { }
};

struct TranslateX : public UnresolvedValuesPrimitive< 1 >
{
	TranslateX(const NumericValue* values) throw() : UnresolvedValuesPrimitive(values) { }
};

struct TranslateY : public UnresolvedValuesPrimitive< 1 >
{
	TranslateY(const NumericValue* values) throw() : UnresolvedValuesPrimitive(values) { }
};

struct TranslateZ : public UnresolvedValuesPrimitive< 1 >
{
	TranslateZ(const NumericValue* values) throw() : UnresolvedValuesPrimitive(values) { }
};

struct Translate2D : public UnresolvedValuesPrimitive< 2 >
{
	Translate2D(const NumericValue* values) throw() : UnresolvedValuesPrimitive(values) { }
};

struct Translate3D : public UnresolvedValuesPrimitive< 3 >
{
	Translate3D(const NumericValue* values) throw() : UnresolvedValuesPrimitive(values) { }
};

struct ScaleX : public ResolvedValuesPrimitive< 1 >
{
	ScaleX(const NumericValue* values) throw() : ResolvedValuesPrimitive(values) { }
};

struct ScaleY : public ResolvedValuesPrimitive< 1 >
{
	ScaleY(const NumericValue* values) throw() : ResolvedValuesPrimitive(values) { }
};

struct ScaleZ : public ResolvedValuesPrimitive< 1 >
{
	ScaleZ(const NumericValue* values) throw() : ResolvedValuesPrimitive(values) { }
};

struct Scale2D : public ResolvedValuesPrimitive< 2 >
{
	Scale2D(const NumericValue* values) throw() : ResolvedValuesPrimitive(values) { }
};

struct Scale3D : public ResolvedValuesPrimitive< 3 >
{
	Scale3D(const NumericValue* values) throw() : ResolvedValuesPrimitive(values) { }
};

struct RotateX : public ResolvedValuesPrimitive< 1 >
{
	RotateX(const NumericValue* values) throw() : ResolvedValuesPrimitive(values) { }
};

struct RotateY : public ResolvedValuesPrimitive< 1 >
{
	RotateY(const NumericValue* values) throw() : ResolvedValuesPrimitive(values) {}
};

struct RotateZ : public ResolvedValuesPrimitive< 1 >
{
	RotateZ(const NumericValue* values) throw() : ResolvedValuesPrimitive(values) { }
};

struct Rotate2D : public ResolvedValuesPrimitive< 1 >
{
	Rotate2D(const NumericValue* values) throw() : ResolvedValuesPrimitive(values) { }
};

struct Rotate3D : public ResolvedValuesPrimitive< 4 >
{
	Rotate3D(const NumericValue* values) throw() : ResolvedValuesPrimitive(values) { }
};

struct SkewX : public ResolvedValuesPrimitive< 1 >
{
	SkewX(const NumericValue* values) throw() : ResolvedValuesPrimitive(values) { }
};

struct SkewY : public ResolvedValuesPrimitive< 1 >
{
	SkewY(const NumericValue* values) throw() : ResolvedValuesPrimitive(values) { }
};

struct Skew2D : public ResolvedValuesPrimitive< 2 >
{
	Skew2D(const NumericValue* values) throw() : ResolvedValuesPrimitive(values) { }
};

struct Perspective : public UnresolvedValuesPrimitive< 1 >
{
	Perspective(const NumericValue* values) throw() : UnresolvedValuesPrimitive(values) { }
};


using PrimitiveVariant = std::variant<
	Matrix2D, Matrix3D,
	TranslateX, TranslateY, TranslateZ, Translate2D, Translate3D,
	ScaleX, ScaleY, ScaleZ, Scale2D, Scale3D,
	RotateX, RotateY, RotateZ, Rotate2D, Rotate3D,
	SkewX, SkewY, Skew2D,
	Perspective>;


/**
	The Primitive struct is the base struct of geometric transforms such as rotations, scalings and translations.
	Instances of this struct are added to a Rocket::Core::Transform instance
	by the Rocket::Core::PropertyParserTransform, which is responsible for
	parsing the `transform' property.

	@author Markus Schöngart
	@see Rocket::Core::Transform
	@see Rocket::Core::PropertyParserTransform
 */
struct Primitive
{
	PrimitiveVariant primitive;

	void SetIdentity() throw();
	bool ResolveTransform(Matrix4f& m, Element& e) const throw();
	bool ResolvePerspective(float &p, Element& e) const throw();
	bool InterpolateWith(const Primitive& other, float alpha) throw();
};


}
}
}

#endif
