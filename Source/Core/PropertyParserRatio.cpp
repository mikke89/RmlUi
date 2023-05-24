/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
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

#include "PropertyParserRatio.h"

namespace Rml {

PropertyParserRatio::PropertyParserRatio() {}

PropertyParserRatio::~PropertyParserRatio() {}

bool PropertyParserRatio::ParseValue(Property& property, const String& value, const ParameterMap& /*parameters*/) const
{
	StringList parts;
	StringUtilities::ExpandString(parts, value, '/');

	if (parts.size() != 2)
	{
		return false;
	}

	float first_value = 0;
	if (!TypeConverter<String, float>::Convert(parts[0], first_value))
	{
		// Number conversion failed
		return false;
	}

	float second_value = 0;
	if (!TypeConverter<String, float>::Convert(parts[1], second_value))
	{
		// Number conversion failed
		return false;
	}

	property.value = Variant(Vector2f(first_value, second_value));
	property.unit = Unit::RATIO;

	return true;
}

} // namespace Rml
