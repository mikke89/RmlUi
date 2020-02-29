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

#include "PropertyParserTransform.h"
#include "../../Include/RmlUi/Core/TransformPrimitive.h"
#include "../../Include/RmlUi/Core/Transform.h"
#include "StringCache.h"
#include <string.h>

namespace Rml {
namespace Core {

PropertyParserTransform::PropertyParserTransform()
	: number(Property::NUMBER),
	  length(Property::LENGTH_PERCENT, Property::PX),
	  angle(Property::ANGLE, Property::RAD)
{
}

PropertyParserTransform::~PropertyParserTransform()
{
}

// Called to parse a RCSS transform declaration.
bool PropertyParserTransform::ParseValue(Property& property, const String& value, const ParameterMap& parameters) const
{
	if(value == NONE)
	{
		property.value = Variant(TransformPtr());
		property.unit = Property::TRANSFORM;
		return true;
	}

	UniquePtr<Transform> transform(new Transform);

	char const* next = value.c_str();

	Transforms::NumericValue args[16];

	const PropertyParser* angle1[] = { &angle };
	const PropertyParser* angle2[] = { &angle, &angle };
	const PropertyParser* length1[] = { &length };
	const PropertyParser* length2[] = { &length, &length };
	const PropertyParser* length3[] = { &length, &length, &length };
	const PropertyParser* number3angle1[] = { &number, &number, &number, &angle };
	const PropertyParser* number1[] = { &number };
	const PropertyParser* number2[] = { &number, &number };
	const PropertyParser* number3[] = { &number, &number, &number };
	const PropertyParser* number6[] = { &number, &number, &number, &number, &number, &number };
	const PropertyParser* number16[] = { &number, &number, &number, &number, &number, &number, &number, &number, &number, &number, &number, &number, &number, &number, &number, &number };
	while (strlen(next))
	{
		using namespace Transforms;
		int bytes_read = 0;

		if ((bytes_read = Scan(next, "perspective", length1, args, 1)))
		{
			transform->AddPrimitive({ Perspective(args) });
		}
		else if ((bytes_read = Scan(next, "matrix", number6, args, 6)))
		{
			transform->AddPrimitive({ Matrix2D(args) });
		}
		else if ((bytes_read = Scan(next, "matrix3d", number16, args, 16)))
		{
			transform->AddPrimitive({ Matrix3D(args) });
		}
		else if ((bytes_read = Scan(next, "translateX", length1, args, 1)))
		{
			transform->AddPrimitive({ TranslateX(args) });
		}
		else if ((bytes_read = Scan(next, "translateY", length1, args, 1)))
		{
			transform->AddPrimitive({ TranslateY(args) });
		}
		else if ((bytes_read = Scan(next, "translateZ", length1, args, 1)))
		{
			transform->AddPrimitive({ TranslateZ(args) });
		}
		else if ((bytes_read = Scan(next, "translate", length2, args, 2)))
		{
			transform->AddPrimitive({ Translate2D(args) });
		}
		else if ((bytes_read = Scan(next, "translate3d", length3, args, 3)))
		{
			transform->AddPrimitive({ Translate3D(args) });
		}
		else if ((bytes_read = Scan(next, "scaleX", number1, args, 1)))
		{
			transform->AddPrimitive({ ScaleX(args) });
		}
		else if ((bytes_read = Scan(next, "scaleY", number1, args, 1)))
		{
			transform->AddPrimitive({ ScaleY(args) });
		}
		else if ((bytes_read = Scan(next, "scaleZ", number1, args, 1)))
		{
			transform->AddPrimitive({ ScaleZ(args) });
		}
		else if ((bytes_read = Scan(next, "scale", number2, args, 2)))
		{
			transform->AddPrimitive({ Scale2D(args) });
		}
		else if ((bytes_read = Scan(next, "scale", number1, args, 1)))
		{
			args[1] = args[0];
			transform->AddPrimitive({ Scale2D(args) });
		}
		else if ((bytes_read = Scan(next, "scale3d", number3, args, 3)))
		{
			transform->AddPrimitive({ Scale3D(args) });
		}
		else if ((bytes_read = Scan(next, "rotateX", angle1, args, 1)))
		{
			transform->AddPrimitive({ RotateX(args) });
		}
		else if ((bytes_read = Scan(next, "rotateY", angle1, args, 1)))
		{
			transform->AddPrimitive({ RotateY(args) });
		}
		else if ((bytes_read = Scan(next, "rotateZ", angle1, args, 1)))
		{
			transform->AddPrimitive({ RotateZ(args) });
		}
		else if ((bytes_read = Scan(next, "rotate", angle1, args, 1)))
		{
			transform->AddPrimitive({ Rotate2D(args) });
		}
		else if ((bytes_read = Scan(next, "rotate3d", number3angle1, args, 4)))
		{
			transform->AddPrimitive({ Rotate3D(args) });
		}
		else if ((bytes_read = Scan(next, "skewX", angle1, args, 1)))
		{
			transform->AddPrimitive({ SkewX(args) });
		}
		else if ((bytes_read = Scan(next, "skewY", angle1, args, 1)))
		{
			transform->AddPrimitive({ SkewY(args) });
		}
		else if ((bytes_read = Scan(next, "skew", angle2, args, 2)))
		{
			transform->AddPrimitive({ Skew2D(args) });
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
	
	property.value = Variant(TransformPtr(std::move(transform)));
	property.unit = Property::TRANSFORM;

	return true;
}

// Scan a string for a parameterized keyword with a certain number of numeric arguments.
int PropertyParserTransform::Scan(const char* str, const char* keyword, const PropertyParser** parsers, Transforms::NumericValue* args, int nargs) const
{
	int total_bytes_read = 0, bytes_read = 0;

	/* use the quicker stack-based argument buffer, if possible */
	char *arg = 0;
	char arg_stack[1024];
	std::string arg_heap;
	if (strlen(str) < sizeof(arg_stack))
	{
		arg = arg_stack;
	}
	else
	{
		arg_heap = str;
		arg = &arg_heap[0];
	}

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
		return 0;
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
		return 0;
	}

	/* parse the arguments */
	for (int i = 0; i < nargs; ++i)
	{
		Property prop;

		bytes_read = 0;
		if (sscanf(str, " %[^,)] %n", arg, &bytes_read), bytes_read
			&& parsers[i]->ParseValue(prop, String(arg), ParameterMap()))
		{
			args[i].number = prop.value.Get<float>();
			args[i].unit = prop.unit;
			str += bytes_read;
			total_bytes_read += bytes_read;
		}
		else
		{
			return 0;
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
				return 0;
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
		return 0;
	}

	return total_bytes_read;
}

}
}
