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

#ifndef RMLUI_CORE_PROPERTYPARSERCOLOUR_H
#define RMLUI_CORE_PROPERTYPARSERCOLOUR_H

#include "../../Include/RmlUi/Core/PropertyParser.h"
#include "../../Include/RmlUi/Core/Types.h"

namespace Rml {

/**
    A property parser that parses a colour value.

    @author Peter Curry
 */

class PropertyParserColour : public PropertyParser {
public:
	PropertyParserColour();
	virtual ~PropertyParserColour();

	/// Called to parse a RCSS colour declaration.
	/// @param[out] property The property to set the parsed value on.
	/// @param[in] value The raw value defined for this property.
	/// @param[in] parameters The parameters defined for this property; not used for this parser.
	/// @return True if the value was parsed successfully, false otherwise.
	bool ParseValue(Property& property, const String& value, const ParameterMap& parameters) const override;

	/// Parse a colour directly.
	static bool ParseColour(Colourb& colour, const String& value);

private:
	using ColourMap = UnorderedMap<String, Colourb>;
	static const ColourMap html_colours;
};

} // namespace Rml
#endif
