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

#ifndef RMLUICOREPROPERTYPARSERNUMBER_H
#define RMLUICOREPROPERTYPARSERNUMBER_H

#include "../../Include/RmlUi/Core/PropertyParser.h"

namespace Rml {
namespace Core {

/**
	A property parser that parses a floating-point number with an optional unit.

	@author Peter Curry
 */

class PropertyParserNumber : public PropertyParser
{
public:
	PropertyParserNumber(int units, Property::Unit zero_unit = Property::UNKNOWN);
	virtual ~PropertyParserNumber();

	/// Called to parse a RCSS number declaration.
	/// @param[out] property The property to set the parsed value on.
	/// @param[in] value The raw value defined for this property.
	/// @param[in] parameters The parameters defined for this property.
	/// @return True if the value was validated successfully, false otherwise.
	bool ParseValue(Property& property, const String& value, const ParameterMap& parameters) const override;

private:
	// Stores a bit mask of allowed units.
	int units;

	// If zero unit is set and pure numbers are not allowed, parsing of "0" is still allowed and assigned the given unit
	Property::Unit zero_unit;

	// Stores a list of the numerical units and their suffixes.
	typedef std::pair< Property::Unit, String > UnitSuffix;
	std::vector< UnitSuffix > unit_suffixes;
};

}
}

#endif
