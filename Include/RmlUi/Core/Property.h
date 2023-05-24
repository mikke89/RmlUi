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

#ifndef RMLUI_CORE_PROPERTY_H
#define RMLUI_CORE_PROPERTY_H

#include "Header.h"
#include "NumericValue.h"
#include "Unit.h"
#include "Variant.h"
#include <type_traits>

namespace Rml {

class PropertyDefinition;

struct RMLUICORE_API PropertySource {
	PropertySource(String path, int line_number, String rule_name) : path(std::move(path)), line_number(line_number), rule_name(std::move(rule_name))
	{}
	String path;
	int line_number;
	String rule_name;
};

/**
    @author Peter Curry
 */

class RMLUICORE_API Property {
public:
	Property();
	template <typename PropertyType>
	Property(PropertyType value, Unit unit, int specificity = -1) : value(value), unit(unit), specificity(specificity)
	{
		definition = nullptr;
		parser_index = -1;
	}
	template <typename EnumType, typename = typename std::enable_if<std::is_enum<EnumType>::value, EnumType>::type>
	Property(EnumType value) : value(static_cast<int>(value)), unit(Unit::KEYWORD), specificity(-1)
	{}

	/// Get the value of the property as a string.
	String ToString() const;

	/// Get the value of the property as a numeric value, if applicable.
	NumericValue GetNumericValue() const;

	/// Templatised accessor.
	template <typename T>
	T Get() const
	{
		return value.Get<T>();
	}

	bool operator==(const Property& other) const { return unit == other.unit && value == other.value; }
	bool operator!=(const Property& other) const { return !(*this == other); }

	Variant value;
	Unit unit;
	int specificity;

	const PropertyDefinition* definition = nullptr;
	int parser_index = -1;

	SharedPtr<const PropertySource> source;
};

} // namespace Rml
#endif
