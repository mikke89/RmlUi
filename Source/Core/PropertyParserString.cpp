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

#include <sstream>

#include "PropertyParserString.h"

namespace Rml {

PropertyParserString::PropertyParserString()
{
}

PropertyParserString::~PropertyParserString()
{
}
	
std::string to_utf8(uint32_t cp)
{
	// from https://stackoverflow.com/a/28553727/4882174
	std::string result;
	
	int count;
	if (cp <= 0x007F)
		count = 1;
	else if (cp <= 0x07FF)
		count = 2;
	else if (cp <= 0xFFFF)
		count = 3;
	else if (cp <= 0x10FFFF)
		count = 4;
	else
		return result; // or throw an exception
	
	result.resize(count);
	
	if (count > 1)
	{
		for (int i = count - 1; i > 0; --i)
		{
			result[i] = (char) (0x80 | (cp & 0x3F));
			cp >>= 6;
		}
		
		for (int i = 0; i < count; ++i)
			cp |= (1 << (7 - i));
	}
	
	result[0] = (char) cp;
	
	return result;
}

// Called to parse a RCSS string declaration.
bool PropertyParserString::ParseValue(Property& property, const String& value, const ParameterMap& RMLUI_UNUSED_PARAMETER(parameters)) const
{
	RMLUI_UNUSED(parameters);
	
	// parse escaped unicode literals according to https://www.w3.org/TR/CSS2/syndata.html#characters

	std::string str = value;
	std::string::size_type startIdx = 0;
	do
	{
		startIdx = str.find("\\", startIdx);
		if (startIdx == std::string::npos) 
			break;

		std::string::size_type endIdx = str.find_first_not_of("0123456789abcdefABCDEF", startIdx + 1);
		if (endIdx == std::string::npos) 
			endIdx = str.length();

		std::string tmpStr = str.substr(startIdx + 1, endIdx - (startIdx + 1));
		std::istringstream iss(tmpStr);

		uint32_t cp;
		if (iss >> std::hex >> cp)
		{
			std::string utf8 = to_utf8(cp);
			str.replace(startIdx, 1 + tmpStr.length(), utf8);
			startIdx += utf8.length();
		}
		else
			startIdx += 1;
	}
	while (true);

	property.value = Variant(str);
	property.unit = Property::STRING;

	return true;
}

} // namespace Rml
