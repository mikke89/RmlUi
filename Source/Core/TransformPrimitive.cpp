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

#include "../../Include/RmlUi/Core/TransformPrimitive.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/TypeConverter.h"

namespace Rml {
namespace Transforms {

	/// Returns the numeric value converted to 'base_unit'. Only accepts base units of 'Number' or 'Rad':
	///   'Number' will pass-through the provided value.
	///   'Rad' will convert {Rad, Deg, %} -> Rad.
	static float ResolvePrimitiveAbsoluteValue(NumericValue value, Unit base_unit) noexcept
	{
		RMLUI_ASSERT(base_unit == Unit::RAD || base_unit == Unit::NUMBER);

		if (base_unit == Unit::RAD)
		{
			switch (value.unit)
			{
			case Unit::RAD: return value.number;
			case Unit::DEG: return Math::DegreesToRadians(value.number);
			case Unit::PERCENT: return value.number * 0.01f * 2.0f * Math::RMLUI_PI;
			default: Log::Message(Log::LT_WARNING, "Trying to pass a non-angle unit to a property expecting an angle.");
			}
		}
		else if (base_unit == Unit::NUMBER && value.unit != Unit::NUMBER)
		{
			Log::Message(Log::LT_WARNING, "A unit was passed to a property which expected a unit-less number.");
		}

		return value.number;
	}

	template <size_t N>
	inline ResolvedPrimitive<N>::ResolvedPrimitive(const float* values) noexcept
	{
		for (size_t i = 0; i < N; ++i)
			this->values[i] = values[i];
	}

	template <size_t N>
	inline ResolvedPrimitive<N>::ResolvedPrimitive(const NumericValue* values) noexcept
	{
		for (size_t i = 0; i < N; ++i)
			this->values[i] = values[i].number;
	}

	template <size_t N>
	inline ResolvedPrimitive<N>::ResolvedPrimitive(const NumericValue* values, Array<Unit, N> base_units) noexcept
	{
		for (size_t i = 0; i < N; ++i)
			this->values[i] = ResolvePrimitiveAbsoluteValue(values[i], base_units[i]);
	}

	template <size_t N>
	inline ResolvedPrimitive<N>::ResolvedPrimitive(Array<NumericValue, N> values, Array<Unit, N> base_units) noexcept
	{
		for (size_t i = 0; i < N; ++i)
			this->values[i] = ResolvePrimitiveAbsoluteValue(values[i], base_units[i]);
	}

	template <size_t N>
	inline ResolvedPrimitive<N>::ResolvedPrimitive(Array<float, N> values) noexcept : values(values)
	{}

	template <size_t N>
	inline UnresolvedPrimitive<N>::UnresolvedPrimitive(const NumericValue* values) noexcept
	{
		for (size_t i = 0; i < N; ++i)
			this->values[i] = values[i];
	}

	template <size_t N>
	inline UnresolvedPrimitive<N>::UnresolvedPrimitive(Array<NumericValue, N> values) noexcept : values(values)
	{}

	Matrix2D::Matrix2D(const NumericValue* values) noexcept : ResolvedPrimitive(values) {}

	Matrix3D::Matrix3D(const NumericValue* values) noexcept : ResolvedPrimitive(values) {}
	Matrix3D::Matrix3D(const Matrix4f& matrix) noexcept : ResolvedPrimitive(matrix.data()) {}

	TranslateX::TranslateX(const NumericValue* values) noexcept : UnresolvedPrimitive(values) {}
	TranslateX::TranslateX(float x, Unit unit) noexcept : UnresolvedPrimitive({NumericValue(x, unit)}) {}

	TranslateY::TranslateY(const NumericValue* values) noexcept : UnresolvedPrimitive(values) {}
	TranslateY::TranslateY(float y, Unit unit) noexcept : UnresolvedPrimitive({NumericValue(y, unit)}) {}

	TranslateZ::TranslateZ(const NumericValue* values) noexcept : UnresolvedPrimitive(values) {}
	TranslateZ::TranslateZ(float z, Unit unit) noexcept : UnresolvedPrimitive({NumericValue(z, unit)}) {}

	Translate2D::Translate2D(const NumericValue* values) noexcept : UnresolvedPrimitive(values) {}
	Translate2D::Translate2D(float x, float y, Unit units) noexcept : UnresolvedPrimitive({NumericValue(x, units), NumericValue(y, units)}) {}

