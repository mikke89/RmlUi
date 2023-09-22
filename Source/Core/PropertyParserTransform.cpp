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

#include "PropertyParserTransform.h"
#include "../../Include/RmlUi/Core/NumericValue.h"
#include "../../Include/RmlUi/Core/Transform.h"
#include "../../Include/RmlUi/Core/TransformPrimitive.h"
#include <string.h>

namespace Rml {

PropertyParserTransform::PropertyParserTransform() :
	number(Unit::NUMBER), length(Unit::LENGTH, Unit::PX), length_pct(Unit::LENGTH_PERCENT, Unit::PX), angle(Unit::ANGLE, Unit::RAD)
{}

PropertyParserTransform::~PropertyParserTransform() {}

bool PropertyParserTransform::ParseValue(Property& property, const String& value, const ParameterMap& /*parameters*/) const
{
	if (value == "none")
	{
		property.value = Variant(TransformPtr());
		property.unit = Unit::TRANSFORM;
		return true;
	}

	TransformPtr transform = MakeShared<Transform>();

	char const* next = value.c_str();

	NumericValue args[16];

	const PropertyParser* number16[] = {&number, &number, &number, &number, &number, &number, &number, &number, &number, &number, &number, &number,
		&number, &number, &number, &number};
	const PropertyParser* lengthpct2_length1[] = {&length_pct, &length_pct, &length};
	const PropertyParser* number3angle1[] = {&number, &number, &number, &angle};
	const PropertyParser* angle2[] = {&angle, &angle};
	const PropertyParser* length1[] = {&length};

	// For semantic purposes, define subsets of the above parsers when scanning primitives below.
	auto lengthpct1 = lengthpct2_length1;
	auto lengthpct2 = lengthpct2_length1;
	auto angle1 = angle2;
	auto number1 = number16;
	auto number2 = number16;
	auto number3 = number16;
	auto number6 = number16;

	while (*next)
	{
		using namespace Transforms;
		int bytes_read = 0;

		if (Scan(bytes_read, next, "perspective", length1, args, 1))
		{
			transform->AddPrimitive({Perspective(args)});
		}
		else if (Scan(bytes_read, next, "matrix", number6, args, 6))
		{
			transform->AddPrimitive({Matrix2D(args)});
		}
		else if (Scan(bytes_read, next, "matrix3d", number16, args, 16))
		{
			transform->AddPrimitive({Matrix3D(args)});
		}
		else if (Scan(bytes_read, next, "translateX", lengthpct1, args, 1))
		{
			transform->AddPrimitive({TranslateX(args)});
		}
		else if (Scan(bytes_read, next, "translateY", lengthpct1, args, 1))
		{
			transform->AddPrimitive({TranslateY(args)});
		}
		else if (Scan(bytes_read, next, "translateZ", length1, args, 1))
		{
			transform->AddPrimitive({TranslateZ(args)});
		}
		else if (Scan(bytes_read, next, "translate", lengthpct2, args, 2))
		{
			transform->AddPrimitive({Translate2D(args)});
		}
		else if (Scan(bytes_read, next, "translate3d", lengthpct2_length1, args, 3))
		{
			transform->AddPrimitive({Translate3D(args)});
		}
		else if (Scan(bytes_read, next, "scaleX", number1, args, 1))
		{
			transform->AddPrimitive({ScaleX(args)});
		}
		else if (Scan(bytes_read, next, "scaleY", number1, args, 1))
		{
			transform->AddPrimitive({ScaleY(args)});
		}
		else if (Scan(bytes_read, next, "scaleZ", number1, args, 1))
		{
			transform->AddPrimitive({ScaleZ(args)});
		}
		else if (Scan(bytes_read, next, "scale", number2, args, 2))
		{
			transform->AddPrimitive({Scale2D(args)});
		}
		else if (Scan(bytes_read, next, "scale", number1, args, 1))
		{
			args[1] = args[0];
			transform->AddPrimitive({Scale2D(args)});
		}
		else if (Scan(bytes_read, next, "scale3d", number3, args, 3))
		{
			transform->AddPrimitive({Scale3D(args)});
		}
		else if (Scan(bytes_read, next, "rotateX", angle1, args, 1))
		{
			transform->AddPrimitive({RotateX(args)});
		}
		else if (Scan(bytes_read, next, "rotateY", angle1, args, 1))
		{
			transform->AddPrimitive({RotateY(args)});
		}
		else if (Scan(bytes_read, next, "rotateZ", angle1, args, 1))
		{
			transform->AddPrimitive({RotateZ(args)});
		}
		else if (Scan(bytes_read, next, "rotate", angle1, args, 1))
		{
			transform->AddPrimitive({Rotate2D(args)});
		}
		else if (Scan(bytes_read, next, "rotate3d", number3angle1, args, 4))
		{
			transform->AddPrimitive({Rotate3D(args)});
		}
		else if (Scan(bytes_read, next, "skewX", angle1, args, 1))
		{
			transform->AddPrimitive({SkewX(args)});
		}
		else if (Scan(bytes_read, next, "skewY", angle1, args, 1))
		{
			transform->AddPrimitive({SkewY(args)});
		}
		else if (Scan(bytes_read, next, "skew", angle2, args, 2))
		{
			transform->AddPrimitive({Skew2D(args)});
		}

		if (bytes_read > 0)
		{
			next += bytes_read;
		}
		else
		{
			return false;
		}
	}

	property.value = Variant(std::move(transform));
	property.unit = Unit::TRANSFORM;

	return true;
}

bool PropertyParserTransform::Scan(int& out_bytes_read, const char* str, const char* keyword, const PropertyParser** parsers, NumericValue* args,
	int nargs) const
{
	out_bytes_read = 0;
	int total_bytes_read = 0, bytes_read = 0;

	/* skip leading white space */
	bytes_read = 0;
	sscanf(str, " %n", &bytes_read);
	str += bytes_read;
	total_bytes_read += bytes_read;

	/* find the keyword */
	if (!memcmp(str, keyword, strlen(keyword)))
	{
		bytes_read = (int)strlen(keyword);
		str += bytes_read;
		total_bytes_read += bytes_read;
	}
	else
	{
		return false;
	}

	/* skip any white space */
	bytes_read = 0;
	sscanf(str, " %n", &bytes_read);
	str += bytes_read;
	total_bytes_read += bytes_read;

	/* find the opening brace */
	bytes_read = 0;
	if (sscanf(str, " ( %n", &bytes_read), bytes_read)
	{
		str += bytes_read;
		total_bytes_read += bytes_read;
	}
	else
	{
		return false;
	}

	/* use the quicker stack-based argument buffer, if possible */
	char* arg = nullptr;
	char arg_stack[1024];
	String arg_heap;
	if (strlen(str) < sizeof(arg_stack))
	{
		arg = arg_stack;
	}
	else
	{
		arg_heap = str;
		arg = &arg_heap[0];
	}

	/* parse the arguments */
	for (int i = 0; i < nargs; ++i)
	{
		Property prop;

		bytes_read = 0;
		if (sscanf(str, " %[^,)] %n", arg, &bytes_read), bytes_read && parsers[i]->ParseValue(prop, String(arg), ParameterMap()))
		{
			args[i].number = prop.value.Get<float>();
			args[i].unit = prop.unit;
			str += bytes_read;
			total_bytes_read += bytes_read;
		}
		else
		{
			return false;
		}

		/* find the comma */
		if (i < nargs - 1)
		{
			bytes_read = 0;
			if (sscanf(str, " , %n", &bytes_read), bytes_read)
			{
				str += bytes_read;
				total_bytes_read += bytes_read;
			}
			else
			{
				return false;
			}
		}
	}

	/* find the closing brace */
	bytes_read = 0;
	if (sscanf(str, " ) %n", &bytes_read), bytes_read)
	{
		str += bytes_read;
		total_bytes_read += bytes_read;
	}
	else
	{
		return false;
	}

	out_bytes_read = total_bytes_read;
	return total_bytes_read > 0;
}

} // namespace Rml
