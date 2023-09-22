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

#ifndef RMLUI_CORE_PROPERTYPARSERTRANSFORM_H
#define RMLUI_CORE_PROPERTYPARSERTRANSFORM_H

#include "../../Include/RmlUi/Core/PropertyParser.h"
#include "PropertyParserNumber.h"

namespace Rml {

/**
    A property parser that parses a RCSS transform property specification.

    @author Markus Schöngart
 */
class PropertyParserTransform : public PropertyParser {
public:
	PropertyParserTransform();
	virtual ~PropertyParserTransform();

	/// Called to parse a RCSS transform declaration.
	/// @param[out] property The property to set the parsed value on.
	/// @param[in] value The raw value defined for this property.
	/// @param[in] parameters The parameters defined for this property.
	/// @return True if the value was validated successfully, false otherwise.
	bool ParseValue(Property& property, const String& value, const ParameterMap& parameters) const override;

private:
	/// Scan a string for a parameterized keyword with a certain number of numeric arguments.
	/// @param[out] out_bytes_read The number of bytes read if the keyword occurs at the beginning of str, 0 otherwise.
	/// @param[in] str The string to search for the parameterized keyword
	/// @param[in] keyword The name of the keyword to search for
	/// @param[in] parsers The numeric argument parsers
	/// @param[out] args The numeric arguments encountered
	/// @param[in] nargs The number of numeric arguments expected
	/// @return True if parsed successfully, false otherwise.
	bool Scan(int& out_bytes_read, const char* str, const char* keyword, const PropertyParser** parsers, NumericValue* args, int nargs) const;

	PropertyParserNumber number, length, length_pct, angle;
};

} // namespace Rml
#endif
