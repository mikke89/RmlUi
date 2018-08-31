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

	String ToString() const noexcept;

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
	ResolvedPrimitive(std::array<NumericValue, N> values, std::array<Property::Unit, N> base_units) noexcept
	{
		for (size_t i = 0; i < N; ++i)
			this->values[i] = values[i].ResolveAbsoluteUnit(base_units[i]);
	}
	ResolvedPrimitive(std::array<float, N> values) noexcept : values(values) { }
	
	std::array<float, N> values;

	String ToString(String unit, bool rad_to_deg = false, bool only_unit_on_last_value = false) const noexcept;
};

template< size_t N >
struct UnresolvedPrimitive
{
	UnresolvedPrimitive(const NumericValue* values) noexcept
	{
		for (size_t i = 0; i < N; ++i)
			this->values[i] = values[i];
	}
	UnresolvedPrimitive(std::array<NumericValue, N> values) noexcept : values(values) { }

	std::array<NumericValue, N> values;

	String ToString() const noexcept;
};





struct Matrix2D : public ResolvedPrimitive< 6 >
{
	Matrix2D(const NumericValue* values) noexcept : ResolvedPrimitive(values) { }
	String ToString() const noexcept { return "matrix" + ResolvedPrimitive< 6 >::ToString(""); }
};

struct Matrix3D : public ResolvedPrimitive< 16 >
{
	Matrix3D(const Matrix4f& matrix) noexcept : ResolvedPrimitive(matrix.data()) { }
	Matrix3D(const NumericValue* values) noexcept : ResolvedPrimitive(values) { }
	String ToString() const noexcept { return "matrix3d" + ResolvedPrimitive< 16 >::ToString(""); }
};

struct TranslateX : public UnresolvedPrimitive< 1 >
{
	TranslateX(const NumericValue* values) noexcept : UnresolvedPrimitive(values) { }
	TranslateX(float x, Property::Unit unit = Property::PX) noexcept : UnresolvedPrimitive({ NumericValue{x, unit} }) { }
	String ToString() const noexcept { return "translateX" + UnresolvedPrimitive< 1 >::ToString(); }
};

struct TranslateY : public UnresolvedPrimitive< 1 >
{
	TranslateY(const NumericValue* values) noexcept : UnresolvedPrimitive(values) { }
	TranslateY(float y, Property::Unit unit = Property::PX) noexcept : UnresolvedPrimitive({ NumericValue(y, unit) }) { }
	String ToString() const noexcept { return "translateY" + UnresolvedPrimitive< 1 >::ToString(); }
};

struct TranslateZ : public UnresolvedPrimitive< 1 >
{
	TranslateZ(const NumericValue* values) noexcept : UnresolvedPrimitive(values) { }
	TranslateZ(float z, Property::Unit unit = Property::PX) noexcept : UnresolvedPrimitive({ NumericValue(z, unit) }) { }
	String ToString() const noexcept { return "translateZ" + UnresolvedPrimitive< 1 >::ToString(); }
};

struct Translate2D : public UnresolvedPrimitive< 2 >
{
	Translate2D(const NumericValue* values) noexcept : UnresolvedPrimitive(values) { }
	Translate2D(float x, float y, Property::Unit units = Property::PX) noexcept : UnresolvedPrimitive({ NumericValue(x, units), NumericValue(y, units) }) { }
	String ToString() const noexcept { return "translate" + UnresolvedPrimitive< 2 >::ToString(); }
};

struct Translate3D : public UnresolvedPrimitive< 3 >
{
	Translate3D(const NumericValue* values) noexcept : UnresolvedPrimitive(values) { }
	Translate3D(NumericValue x, NumericValue y, NumericValue z) noexcept : UnresolvedPrimitive({ x, y, z }) { }
	Translate3D(float x, float y, float z, Property::Unit units = Property::PX) noexcept : UnresolvedPrimitive({ NumericValue(x, units), NumericValue(y, units), NumericValue(z, units) }) { }
	String ToString() const noexcept { return "translate3d" + UnresolvedPrimitive< 3 >::ToString(); }
};

struct ScaleX : public ResolvedPrimitive< 1 >
{
	ScaleX(const NumericValue* values) noexcept : ResolvedPrimitive(values) { }
	ScaleX(float value) noexcept : ResolvedPrimitive({ value }) { }
	String ToString() const noexcept { return "scaleX" + ResolvedPrimitive< 1 >::ToString(""); }
};

