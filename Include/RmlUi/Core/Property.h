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

#ifndef RMLUI_CORE_PROPERTY_H
#define RMLUI_CORE_PROPERTY_H

#include "Variant.h"
#include "Header.h"
#include <type_traits>

namespace Rml {

class PropertyDefinition;

struct RMLUICORE_API PropertySource {
	PropertySource(String path, int line_number, String rule_name) 
		: path(std::move(path)), line_number(line_number), rule_name(std::move(rule_name)) {}
	String path;
	int line_number;
	String rule_name;
};


/**
	@author Peter Curry
 */

class RMLUICORE_API Property
{
public:
	enum Unit
	{
		UNKNOWN = 1 << 0,

		KEYWORD = 1 << 1,           // generic keyword; fetch as < int >

		STRING = 1 << 2,            // generic string; fetch as < String >

		// Absolute values.
		NUMBER = 1 << 3,            // number unsuffixed; fetch as < float >
		PX = 1 << 4,                // number suffixed by 'px'; fetch as < float >
		DEG = 1 << 5,               // number suffixed by 'deg'; fetch as < float >
		RAD = 1 << 6,               // number suffixed by 'rad'; fetch as < float >
		COLOUR = 1 << 7,            // colour; fetch as < Colourb >
		DP = 1 << 8,                // density-independent pixel; number suffixed by 'dp'; fetch as < float >
		X = 1 << 9,                 // dots per px unit; number suffixed by 'x'; fetch as < float >
		VW = 1 << 10,               // viewport-width percentage; number suffixed by 'vw'; fetch as < float >
		VH = 1 << 11,               // viewport-height percentage; number suffixed by 'vh'; fetch as < float >
		ABSOLUTE_UNIT = NUMBER | PX | DP | X | DEG | RAD | COLOUR | VW | VH,

		// Relative values.
		EM = 1 << 12,               // number suffixed by 'em'; fetch as < float >
		PERCENT = 1 << 13,          // number suffixed by '%'; fetch as < float >
		REM = 1 << 14,              // number suffixed by 'rem'; fetch as < float >
		RELATIVE_UNIT = EM | REM | PERCENT,

		// Values based on pixels-per-inch.
		INCH = 1 << 15,             // number suffixed by 'in'; fetch as < float >
		CM = 1 << 16,               // number suffixed by 'cm'; fetch as < float >
		MM = 1 << 17,               // number suffixed by 'mm'; fetch as < float >
		PT = 1 << 18,               // number suffixed by 'pt'; fetch as < float >
		PC = 1 << 19,               // number suffixed by 'pc'; fetch as < float >
		PPI_UNIT = INCH | CM | MM | PT | PC,

		TRANSFORM = 1 << 20,        // transform; fetch as < TransformPtr >, may be empty
		TRANSITION = 1 << 21,       // transition; fetch as < TransitionList >
		ANIMATION = 1 << 22,        // animation; fetch as < AnimationList >
		DECORATOR = 1 << 23,        // decorator; fetch as < DecoratorsPtr >
		FONTEFFECT = 1 << 24,       // font-effect; fetch as < FontEffectsPtr >
		RATIO = 1 << 25,            // ratio defined as x/y; fetch as < Vector2f >

		LENGTH = PX | DP | PPI_UNIT | EM | REM | VW | VH | X,
		LENGTH_PERCENT = LENGTH | PERCENT,
		NUMBER_LENGTH_PERCENT = NUMBER | LENGTH | PERCENT,
		ABSOLUTE_LENGTH = PX | DP | PPI_UNIT | VH | VW | X,
		ANGLE = DEG | RAD
	};

	Property();
	template < typename PropertyType >
	Property(PropertyType value, Unit unit, int specificity = -1) : value(value), unit(unit), specificity(specificity)
	{
		definition = nullptr;
		parser_index = -1;
	}
	template<typename EnumType, typename = typename std::enable_if< std::is_enum<EnumType>::value, EnumType >::type>
	Property(EnumType value) : value(static_cast<int>(value)), unit(KEYWORD), specificity(-1) {}

	/// Get the value of the property as a string.
	String ToString() const;

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

// OR operator for combining multiple units where applicable.
inline Property::Unit operator|(Property::Unit lhs, Property::Unit rhs)
{
	using underlying_t = std::underlying_type<Property::Unit>::type;
	return static_cast<Property::Unit>(static_cast<underlying_t>(lhs) | static_cast<underlying_t>(rhs));
}

} // namespace Rml
#endif