	Translate3D::Translate3D(const NumericValue* values) noexcept : UnresolvedPrimitive(values) {}
	Translate3D::Translate3D(NumericValue x, NumericValue y, NumericValue z) noexcept : UnresolvedPrimitive({x, y, z}) {}
	Translate3D::Translate3D(float x, float y, float z, Unit units) noexcept :
		UnresolvedPrimitive({NumericValue(x, units), NumericValue(y, units), NumericValue(z, units)})
	{}

	ScaleX::ScaleX(const NumericValue* values) noexcept : ResolvedPrimitive(values) {}
	ScaleX::ScaleX(float value) noexcept : ResolvedPrimitive({value}) {}

	ScaleY::ScaleY(const NumericValue* values) noexcept : ResolvedPrimitive(values) {}
	ScaleY::ScaleY(float value) noexcept : ResolvedPrimitive({value}) {}

	ScaleZ::ScaleZ(const NumericValue* values) noexcept : ResolvedPrimitive(values) {}
	ScaleZ::ScaleZ(float value) noexcept : ResolvedPrimitive({value}) {}

	Scale2D::Scale2D(const NumericValue* values) noexcept : ResolvedPrimitive(values) {}
	Scale2D::Scale2D(float xy) noexcept : ResolvedPrimitive({xy, xy}) {}
	Scale2D::Scale2D(float x, float y) noexcept : ResolvedPrimitive({x, y}) {}

	Scale3D::Scale3D(const NumericValue* values) noexcept : ResolvedPrimitive(values) {}
	Scale3D::Scale3D(float xyz) noexcept : ResolvedPrimitive({xyz, xyz, xyz}) {}
	Scale3D::Scale3D(float x, float y, float z) noexcept : ResolvedPrimitive({x, y, z}) {}

	RotateX::RotateX(const NumericValue* values) noexcept : ResolvedPrimitive(values, {Unit::RAD}) {}
	RotateX::RotateX(float angle, Unit unit) noexcept : ResolvedPrimitive({NumericValue{angle, unit}}, {Unit::RAD}) {}

	RotateY::RotateY(const NumericValue* values) noexcept : ResolvedPrimitive(values, {Unit::RAD}) {}
	RotateY::RotateY(float angle, Unit unit) noexcept : ResolvedPrimitive({NumericValue{angle, unit}}, {Unit::RAD}) {}

	RotateZ::RotateZ(const NumericValue* values) noexcept : ResolvedPrimitive(values, {Unit::RAD}) {}
	RotateZ::RotateZ(float angle, Unit unit) noexcept : ResolvedPrimitive({NumericValue{angle, unit}}, {Unit::RAD}) {}

	Rotate2D::Rotate2D(const NumericValue* values) noexcept : ResolvedPrimitive(values, {Unit::RAD}) {}
	Rotate2D::Rotate2D(float angle, Unit unit) noexcept : ResolvedPrimitive({NumericValue{angle, unit}}, {Unit::RAD}) {}

	Rotate3D::Rotate3D(const NumericValue* values) noexcept : ResolvedPrimitive(values, {Unit::NUMBER, Unit::NUMBER, Unit::NUMBER, Unit::RAD}) {}
	Rotate3D::Rotate3D(float x, float y, float z, float angle, Unit angle_unit) noexcept :
		ResolvedPrimitive(
			{NumericValue{x, Unit::NUMBER}, NumericValue{y, Unit::NUMBER}, NumericValue{z, Unit::NUMBER}, NumericValue{angle, angle_unit}},
			{Unit::NUMBER, Unit::NUMBER, Unit::NUMBER, Unit::RAD})
	{}

	SkewX::SkewX(const NumericValue* values) noexcept : ResolvedPrimitive(values, {Unit::RAD}) {}
	SkewX::SkewX(float angle, Unit unit) noexcept : ResolvedPrimitive({NumericValue{angle, unit}}, {Unit::RAD}) {}

	SkewY::SkewY(const NumericValue* values) noexcept : ResolvedPrimitive(values, {Unit::RAD}) {}
	SkewY::SkewY(float angle, Unit unit) noexcept : ResolvedPrimitive({NumericValue{angle, unit}}, {Unit::RAD}) {}

	Skew2D::Skew2D(const NumericValue* values) noexcept : ResolvedPrimitive(values, {Unit::RAD, Unit::RAD}) {}
	Skew2D::Skew2D(float x, float y, Unit unit) noexcept : ResolvedPrimitive({NumericValue{x, unit}, {NumericValue{y, unit}}}, {Unit::RAD, Unit::RAD})
	{}

	Perspective::Perspective(const NumericValue* values) noexcept : UnresolvedPrimitive(values) {}

} // namespace Transforms
} // namespace Rml