struct ScaleY : public ResolvedPrimitive< 1 >
{
	ScaleY(const NumericValue* values) noexcept : ResolvedPrimitive(values) { }
	ScaleY(float value) noexcept : ResolvedPrimitive({ value }) { }
	String ToString() const noexcept { return "scaleY" + ResolvedPrimitive< 1 >::ToString(""); }
};

struct ScaleZ : public ResolvedPrimitive< 1 >
{
	ScaleZ(const NumericValue* values) noexcept : ResolvedPrimitive(values) { }
	ScaleZ(float value) noexcept : ResolvedPrimitive({ value }) { }
	String ToString() const noexcept { return "scaleZ" + ResolvedPrimitive< 1 >::ToString(""); }
};

struct Scale2D : public ResolvedPrimitive< 2 >
{
	Scale2D(const NumericValue* values) noexcept : ResolvedPrimitive(values) { }
	Scale2D(float xy) noexcept : ResolvedPrimitive({ xy, xy }) { }
	Scale2D(float x, float y) noexcept : ResolvedPrimitive({ x, y }) { }
	String ToString() const noexcept { return "scale" + ResolvedPrimitive< 2 >::ToString(""); }
};

struct Scale3D : public ResolvedPrimitive< 3 >
{
	Scale3D(const NumericValue* values) noexcept : ResolvedPrimitive(values) { }
	Scale3D(float xyz) noexcept : ResolvedPrimitive({ xyz, xyz, xyz }) { }
	Scale3D(float x, float y, float z) noexcept : ResolvedPrimitive({ x, y, z }) { }
	String ToString() const noexcept { return "scale3d" + ResolvedPrimitive< 3 >::ToString(""); }
};

struct RotateX : public ResolvedPrimitive< 1 >
{
	RotateX(const NumericValue* values) noexcept : ResolvedPrimitive(values, { Property::RAD }) { }
	RotateX(float angle, Property::Unit unit = Property::DEG) noexcept : ResolvedPrimitive({ NumericValue{ angle, unit } }, { Property::RAD }) { }
	String ToString() const noexcept { return "rotateX" + ResolvedPrimitive< 1 >::ToString("deg", true); }
};

struct RotateY : public ResolvedPrimitive< 1 >
{
	RotateY(const NumericValue* values) noexcept : ResolvedPrimitive(values, { Property::RAD }) {}
	RotateY(float angle, Property::Unit unit = Property::DEG) noexcept : ResolvedPrimitive({ NumericValue{ angle, unit } }, { Property::RAD }) { }
	String ToString() const noexcept { return "rotateY" + ResolvedPrimitive< 1 >::ToString("deg", true); }
};

struct RotateZ : public ResolvedPrimitive< 1 >
{
	RotateZ(const NumericValue* values) noexcept : ResolvedPrimitive(values, { Property::RAD }) { }
	RotateZ(float angle, Property::Unit unit = Property::DEG) noexcept : ResolvedPrimitive({ NumericValue{ angle, unit } }, { Property::RAD }) { }
	String ToString() const noexcept { return "rotateZ" + ResolvedPrimitive< 1 >::ToString("deg", true); }
};

struct Rotate2D : public ResolvedPrimitive< 1 >
{
	Rotate2D(const NumericValue* values) noexcept : ResolvedPrimitive(values, { Property::RAD }) { }
	Rotate2D(float angle, Property::Unit unit = Property::DEG) noexcept : ResolvedPrimitive({ NumericValue{ angle, unit } }, { Property::RAD }) { }
	String ToString() const noexcept { return "rotate" + ResolvedPrimitive< 1 >::ToString("deg", true); }
};

struct Rotate3D : public ResolvedPrimitive< 4 >
{
	Rotate3D(const NumericValue* values) noexcept : ResolvedPrimitive(values, { Property::NUMBER, Property::NUMBER, Property::NUMBER, Property::RAD }) { }
	Rotate3D(float x, float y, float z, float angle, Property::Unit angle_unit = Property::DEG) noexcept
		: ResolvedPrimitive({ NumericValue{x, Property::NUMBER}, NumericValue{y, Property::NUMBER}, NumericValue{z, Property::NUMBER}, NumericValue{angle, angle_unit} },
			{ Property::NUMBER, Property::NUMBER, Property::NUMBER, Property::RAD }) { }
	String ToString() const noexcept { return "rotate3d" + ResolvedPrimitive< 4 >::ToString("deg", true, true); }
};

