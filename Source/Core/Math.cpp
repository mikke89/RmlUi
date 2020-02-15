/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
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

#include "../../Include/RmlUi/Core/Math.h"
#include <time.h>
#include <math.h>
#include <stdlib.h>

namespace Rml {
namespace Core {
namespace Math {

const float RMLUI_PI = 3.141592653f;

static constexpr float FZERO = 0.0001f;

// Evaluates if a number is, or close to, zero.
RMLUICORE_API bool IsZero(float value)
{
	return AbsoluteValue(value) < FZERO;
}

// Evaluates if two floating-point numbers are equal, or so similar that they could be considered
// so.
RMLUICORE_API bool AreEqual(float value_0, float value_1)
{
	return IsZero(value_1 - value_0);
}

// Calculates the absolute value of a number.
RMLUICORE_API float AbsoluteValue(float value)
{
	return fabsf(value);
}

// Calculates the cosine of an angle.
RMLUICORE_API float Cos(float angle)
{
	return cosf(angle);
}

// Calculates the arc-cosine of an value.
RMLUICORE_API float ACos(float value)
{
	return acosf(value);
}

// Calculates the sine of an angle.
RMLUICORE_API float Sin(float angle)
{
	return sinf(angle);
}

// Calculates the arc-sine of an value.
RMLUICORE_API float ASin(float angle)
{
	return asinf(angle);
}

// Calculates the tangent of an angle.
RMLUICORE_API float Tan(float angle)
{
	return tanf(angle);
}

// Calculates the angle of a two-dimensional line.
RMLUICORE_API float ATan2(float y, float x)
{
	return atan2f(y, x);
}

// Evaluates the natural exponential function on a value.
RMLUICORE_API float Exp(float value)
{
	return expf(value);
}

// Converts an angle from radians to degrees.
RMLUICORE_API float RadiansToDegrees(float angle)
{
	return angle * (180.0f / RMLUI_PI);
}

// Converts an angle from degrees to radians.
RMLUICORE_API float DegreesToRadians(float angle)
{
	return angle * (RMLUI_PI / 180.0f);
}

// Normalises an angle in radians
RMLUICORE_API float NormaliseAngle(float angle)
{
	return fmodf(angle, RMLUI_PI * 2.0f);
}

// Calculates the square root of a value.
RMLUICORE_API float SquareRoot(float value)
{
	return sqrtf(value);
}

// Rounds a floating-point value to the nearest integer.
RMLUICORE_API float RoundFloat(float value)
{
	return roundf(value);
}

// Rounds a floating-point value to the nearest integer.
RMLUICORE_API double RoundFloat(double value)
{
	return round(value);
}

// Rounds a floating-point value to the nearest integer.
RMLUICORE_API int RoundToInteger(float value)
{
	if (value > 0.0f)
		return RealToInteger(value + 0.5f);

	return RealToInteger(value - 0.5f);
}

// Rounds a floating-point value up to the nearest integer.
RMLUICORE_API int RoundUpToInteger(float value)
{
	return RealToInteger(ceilf(value));
}

// Rounds a floating-point value down to the nearest integer.
RMLUICORE_API int RoundDownToInteger(float value)
{
	return RealToInteger(floorf(value));
}

// Efficiently truncates a floating-point value into an integer.
RMLUICORE_API int RealToInteger(float value)
{
	return int(value);
}

// Converts the given number to a power of two, rounding up if necessary.
RMLUICORE_API int ToPowerOfTwo(int number)
{
	// Check if the number is already a power of two.
	if ((number & (number - 1)) == 0)
		return number;

	// Assuming 31 useful bits in an int here ... !
	for (int i = 31; i >= 0; i--)
	{
		if (number & (1 << i))
		{
			if (i == 31)
				return 1 << 31;
			else
				return 1 << (i + 1);
		}
	}

	return 0;
}

// Converts from a hexadecimal digit to decimal.
RMLUICORE_API int HexToDecimal(char hex_digit)
{
	if (hex_digit >= '0' && hex_digit <= '9')
		return hex_digit - '0';
	else if (hex_digit >= 'a' && hex_digit <= 'f')
		return 10 + (hex_digit - 'a');
	else if (hex_digit >= 'A' && hex_digit <= 'F')
		return 10 + (hex_digit - 'A');

	return -1;
}

// Generates a random floating-point value between 0 and a user-specified value.
RMLUICORE_API float RandomReal(float max_value)
{
	return (rand() / (float) RAND_MAX) * max_value;
}

// Generates a random integer value between 0 and a user-specified value.
RMLUICORE_API int RandomInteger(int max_value)
{
	return (rand() % max_value);
}

// Generates a random boolean value, with equal chance of true or false.
RMLUICORE_API bool RandomBool()
{
	return RandomInteger(2) == 1;
}

}
}
}
