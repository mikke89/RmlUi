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
namespace Transforms {


/// Returns the numeric value converted to 'base_unit'. Only accepts base units of 'Number' or 'Rad':
///   'Number' will pass-through the provided value.
///   'Rad' will convert {Rad, Deg, %} -> Rad.
static float ResolvePrimitiveAbsoluteValue(NumericValue value, Property::Unit base_unit) noexcept
{
	RMLUI_ASSERT(base_unit == Property::RAD || base_unit == Property::NUMBER);

	if (base_unit == Property::RAD)
	{
		switch (value.unit)
		{
		case Property::RAD:
			return value.number;
		case Property::DEG:
			return Math::DegreesToRadians(value.number);
		case Property::PERCENT:
			return value.number * 0.01f * 2.0f * Math::RMLUI_PI;
		default:
			Log::Message(Log::LT_WARNING, "Trying to pass a non-angle unit to a property expecting an angle.");
		}
	}
	else if (base_unit == Property::NUMBER && value.unit != Property::NUMBER)
	{
		Log::Message(Log::LT_WARNING, "A unit was passed to a property which expected a unit-less number.");
	}

	return value.number;
}


template<size_t N>
inline ResolvedPrimitive<N>::ResolvedPrimitive(const float* values) noexcept
{
	for (size_t i = 0; i < N; ++i)
		this->values[i] = values[i];
}

template<size_t N>
inline ResolvedPrimitive<N>::ResolvedPrimitive(const NumericValue* values) noexcept
{
	for (size_t i = 0; i < N; ++i)
		this->values[i] = values[i].number;
}

template<size_t N>
inline ResolvedPrimitive<N>::ResolvedPrimitive(const NumericValue* values, Array<Property::Unit, N> base_units) noexcept
{
	for (size_t i = 0; i < N; ++i)
		this->values[i] = ResolvePrimitiveAbsoluteValue(values[i], base_units[i]);
}

template<size_t N>
inline ResolvedPrimitive<N>::ResolvedPrimitive(Array<NumericValue, N> values, Array<Property::Unit, N> base_units) noexcept
{
	for (size_t i = 0; i < N; ++i)
		this->values[i] = ResolvePrimitiveAbsoluteValue(values[i], base_units[i]);
}

template<size_t N>
inline ResolvedPrimitive<N>::ResolvedPrimitive(Array<float, N> values) noexcept : values(values) { }

template<size_t N>
inline UnresolvedPrimitive<N>::UnresolvedPrimitive(const NumericValue* values) noexcept
{
	for (size_t i = 0; i < N; ++i)
		this->values[i] = values[i];
}

template<size_t N>
inline UnresolvedPrimitive<N>::UnresolvedPrimitive(Array<NumericValue, N> values) noexcept : values(values) { }


Matrix2D::Matrix2D(const NumericValue* values) noexcept : ResolvedPrimitive(values) { }

Matrix3D::Matrix3D(const NumericValue* values) noexcept : ResolvedPrimitive(values) { }
Matrix3D::Matrix3D(const Matrix4f& matrix) noexcept : ResolvedPrimitive(matrix.data()) { }

TranslateX::TranslateX(const NumericValue* values) noexcept : UnresolvedPrimitive(values) { }
TranslateX::TranslateX(float x, Property::Unit unit) noexcept : UnresolvedPrimitive({ NumericValue(x, unit) }) { }

TranslateY::TranslateY(const NumericValue* values) noexcept : UnresolvedPrimitive(values) { }
TranslateY::TranslateY(float y, Property::Unit unit) noexcept : UnresolvedPrimitive({ NumericValue(y, unit) }) { }

TranslateZ::TranslateZ(const NumericValue* values) noexcept : UnresolvedPrimitive(values) { }
TranslateZ::TranslateZ(float z, Property::Unit unit) noexcept : UnresolvedPrimitive({ NumericValue(z, unit) }) { }

Translate2D::Translate2D(const NumericValue* values) noexcept : UnresolvedPrimitive(values) { }
Translate2D::Translate2D(float x, float y, Property::Unit units) noexcept : UnresolvedPrimitive({ NumericValue(x, units), NumericValue(y, units) }) { }

Translate3D::Translate3D(const NumericValue* values) noexcept : UnresolvedPrimitive(values) { }
Translate3D::Translate3D(NumericValue x, NumericValue y, NumericValue z) noexcept : UnresolvedPrimitive({ x, y, z }) { }
Translate3D::Translate3D(float x, float y, float z, Property::Unit units) noexcept 
	: UnresolvedPrimitive({ NumericValue(x, units), NumericValue(y, units), NumericValue(z, units) }) { }

ScaleX::ScaleX(const NumericValue* values) noexcept : ResolvedPrimitive(values) { }
ScaleX::ScaleX(float value) noexcept : ResolvedPrimitive({ value }) { }

ScaleY::ScaleY(const NumericValue* values) noexcept : ResolvedPrimitive(values) { }
ScaleY::ScaleY(float value) noexcept : ResolvedPrimitive({ value }) { }

ScaleZ::ScaleZ(const NumericValue* values) noexcept : ResolvedPrimitive(values) { }
ScaleZ::ScaleZ(float value) noexcept : ResolvedPrimitive({ value }) { }

Scale2D::Scale2D(const NumericValue* values) noexcept : ResolvedPrimitive(values) { }
Scale2D::Scale2D(float xy) noexcept : ResolvedPrimitive({ xy, xy }) { }
Scale2D::Scale2D(float x, float y) noexcept : ResolvedPrimitive({ x, y }) { }

Scale3D::Scale3D(const NumericValue* values) noexcept : ResolvedPrimitive(values) { }
Scale3D::Scale3D(float xyz) noexcept : ResolvedPrimitive({ xyz, xyz, xyz }) { }
Scale3D::Scale3D(float x, float y, float z) noexcept : ResolvedPrimitive({ x, y, z }) { }

RotateX::RotateX(const NumericValue* values) noexcept : ResolvedPrimitive(values, { Property::RAD }) { }
RotateX::RotateX(float angle, Property::Unit unit) noexcept : ResolvedPrimitive({ NumericValue{ angle, unit } }, { Property::RAD }) { }

RotateY::RotateY(const NumericValue* values) noexcept : ResolvedPrimitive(values, { Property::RAD }) {}
RotateY::RotateY(float angle, Property::Unit unit) noexcept : ResolvedPrimitive({ NumericValue{ angle, unit } }, { Property::RAD }) { }

RotateZ::RotateZ(const NumericValue* values) noexcept : ResolvedPrimitive(values, { Property::RAD }) { }
RotateZ::RotateZ(float angle, Property::Unit unit) noexcept : ResolvedPrimitive({ NumericValue{ angle, unit } }, { Property::RAD }) { }

Rotate2D::Rotate2D(const NumericValue* values) noexcept : ResolvedPrimitive(values, { Property::RAD }) { }
Rotate2D::Rotate2D(float angle, Property::Unit unit) noexcept : ResolvedPrimitive({ NumericValue{ angle, unit } }, { Property::RAD }) { }

Rotate3D::Rotate3D(const NumericValue* values) noexcept : ResolvedPrimitive(values, { Property::NUMBER, Property::NUMBER, Property::NUMBER, Property::RAD }) { }
Rotate3D::Rotate3D(float x, float y, float z, float angle, Property::Unit angle_unit) noexcept
	: ResolvedPrimitive(
		{ NumericValue{x, Property::NUMBER}, NumericValue{y, Property::NUMBER}, NumericValue{z, Property::NUMBER}, NumericValue{angle, angle_unit} },
		{ Property::NUMBER, Property::NUMBER, Property::NUMBER, Property::RAD }
	)
{ }

SkewX::SkewX(const NumericValue* values) noexcept : ResolvedPrimitive(values, { Property::RAD }) { }
SkewX::SkewX(float angle, Property::Unit unit) noexcept : ResolvedPrimitive({ NumericValue{ angle, unit } }, { Property::RAD }) { }

SkewY::SkewY(const NumericValue* values) noexcept : ResolvedPrimitive(values, { Property::RAD }) { }
SkewY::SkewY(float angle, Property::Unit unit) noexcept : ResolvedPrimitive({ NumericValue{ angle, unit } }, { Property::RAD }) { }

Skew2D::Skew2D(const NumericValue* values) noexcept : ResolvedPrimitive(values, { Property::RAD, Property::RAD }) { }
Skew2D::Skew2D(float x, float y, Property::Unit unit) noexcept 
	: ResolvedPrimitive({ NumericValue{ x, unit }, { NumericValue{ y, unit }} }, { Property::RAD, Property::RAD }) { }

Perspective::Perspective(const NumericValue* values) noexcept : UnresolvedPrimitive(values) { }


}
} // namespace Rml