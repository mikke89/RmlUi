#include "PropertyParserTransform.h"
#include "../../Include/RmlUi/Core/NumericValue.h"
#include "../../Include/RmlUi/Core/Transform.h"
#include "../../Include/RmlUi/Core/TransformPrimitive.h"
#include <string.h>

namespace Rml {

PropertyParserTransform::PropertyParserTransform() :
	number(Unit::NUMBER, Unit::NUMBER
#ifdef RMLUI_MATH_EXPRESSIONS
		,
		MakeCalculationParseTarget(CalculationFinalType::Number)
#endif
			),
	length(Unit::LENGTH, Unit::PX
#ifdef RMLUI_MATH_EXPRESSIONS
		,
		MakeCalculationParseTarget(CalculationFinalType::Length)
#endif
			),
	length_pct(Unit::LENGTH_PERCENT, Unit::PX
#ifdef RMLUI_MATH_EXPRESSIONS
		,
		MakeCalculationParseTarget(CalculationFinalType::Length, CalculationPercentageHint::Length)
#endif
			),
	angle(Unit::ANGLE, Unit::RAD
#ifdef RMLUI_MATH_EXPRESSIONS
		,
		MakeCalculationParseTarget(CalculationFinalType::Angle)
#endif
	)
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
#ifdef RMLUI_MATH_EXPRESSIONS
	CalculationPtr calculations[16];
#endif

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

	auto scan = [&](int& bytes_read, const char* input, const char* keyword, const PropertyParser** parsers, int nargs) {
		return Scan(bytes_read, input, keyword, parsers, args, nargs
#ifdef RMLUI_MATH_EXPRESSIONS
			,
			calculations
#endif
		);
	};
	auto add_primitive = [&](const TransformPrimitive& primitive, int nargs) {
#ifdef RMLUI_MATH_EXPRESSIONS
		const int primitive_index = transform->GetNumPrimitives();
#endif
		transform->AddPrimitive(primitive);
#ifdef RMLUI_MATH_EXPRESSIONS
		for (int argument_index = 0; argument_index < nargs; ++argument_index)
		{
			if (calculations[argument_index])
				transform->AddCalculation(primitive_index, argument_index, calculations[argument_index]);
		}
#else
		(void)nargs;
#endif
	};

