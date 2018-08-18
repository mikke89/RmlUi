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
#include <array>

namespace Rocket {
namespace Core {
namespace Transforms {
	

struct NumericValue
{
	/// Non-initializing constructor.
	NumericValue() noexcept;
	/// Construct from a float and a Unit.
	NumericValue(float number, Property::Unit unit) noexcept;

	/// Resolve a numeric property value for an element.
	float Resolve(Element& e, float base) const noexcept;
	/// Resolve a numeric property value with the element's width as relative base value.
	float ResolveWidth(Element& e) const noexcept;
	/// Resolve a numeric property value with the element's height as relative base value.
	float ResolveHeight(Element& e) const noexcept;
	/// Resolve a numeric property value with the element's depth as relative base value.
	float ResolveDepth(Element& e) const noexcept;
	/// Returns the numeric value converted to base_unit, or 'number' if no relationship defined.
	/// Defined for: {Number, Deg, %} -> Rad
	float ResolveAbsoluteUnit(Property::Unit base_unit) const noexcept;

	float number;
	Property::Unit unit;
};


template< size_t N >
struct ResolvedPrimitive
{
	ResolvedPrimitive(const float* values) noexcept
	{
		for (size_t i = 0; i < N; ++i)
			this->values[i] = values[i];
	}
	ResolvedPrimitive(const NumericValue* values) noexcept
	{
		for (size_t i = 0; i < N; ++i) 
			this->values[i] = values[i].number;
	}
	ResolvedPrimitive(const NumericValue* values, std::array<Property::Unit, N> base_units) noexcept
	{
		for (size_t i = 0; i < N; ++i)
			this->values[i] = values[i].ResolveAbsoluteUnit(base_units[i]);
	}
	
	float values[N];
};

template< size_t N >
struct UnresolvedPrimitive
{
	UnresolvedPrimitive(const NumericValue* values) noexcept
	{
		memcpy(this->values, values, sizeof(this->values));
	}

	NumericValue values[N];
};





struct Matrix2D : public ResolvedPrimitive< 6 >
{
	Matrix2D(const NumericValue* values) noexcept : ResolvedPrimitive(values) { }
};

struct Matrix3D : public ResolvedPrimitive< 16 >
{
	Matrix3D(const Matrix4f& matrix) noexcept : ResolvedPrimitive(matrix.data()) { }
	Matrix3D(const NumericValue* values) noexcept : ResolvedPrimitive(values) { }
};

struct TranslateX : public UnresolvedPrimitive< 1 >
{
	TranslateX(const NumericValue* values) noexcept : UnresolvedPrimitive(values) { }
};

struct TranslateY : public UnresolvedPrimitive< 1 >
{
	TranslateY(const NumericValue* values) noexcept : UnresolvedPrimitive(values) { }
};

struct TranslateZ : public UnresolvedPrimitive< 1 >
{
	TranslateZ(const NumericValue* values) noexcept : UnresolvedPrimitive(values) { }
};

struct Translate2D : public UnresolvedPrimitive< 2 >
{
	Translate2D(const NumericValue* values) noexcept : UnresolvedPrimitive(values) { }
};

struct Translate3D : public UnresolvedPrimitive< 3 >
{
	Translate3D(const NumericValue* values) noexcept : UnresolvedPrimitive(values) { }
};

struct ScaleX : public ResolvedPrimitive< 1 >
{
	ScaleX(const NumericValue* values) noexcept : ResolvedPrimitive(values) { }
};

struct ScaleY : public ResolvedPrimitive< 1 >
{
	ScaleY(const NumericValue* values) noexcept : ResolvedPrimitive(values) { }
};

struct ScaleZ : public ResolvedPrimitive< 1 >
{
	ScaleZ(const NumericValue* values) noexcept : ResolvedPrimitive(values) { }
};

struct Scale2D : public ResolvedPrimitive< 2 >
{
	Scale2D(const NumericValue* values) noexcept : ResolvedPrimitive(values) { }
};

struct Scale3D : public ResolvedPrimitive< 3 >
{
	Scale3D(const NumericValue* values) noexcept : ResolvedPrimitive(values) { }
};

struct RotateX : public ResolvedPrimitive< 1 >
{
	RotateX(const NumericValue* values) noexcept : ResolvedPrimitive(values, { Property::RAD }) { }
};

struct RotateY : public ResolvedPrimitive< 1 >
{
	RotateY(const NumericValue* values) noexcept : ResolvedPrimitive(values, { Property::RAD }) {}
};

struct RotateZ : public ResolvedPrimitive< 1 >
{
	RotateZ(const NumericValue* values) noexcept : ResolvedPrimitive(values, { Property::RAD }) { }
};

struct Rotate2D : public ResolvedPrimitive< 1 >
{
	Rotate2D(const NumericValue* values) noexcept : ResolvedPrimitive(values, { Property::RAD }) { }
};

struct Rotate3D : public ResolvedPrimitive< 4 >
{
	Rotate3D(const NumericValue* values) noexcept : ResolvedPrimitive(values, { Property::NUMBER, Property::NUMBER, Property::NUMBER, Property::RAD }) { }
};

struct SkewX : public ResolvedPrimitive< 1 >
{
	SkewX(const NumericValue* values) noexcept : ResolvedPrimitive(values, { Property::RAD }) { }
};

struct SkewY : public ResolvedPrimitive< 1 >
{
	SkewY(const NumericValue* values) noexcept : ResolvedPrimitive(values, { Property::RAD }) { }
};

struct Skew2D : public ResolvedPrimitive< 2 >
{
	Skew2D(const NumericValue* values) noexcept : ResolvedPrimitive(values, { Property::RAD, Property::RAD }) { }
};

struct Perspective : public UnresolvedPrimitive< 1 >
{
	Perspective(const NumericValue* values) noexcept : UnresolvedPrimitive(values) { }
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

	bool ResolveTransform(Matrix4f& m, Element& e) const noexcept;
	bool ResolvePerspective(float &p, Element& e) const noexcept;
	
	// Promote units to basic types which can be interpolated, that is, convert 'length -> pixel' for unresolved primitives.
	bool ResolveUnits(Element& e) noexcept;

	bool InterpolateWith(const Primitive& other, float alpha) noexcept;
	void SetIdentity() noexcept;
};


}
}
}

#endif