struct SkewX : public ResolvedPrimitive< 1 >
{
	SkewX(const NumericValue* values) noexcept : ResolvedPrimitive(values, { Property::RAD }) { }
	SkewX(float angle, Property::Unit unit = Property::DEG) noexcept : ResolvedPrimitive({ NumericValue{ angle, unit } }, { Property::RAD }) { }
	String ToString() const noexcept { return "skewX" + ResolvedPrimitive< 1 >::ToString("deg", true); }
};

struct SkewY : public ResolvedPrimitive< 1 >
{
	SkewY(const NumericValue* values) noexcept : ResolvedPrimitive(values, { Property::RAD }) { }
	SkewY(float angle, Property::Unit unit = Property::DEG) noexcept : ResolvedPrimitive({ NumericValue{ angle, unit } }, { Property::RAD }) { }
	String ToString() const noexcept { return "skewY" + ResolvedPrimitive< 1 >::ToString("deg", true); }
};

struct Skew2D : public ResolvedPrimitive< 2 >
{
	Skew2D(const NumericValue* values) noexcept : ResolvedPrimitive(values, { Property::RAD, Property::RAD }) { }
	Skew2D(float x, float y, Property::Unit unit = Property::DEG) noexcept : ResolvedPrimitive({NumericValue{ x, unit }, { NumericValue{ y, unit }}}, {Property::RAD, Property::RAD}) { }
	String ToString() const noexcept { return "skew" + ResolvedPrimitive< 2 >::ToString("deg", true); }
};

struct Perspective : public UnresolvedPrimitive< 1 >
{
	Perspective(const NumericValue* values) noexcept : UnresolvedPrimitive(values) { }
	String ToString() const noexcept { return "perspective" + UnresolvedPrimitive< 1 >::ToString(); }
};

struct DecomposedMatrix4
{
	Vector4f perspective;
	Vector4f quaternion;
	Vector3f translation;
	Vector3f scale;
	Vector3f skew;

	bool Decompose(const Matrix4f& m);
	String ToString() const noexcept { return "decomposedMatrix3d"; }
};


using PrimitiveVariant = std::variant<
	Matrix2D, Matrix3D,
	TranslateX, TranslateY, TranslateZ, Translate2D, Translate3D,
	ScaleX, ScaleY, ScaleZ, Scale2D, Scale3D,
	RotateX, RotateY, RotateZ, Rotate2D, Rotate3D,
	SkewX, SkewY, Skew2D,
	Perspective, DecomposedMatrix4>;


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

	template<typename PrimitiveType>
	Primitive(PrimitiveType primitive) : primitive(primitive) {}

	void SetIdentity() noexcept;

	bool ResolveTransform(Matrix4f& m, Element& e) const noexcept;
	bool ResolvePerspective(float &p, Element& e) const noexcept;
	
	// Prepares this primitive for interpolation. This must be done before calling InterpolateWith().
	// Promote units to basic types which can be interpolated, that is, convert 'length -> pixel' for unresolved primitives.
	// Returns false if the owning transform must to be converted to a DecomposedMatrix4 primitive.
	bool PrepareForInterpolation(Element& e) noexcept;

	// If primitives do not match, try to convert them to a common generic type, e.g. TranslateX -> Translate3D.
	// Returns true if they are already the same type or were converted to a common generic type.
	static bool TryConvertToMatchingGenericType(Primitive& p0, Primitive& p1) noexcept;

	// Interpolate this primitive with another primitive, weighted by alpha [0, 1].
	// Primitives must be of same type and PrepareForInterpolation() must previously have been called on both.
	bool InterpolateWith(const Primitive& other, float alpha) noexcept;

	String ToString() const noexcept;

};


template<size_t N>
inline String ResolvedPrimitive<N>::ToString(String unit, bool rad_to_deg, bool only_unit_on_last_value) const noexcept {
	float multiplier = 1.0f;
	if (rad_to_deg) multiplier = 180.f / Math::ROCKET_PI;
	String tmp;
	String result = "(";
	for (size_t i = 0; i < N; i++) {
		if (TypeConverter<float, String>::Convert(values[i] * multiplier, tmp))
			result += tmp;
		if (!unit.Empty() && (!only_unit_on_last_value || (i == N - 1)))
			result += unit;
		if (i != N - 1) result += ", ";
	}
	result += ")";
	return result;
}

template<size_t N>
inline String UnresolvedPrimitive<N>::ToString() const noexcept {
	String result = "(";
	for (size_t i = 0; i < N; i++) {
		result += values[i].ToString();
		if (i != N - 1) result += ", ";
	}
	result += ")";
	return result;
}

}
}
}

#endif