	while (*next)
	{
		using namespace Transforms;
		int bytes_read = 0;

		if (scan(bytes_read, next, "perspective", length1, 1))
		{
			add_primitive({Perspective(args)}, 1);
		}
		else if (scan(bytes_read, next, "matrix", number6, 6))
		{
			add_primitive({Matrix2D(args)}, 6);
		}
		else if (scan(bytes_read, next, "matrix3d", number16, 16))
		{
			add_primitive({Matrix3D(args)}, 16);
		}
		else if (scan(bytes_read, next, "translateX", lengthpct1, 1))
		{
			add_primitive({TranslateX(args)}, 1);
		}
		else if (scan(bytes_read, next, "translateY", lengthpct1, 1))
		{
			add_primitive({TranslateY(args)}, 1);
		}
		else if (scan(bytes_read, next, "translateZ", length1, 1))
		{
			add_primitive({TranslateZ(args)}, 1);
		}
		else if (scan(bytes_read, next, "translate", lengthpct2, 2))
		{
			add_primitive({Translate2D(args)}, 2);
		}
		else if (scan(bytes_read, next, "translate3d", lengthpct2_length1, 3))
		{
			add_primitive({Translate3D(args)}, 3);
		}
		else if (scan(bytes_read, next, "scaleX", number1, 1))
		{
			add_primitive({ScaleX(args)}, 1);
		}
		else if (scan(bytes_read, next, "scaleY", number1, 1))
		{
			add_primitive({ScaleY(args)}, 1);
		}
		else if (scan(bytes_read, next, "scaleZ", number1, 1))
		{
			add_primitive({ScaleZ(args)}, 1);
		}
		else if (scan(bytes_read, next, "scale", number2, 2))
		{
			add_primitive({Scale2D(args)}, 2);
		}
		else if (scan(bytes_read, next, "scale", number1, 1))
		{
			args[1] = args[0];
#ifdef RMLUI_MATH_EXPRESSIONS
			calculations[1] = calculations[0];
#endif
			add_primitive({Scale2D(args)}, 2);
		}
		else if (scan(bytes_read, next, "scale3d", number3, 3))
		{
			add_primitive({Scale3D(args)}, 3);
		}
		else if (scan(bytes_read, next, "rotateX", angle1, 1))
		{
			add_primitive({RotateX(args)}, 1);
		}
		else if (scan(bytes_read, next, "rotateY", angle1, 1))
		{
			add_primitive({RotateY(args)}, 1);
		}
		else if (scan(bytes_read, next, "rotateZ", angle1, 1))
		{
			add_primitive({RotateZ(args)}, 1);
		}
		else if (scan(bytes_read, next, "rotate", angle1, 1))
		{
			add_primitive({Rotate2D(args)}, 1);
		}
		else if (scan(bytes_read, next, "rotate3d", number3angle1, 4))
		{
			add_primitive({Rotate3D(args)}, 4);
		}
		else if (scan(bytes_read, next, "skewX", angle1, 1))
		{
			add_primitive({SkewX(args)}, 1);
		}
		else if (scan(bytes_read, next, "skewY", angle1, 1))
		{
			add_primitive({SkewY(args)}, 1);
		}
		else if (scan(bytes_read, next, "skew", angle2, 2))
		{
			add_primitive({Skew2D(args)}, 2);
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
	int nargs
#ifdef RMLUI_MATH_EXPRESSIONS
	,
	CalculationPtr* calculations
#endif
) const
{
	out_bytes_read = 0;
	const char* begin = str;
	while (StringUtilities::IsWhitespace(*str))
		++str;
	const size_t keyword_length = strlen(keyword);
	if (strlen(str) < keyword_length)
		return false;
	if (memcmp(str, keyword, keyword_length) != 0)
		return false;
	str += keyword_length;
	while (StringUtilities::IsWhitespace(*str))
		++str;
	if (*str != '(')
		return false;
	const char* arguments_begin = ++str;
	int depth = 1;
	char quote = 0;
	bool escaped = false;
	for (; *str && depth > 0; ++str)
	{
		const char c = *str;
		if (quote)
		{
			if (escaped)
				escaped = false;
			else if (c == '\\')
				escaped = true;
			else if (c == quote)
				quote = 0;
			continue;
		}
		if (c == '\'' || c == '"')
			quote = c;
		else if (c == '(')
			++depth;
		else if (c == ')')
			--depth;
	}
	if (depth != 0 || quote)
		return false;

	const char* arguments_end = str - 1;
	StringList argument_list;
	String current;
	depth = 0;
	quote = 0;
	escaped = false;
	for (const char* p = arguments_begin; p < arguments_end; ++p)
	{
		const char c = *p;
		if (quote)
		{
			current += c;
			if (escaped)
				escaped = false;
			else if (c == '\\')
				escaped = true;
			else if (c == quote)
				quote = 0;
			continue;
		}
		if (c == '\'' || c == '"')
		{
			quote = c;
			current += c;
		}
		else if (c == '(')
		{
			++depth;
			current += c;
		}
		else if (c == ')')
		{
			--depth;
			current += c;
		}
		else if (c == ',' && depth == 0)
		{
			argument_list.push_back(StringUtilities::StripWhitespace(current));
			current.clear();
		}
		else
			current += c;
	}
	argument_list.push_back(StringUtilities::StripWhitespace(current));
	if ((int)argument_list.size() != nargs)
		return false;

	for (int i = 0; i < nargs; ++i)
	{
#ifdef RMLUI_MATH_EXPRESSIONS
		calculations[i].reset();
#endif
		Property prop;
		if (argument_list[i].empty() || !parsers[i]->ParseValue(prop, argument_list[i], ParameterMap()))
			return false;
#ifdef RMLUI_MATH_EXPRESSIONS
		if (prop.unit == Unit::CALCULATION)
		{
			calculations[i] = prop.value.Get<CalculationPtr>();
			if (!calculations[i])
				return false;
			if (parsers[i] == &angle)
				args[i] = NumericValue(0.f, Unit::RAD);
			else if (parsers[i] == &number)
				args[i] = NumericValue(0.f, Unit::NUMBER);
			else
				args[i] = NumericValue(0.f, Unit::PX);
			continue;
		}
#endif
		args[i] = prop.GetNumericValue();
	}

	while (StringUtilities::IsWhitespace(*str))
		++str;
	out_bytes_read = int(str - begin);
	return out_bytes_read > 0;
}

} // namespace Rml
