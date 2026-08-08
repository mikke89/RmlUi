#include "../Common/TestsInterface.h"
#include "../Common/TestsShell.h"
#include <RmlUi/Core/Animation.h>
#include <RmlUi/Core/ComputedValues.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/DecorationTypes.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Factory.h>
#include <RmlUi/Core/StreamMemory.h>
#include <RmlUi/Core/StyleSheet.h>
#include <RmlUi/Core/StyleSheetContainer.h>
#include <RmlUi/Core/Transform.h>
#include <RmlUi/Core/TransformPrimitive.h>
#include <algorithm>
#include <array>
#include <cstdlib>
#include <doctest.h>
#ifdef RMLUI_MATH_EXPRESSIONS
	#include "../../../Source/Core/CalculationParser.h"
	#include "../../../Source/Core/CalculationResolver.h"
	#include "../../../Source/Core/ElementAnimation.h"
	#include "../../../Source/Core/GeometryBoxShadow.h"
	#include "../../../Source/Core/PropertyParserAnimation.h"
	#include <RmlUi/Core/Filter.h>
	#include <RmlUi/Core/ID.h>
	#include <RmlUi/Core/PropertyParser.h>
	#include <RmlUi/Core/PropertyDefinition.h>
	#include <RmlUi/Core/StyleSheetSpecification.h>
#endif

using namespace Rml;

namespace {

#ifdef RMLUI_MATH_EXPRESSIONS

// WPT-derived test data below is adapted from web-platform-tests/wpt at
// 5b6a1e61dbf639ebea0b7f19d9df0e313ae5d959 (BSD-3-Clause). Source paths are
// recorded alongside the adapted cases. Tests requiring unsupported CSS functions, units, or
// browser-only semantics are intentionally omitted.

struct CalculationCorpusCase {
	const char* source;
	const char* expression;
	const char* expected;
};

enum class CalculationCorpusTarget {
	Number,
	Percentage,
	Length,
	LengthPercentage,
	Angle,
	Time,
	LineHeight,
};

struct InvalidCalculationCorpusCase {
	const char* source;
	CalculationCorpusTarget target;
	const char* expression;
};

struct WptParseCase {
	CalculationCorpusTarget target;
	const char* source;
	const char* expression;
};

// Sources:
//   css/css-values/calc-unit-analysis.html
//   css/css-values/calc-nesting.html
//   css/css-values/calc-numbers.html
//   css/css-values/minmax-number-computed.html
constexpr CalculationCorpusCase pure_number_cases[] = {
	{"calc-numbers.html", "calc(2 * 3)", "6"},
	{"calc-numbers.html", "calc(2 / 4)", "0.5"},
	{"adapted:calc-nesting.html", "calc(calc(2) * calc(50))", "100"},
	{"minmax-number-computed.html", "min(1)", "1"},
	{"minmax-number-computed.html", "max(1)", "1"},
	{"minmax-number-computed.html", "min(0.2, max(0.1, 0.15))", "0.15"},
	{"minmax-number-computed.html", "max(0.1, min(0.2, 0.15))", "0.15"},
	{"minmax-number-computed.html", "calc(min(0.1, 0.2) + 0.05)", "0.15"},
	{"minmax-number-computed.html", "calc(min(0.1, 0.2) - 0.05)", "0.05"},
	{"minmax-number-computed.html", "calc(min(0.1, 0.2) * 2)", "0.2"},
	{"minmax-number-computed.html", "calc(min(0.1, 0.2) / 2)", "0.05"},
	{"minmax-number-computed.html", "calc(max(0.1, 0.2) + 0.05)", "0.25"},
	{"minmax-number-computed.html", "calc(max(0.1, 0.2) - 0.05)", "0.15"},
	{"minmax-number-computed.html", "calc(max(0.1, 0.2) * 2)", "0.4"},
	{"minmax-number-computed.html", "calc(max(0.1, 0.2) / 2)", "0.1"},
	{"minmax-number-computed.html", "calc(min(0.1, 0.2) + max(0.1, 0.05))", "0.2"},
	{"minmax-number-computed.html", "calc(min(0.1, 0.2) - max(0.1, 0.05))", "0"},
};

// Sources:
//   css/css-values/calc-unit-analysis.html
//   css/css-values/calc-invalid-parsing.html
//   css/css-values/minmax-number-invalid.html
//   css/css-values/minmax-length-invalid.html
//   css/css-values/minmax-length-percent-invalid.html
//   css/css-values/clamp-length-invalid.html
constexpr InvalidCalculationCorpusCase pure_invalid_cases[] = {
	{"calc-unit-analysis.html", CalculationCorpusTarget::Length, "calc(0)"},
	{"calc-unit-analysis.html", CalculationCorpusTarget::Length, "calc(1px + 2)"},
	{"calc-unit-analysis.html", CalculationCorpusTarget::Length, "calc(2 + 1px)"},
	{"calc-unit-analysis.html", CalculationCorpusTarget::Length, "calc(1px - 2)"},
	{"calc-unit-analysis.html", CalculationCorpusTarget::Length, "calc(2 - 1px)"},
	{"calc-invalid-parsing.html", CalculationCorpusTarget::Length, "calc(7px * up)"},
	{"calc-invalid-parsing.html", CalculationCorpusTarget::Length, "calc([])"},
	{"minmax-number-invalid.html", CalculationCorpusTarget::Number, "min()"},
	{"minmax-number-invalid.html", CalculationCorpusTarget::Number, "min( )"},
	{"minmax-number-invalid.html", CalculationCorpusTarget::Number, "min(,)"},
	{"minmax-number-invalid.html", CalculationCorpusTarget::Number, "min(1, )"},
	{"minmax-number-invalid.html", CalculationCorpusTarget::Number, "min(, 1)"},
	{"minmax-number-invalid.html", CalculationCorpusTarget::Number, "min(1 + )"},
	{"minmax-number-invalid.html", CalculationCorpusTarget::Number, "min(1 - )"},
	{"minmax-number-invalid.html", CalculationCorpusTarget::Number, "min(1 * )"},
	{"minmax-number-invalid.html", CalculationCorpusTarget::Number, "min(1 / )"},
	{"minmax-number-invalid.html", CalculationCorpusTarget::Number, "min(1 2)"},
	{"minmax-number-invalid.html", CalculationCorpusTarget::Number, "min(1, , 2)"},
	{"minmax-number-invalid.html", CalculationCorpusTarget::Number, "max()"},
	{"minmax-number-invalid.html", CalculationCorpusTarget::Number, "max( )"},
	{"minmax-number-invalid.html", CalculationCorpusTarget::Number, "max(,)"},
	{"minmax-number-invalid.html", CalculationCorpusTarget::Number, "max(1, )"},
	{"minmax-number-invalid.html", CalculationCorpusTarget::Number, "max(, 1)"},
	{"minmax-number-invalid.html", CalculationCorpusTarget::Number, "max(1 + )"},
	{"minmax-number-invalid.html", CalculationCorpusTarget::Number, "max(1 - )"},
	{"minmax-number-invalid.html", CalculationCorpusTarget::Number, "max(1 * )"},
	{"minmax-number-invalid.html", CalculationCorpusTarget::Number, "max(1 / )"},
	{"minmax-number-invalid.html", CalculationCorpusTarget::Number, "max(1 2)"},
	{"minmax-number-invalid.html", CalculationCorpusTarget::Number, "max(1, , 2)"},
	{"minmax-number-invalid.html", CalculationCorpusTarget::Number, "min(0px)"},
	{"minmax-number-invalid.html", CalculationCorpusTarget::Number, "min(1, 1%)"},
	{"minmax-number-invalid.html", CalculationCorpusTarget::Number, "min(1, 0px)"},
	{"minmax-number-invalid.html", CalculationCorpusTarget::Number, "max(0px)"},
	{"minmax-number-invalid.html", CalculationCorpusTarget::Number, "max(1, 1%)"},
	{"minmax-number-invalid.html", CalculationCorpusTarget::Number, "max(1, 0px)"},
	{"minmax-length-percent-invalid.html", CalculationCorpusTarget::LengthPercentage, "min(1px, 0)"},
	{"minmax-length-percent-invalid.html", CalculationCorpusTarget::LengthPercentage, "min(1%, 0)"},
	{"minmax-length-percent-invalid.html", CalculationCorpusTarget::LengthPercentage, "max(1px, 0)"},
	{"minmax-length-percent-invalid.html", CalculationCorpusTarget::LengthPercentage, "max(1%, 0)"},
	{"clamp-length-invalid.html", CalculationCorpusTarget::Length, "clamp()"},
	{"clamp-length-invalid.html", CalculationCorpusTarget::Length, "clamp( )"},
	{"clamp-length-invalid.html", CalculationCorpusTarget::Length, "clamp(,)"},
	{"clamp-length-invalid.html", CalculationCorpusTarget::Length, "clamp(1px, )"},
	{"clamp-length-invalid.html", CalculationCorpusTarget::Length, "clamp(, 1px)"},
	{"clamp-length-invalid.html", CalculationCorpusTarget::Length, "clamp(1px, 1px)"},
	{"clamp-length-invalid.html", CalculationCorpusTarget::Length, "clamp(1px, , 1px)"},
	{"clamp-length-invalid.html", CalculationCorpusTarget::Length, "clamp(1px, 1px, )"},
	{"clamp-length-invalid.html", CalculationCorpusTarget::Length, "clamp(1px, 1px, 1px, )"},
	{"clamp-length-invalid.html", CalculationCorpusTarget::Length, "clamp(1px 1px 1px)"},
	{"clamp-length-invalid.html", CalculationCorpusTarget::Length, "clamp(none, none, none)"},
	{"clamp-length-invalid.html", CalculationCorpusTarget::Length, "clamp(none, none + none, none)"},
	{"clamp-length-invalid.html", CalculationCorpusTarget::Length, "clamp(-none, 1px, 1px)"},
	{"clamp-length-invalid.html", CalculationCorpusTarget::Length, "clamp(10px, none * none, 20px)"},
	{"clamp-length-invalid.html", CalculationCorpusTarget::Length, "clamp(10px, 2px + none, 20px)"},
};

// Sources:
//   css/css-values/calc-angle-values.html
// Only deg/rad vectors are in scope because RmlUi does not currently implement grad/turn.
constexpr CalculationCorpusCase angle_cases[] = {
	{"calc-angle-values.html", "calc(45deg + 45deg)", "90deg"},
	{"calc-angle-values.html", "calc(45deg + 1rad)", "102.29578deg"},
	{"calc-angle-values.html", "calc(45rad + 45rad)", "90rad"},
	{"calc-angle-values.html", "calc(45deg - 15deg)", "30deg"},
	{"calc-angle-values.html", "calc(90deg - 1rad)", "32.70422deg"},
	{"calc-angle-values.html", "calc(45rad - 15rad)", "30rad"},
	{"calc-angle-values.html", "calc(45deg * 2)", "90deg"},
	{"calc-angle-values.html", "calc(2 * 45rad)", "90rad"},
	{"calc-angle-values.html", "calc(90deg / 2)", "45deg"},
	{"calc-angle-values.html", "calc(90rad / 2)", "45rad"},
};

// Source: css/css-values/minmax-percentage-computed.html. These rows are basis-independent raw
// percentages, so these pure cases can be evaluated without a computed-value context.
constexpr CalculationCorpusCase pure_percentage_cases[] = {
	{"minmax-percentage-computed.html", "min(1%)", "1%"},
	{"minmax-percentage-computed.html", "max(1%)", "1%"},
	{"minmax-percentage-computed.html", "min(20%, max(10%, 15%))", "15%"},
	{"minmax-percentage-computed.html", "max(10%, min(20%, 15%))", "15%"},
	{"minmax-percentage-computed.html", "calc(min(10%, 20%) + 5%)", "15%"},
	{"minmax-percentage-computed.html", "calc(min(10%, 20%) - 5%)", "5%"},
	{"minmax-percentage-computed.html", "calc(min(10%, 20%) * 2)", "20%"},
	{"minmax-percentage-computed.html", "calc(min(10%, 20%) / 2)", "5%"},
	{"minmax-percentage-computed.html", "calc(max(10%, 20%) + 5%)", "25%"},
	{"minmax-percentage-computed.html", "calc(max(10%, 20%) - 5%)", "15%"},
	{"minmax-percentage-computed.html", "calc(max(10%, 20%) * 2)", "40%"},
	{"minmax-percentage-computed.html", "calc(max(10%, 20%) / 2)", "10%"},
	{"minmax-percentage-computed.html", "calc(min(10%, 20%) + max(10%, 5%))", "20%"},
	{"minmax-percentage-computed.html", "calc(min(11%, 20%) - max(10%, 5%))", "1%"},
};

// Source: css/css-values/minmax-length-computed.html. Relative-unit rows require a computed-value
// context; these are the complete supported absolute-unit rows whose WPT results are pure.
constexpr CalculationCorpusCase pure_absolute_length_cases[] = {
	{"minmax-length-computed.html", "min(1px)", "1px"},
	{"minmax-length-computed.html", "min(1cm)", "1cm"},
	{"minmax-length-computed.html", "min(1mm)", "1mm"},
	{"minmax-length-computed.html", "min(1in)", "1in"},
	{"minmax-length-computed.html", "min(1pc)", "1pc"},
	{"minmax-length-computed.html", "min(1pt)", "1pt"},
	{"minmax-length-computed.html", "max(1px)", "1px"},
	{"minmax-length-computed.html", "max(1cm)", "1cm"},
	{"minmax-length-computed.html", "max(1mm)", "1mm"},
	{"minmax-length-computed.html", "max(1in)", "1in"},
	{"minmax-length-computed.html", "max(1pc)", "1pc"},
	{"minmax-length-computed.html", "max(1pt)", "1pt"},
	{"minmax-length-computed.html", "min(1px, 2px)", "1px"},
	{"minmax-length-computed.html", "min(1cm, 2cm)", "1cm"},
	{"minmax-length-computed.html", "min(1mm, 2mm)", "1mm"},
	{"minmax-length-computed.html", "min(1in, 2in)", "1in"},
	{"minmax-length-computed.html", "min(1pc, 2pc)", "1pc"},
	{"minmax-length-computed.html", "min(1pt, 2pt)", "1pt"},
	{"minmax-length-computed.html", "max(1px, 2px)", "2px"},
	{"minmax-length-computed.html", "max(1cm, 2cm)", "2cm"},
	{"minmax-length-computed.html", "max(1mm, 2mm)", "2mm"},
	{"minmax-length-computed.html", "max(1in, 2in)", "2in"},
	{"minmax-length-computed.html", "max(1pc, 2pc)", "2pc"},
	{"minmax-length-computed.html", "max(1pt, 2pt)", "2pt"},
	{"minmax-length-computed.html", "min(95px, 1in)", "95px"},
	{"minmax-length-computed.html", "max(95px, 1in)", "1in"},
};

// Source: css/css-values/minmax-angle-computed.html. RmlUi does not support grad/turn units here;
// all remaining deg/rad rows are pure evaluation cases.
constexpr CalculationCorpusCase pure_minmax_angle_cases[] = {
	{"minmax-angle-computed.html", "min(1deg)", "1deg"},
	{"minmax-angle-computed.html", "min(1rad)", "1rad"},
	{"minmax-angle-computed.html", "max(1deg)", "1deg"},
	{"minmax-angle-computed.html", "max(1rad)", "1rad"},
	{"minmax-angle-computed.html", "min(1deg, 2deg)", "1deg"},
	{"minmax-angle-computed.html", "min(1rad, 2rad)", "1rad"},
	{"minmax-angle-computed.html", "max(1deg, 2deg)", "2deg"},
	{"minmax-angle-computed.html", "max(1rad, 2rad)", "2rad"},
	{"minmax-angle-computed.html", "min(1.57rad, 95deg)", "1.57rad"},
	{"minmax-angle-computed.html", "max(1.58rad, 90deg)", "1.58rad"},
	{"minmax-angle-computed.html", "calc(min(90deg, 1.58rad) * 1.5", "135deg"},
	{"minmax-angle-computed.html", "calc(min(90deg, 1.58rad) / 2", "45deg"},
	{"minmax-angle-computed.html", "calc(max(90deg, 1.56rad) * 1.5", "135deg"},
	{"minmax-angle-computed.html", "calc(max(90deg, 1.56rad) / 2", "45deg"},
};

// Source: css/css-values/clamp-length-computed.html. The final em/rem rows are context-dependent;
// every preceding px-only row has a pure expected result.
constexpr CalculationCorpusCase pure_clamp_length_cases[] = {
	{"clamp-length-computed.html", "clamp(10px, 20px, 30px)", "20px"},
	{"clamp-length-computed.html", "clamp(10px, 5px, 30px)", "10px"},
	{"clamp-length-computed.html", "clamp(10px, 35px, 30px)", "30px"},
	{"clamp-length-computed.html", "clamp(10px, 35px , 30px)", "30px"},
	{"clamp-length-computed.html", "clamp(10px, 35px /*foo*/, 30px)", "30px"},
	{"clamp-length-computed.html", "clamp(10px /* foo */ , 35px, 30px)", "30px"},
	{"clamp-length-computed.html", "clamp(10px , 35px, 30px)", "30px"},
	{"clamp-length-computed.html", "clamp(30px, 100px, 20px)", "30px"},
	{"clamp-length-computed.html", "clamp(-30px, -20px, -10px)", "-20px"},
	{"clamp-length-computed.html", "clamp(-30px, -100px, -10px)", "-30px"},
	{"clamp-length-computed.html", "clamp(-30px, 100px, -10px)", "-10px"},
	{"clamp-length-computed.html", "clamp(-10px, 100px, -30px)", "-10px"},
	{"clamp-length-computed.html", "clamp(-10px, -100px, -30px)", "-10px"},
	{"clamp-length-computed.html", "calc(0px + clamp(10px, 20px, 30px))", "20px"},
	{"clamp-length-computed.html", "calc(0px - clamp(10px, 20px, 30px))", "-20px"},
	{"clamp-length-computed.html", "calc(0px + clamp(30px, 100px, 20px))", "30px"},
	{"clamp-length-computed.html", "calc(0px - clamp(30px, 100px, 20px))", "-30px"},
	{"clamp-length-computed.html", "clamp(none, 30px, 33px)", "30px"},
	{"clamp-length-computed.html", "clamp(none, 33px, 30px)", "30px"},
	{"clamp-length-computed.html", "clamp(30px, 33px, none)", "33px"},
	{"clamp-length-computed.html", "clamp(33px, 30px, none)", "33px"},
	{"clamp-length-computed.html", "clamp(none, 30px, none)", "30px"},
};

// Sources:
//   css/css-values/calc-time-values.html
//   css/css-values/minmax-time-computed.html
constexpr CalculationCorpusCase time_cases[] = {
	{"calc-time-values.html", "calc(4s + 1s)", "5s"},
	{"calc-time-values.html", "calc(4s + 1ms)", "4.001s"},
	{"calc-time-values.html", "calc(4ms + 1ms)", "0.005s"},
	{"calc-time-values.html", "calc(4s - 1s)", "3s"},
	{"calc-time-values.html", "calc(4s - 1ms)", "3.999s"},
	{"calc-time-values.html", "calc(4 * 1s)", "4s"},
	{"calc-time-values.html", "calc(4 * 1ms)", "0.004s"},
	{"calc-time-values.html", "calc(4s / 2)", "2s"},
	{"calc-time-values.html", "calc(4ms / 2)", "0.002s"},
	{"calc-time-values.html", "calc(4000ms)", "4s"},
	{"minmax-time-computed.html", "min(1s)", "1s"},
	{"minmax-time-computed.html", "min(1ms)", "1ms"},
	{"minmax-time-computed.html", "max(1s)", "1s"},
	{"minmax-time-computed.html", "max(1ms)", "1ms"},
	{"minmax-time-computed.html", "min(1s, 2s)", "1s"},
	{"minmax-time-computed.html", "min(1ms, 2ms)", "1ms"},
	{"minmax-time-computed.html", "max(1s, 2s)", "2s"},
	{"minmax-time-computed.html", "max(1ms, 2ms)", "2ms"},
	{"minmax-time-computed.html", "min(1s, 1100ms)", "1s"},
	{"minmax-time-computed.html", "max(0.9s, 1000ms)", "1000ms"},
	{"minmax-time-computed.html", "min(2s, max(1s, 1500ms))", "1500ms"},
	{"minmax-time-computed.html", "max(1000ms, min(2000ms, 1.5s))", "1.5s"},
	{"minmax-time-computed.html", "calc(min(0.5s, 600ms) + 500ms)", "1s"},
	{"minmax-time-computed.html", "calc(min(0.6s, 700ms) - 500ms)", "0.1s"},
	{"minmax-time-computed.html", "calc(min(0.5s, 600ms) * 2)", "1s"},
	{"minmax-time-computed.html", "calc(min(0.5s, 600ms) / 2)", "0.25s"},
	{"minmax-time-computed.html", "calc(max(0.5s, 400ms) + 500ms)", "1s"},
	{"minmax-time-computed.html", "calc(max(0.5s, 400ms) - 400ms)", "0.1s"},
	{"minmax-time-computed.html", "calc(max(0.5s, 400ms) * 2)", "1s"},
	{"minmax-time-computed.html", "calc(max(0.5s, 400ms) / 2)", "0.25s"},
	{"minmax-time-computed.html", "calc(min(0.5s, 600ms) + max(500ms, 0.4s))", "1s"},
	{"minmax-time-computed.html", "calc(min(0.6s, 700ms) - max(500ms, 0.4s))", "0.1s"},
	{"minmax-time-computed.html", "min(1s + 100ms, 500ms * 3)", "1.1s"},
	{"minmax-time-computed.html", "calc(min(1s, 2s) + max(3s, 4s) + 10s)", "15s"},
};

// Source: css/css-values/minmax-time-invalid.html. Rows using Hz/dpi/fr are excluded by the
// unsupported-unit cases are omitted; every remaining adapted row is executable.
constexpr const char* time_invalid_cases[] = {
	"min()",
	"min( )",
	"min(,)",
	"min(1mt)",
	"min(1s, )",
	"min(, 1s)",
	"min(1s + )",
	"min(1s - )",
	"min(1s * )",
	"min(1s / )",
	"min(1s 2s)",
	"min(1s, , 2s)",
	"max()",
	"max( )",
	"max(,)",
	"max(1dag)",
	"max(1s, )",
	"max(, 1s)",
	"max(1s + )",
	"max(1s - )",
	"max(1s * )",
	"max(1s / )",
	"max(1s 2s)",
	"max(1s, , 2s)",
	"min(0)",
	"min(0%)",
	"min(0px)",
	"min(0deg)",
	"min(1s, 0)",
	"min(1s, 0%)",
	"min(1s, 0px)",
	"min(1s, 0deg)",
	"max(0)",
	"max(0%)",
	"max(0px)",
	"max(0deg)",
	"max(1s, 0)",
	"max(1s, 0%)",
	"max(1s, 0px)",
	"max(1s, 0deg)",
};

constexpr CalculationCorpusCase time_clamp_cases[] = {
	{"plan-required", "clamp(1s, 2s, 3s)", "2s"},
	{"plan-required", "clamp(none, 2s, 3s)", "2s"},
	{"plan-required", "clamp(1s, 2s, none)", "2s"},
	{"plan-required", "clamp(none, 2s, none)", "2s"},
	{"plan-required", "clamp(3s, 2s, 1s)", "3s"},
};

// Sources:
//   css/css-values/minmax-length-computed.html
//   css/css-values/clamp-length-computed.html
constexpr CalculationCorpusCase definite_length_cases[] = {
	{"minmax-length-computed.html", "min(1px, 2px)", "1px"},
	{"minmax-length-computed.html", "max(1px, 2px)", "2px"},
	{"minmax-length-computed.html", "min(95px, 1in)", "95px"},
	{"minmax-length-computed.html", "max(95px, 1in)", "96px"},
	{"minmax-length-computed.html", "min(15px, 1em)", "15px"},
	{"minmax-length-computed.html", "min(25px, 1em)", "20px"},
	{"minmax-length-computed.html", "max(15px, 1em)", "20px"},
	{"minmax-length-computed.html", "max(25px, 1em)", "25px"},
	{"minmax-length-computed.html", "min(25px, max(15px, 1em))", "20px"},
	{"minmax-length-computed.html", "max(15px, min(25px, 1em))", "20px"},
	{"minmax-length-computed.html", "calc(min(1em, 21px) + 10px)", "30px"},
	{"minmax-length-computed.html", "calc(min(1em, 21px) - 10px)", "10px"},
	{"minmax-length-computed.html", "calc(max(1em, 19px) + 10px)", "30px"},
	{"adapted:minmax-length-computed.html", "calc(max(1em, 19px) / 2)", "10px"},
	{"clamp-length-computed.html", "clamp(10px, 20px, 30px)", "20px"},
	{"clamp-length-computed.html", "clamp(10px, 5px, 30px)", "10px"},
	{"clamp-length-computed.html", "clamp(10px, 35px, 30px)", "30px"},
	{"clamp-length-computed.html", "clamp(30px, 100px, 20px)", "30px"},
	{"clamp-length-computed.html", "clamp(-30px, -20px, -10px)", "-20px"},
	{"clamp-length-computed.html", "calc(0px - clamp(10px, 20px, 30px))", "-20px"},
	{"clamp-length-computed.html", "clamp(none, 30px, 33px)", "30px"},
	{"clamp-length-computed.html", "clamp(none, 33px, 30px)", "30px"},
	{"clamp-length-computed.html", "clamp(30px, 33px, none)", "33px"},
	{"clamp-length-computed.html", "clamp(33px, 30px, none)", "33px"},
	{"clamp-length-computed.html", "clamp(none, 30px, none)", "30px"},
};

// Sources:
//   css/css-values/minmax-length-percent-computed.html
//   css/css-values/max-20-arguments.html
//   css/css-values/calc-nesting.html
constexpr CalculationCorpusCase used_length_percentage_cases[] = {
	{"minmax-length-percent-computed.html", "min(20px, 10%)", "basis 400px -> 20px; basis 100px -> 10px"},
	{"minmax-length-percent-computed.html", "max(20px, 10%)", "basis 400px -> 40px; basis 100px -> 20px"},
	{"minmax-length-percent-computed.html", "min(30px + 10%, 60px + 5%)", "basis 400px -> 70px"},
	{"minmax-length-percent-computed.html", "max(2em + 10%, 1em + 20%)", "font 20px/basis 400px -> 100px"},
	{"minmax-length-percent-computed.html", "calc(min(1.5em, 10%) + 10px)", "font 20px/basis 400px -> 40px"},
	{"minmax-length-percent-computed.html", "calc(min(1.5em, 10%) - 10px)", "font 20px/basis 400px -> 20px"},
	{"minmax-length-percent-computed.html", "calc(min(1.5em, 10%) * 2)", "font 20px/basis 400px -> 60px"},
	{"minmax-length-percent-computed.html", "calc(min(1.5em, 10%) / 2)", "font 20px/basis 400px -> 15px"},
	{"minmax-length-percent-computed.html", "calc(max(1em, 15%) + 10px)", "font 20px/basis 400px -> 70px"},
	{"minmax-length-percent-computed.html", "calc(max(1em, 15%) / 2)", "font 20px/basis 400px -> 30px"},
	{"max-20-arguments.html", "max(5%, 10%, 15%, 20%, 25%, 30%, 35%, 40%, 45%, 50%, 55%, 60%, 65%, 70%, 75%, 80%, 85%, 90%, 95%, 100%)", "100%"},
	{"calc-nesting.html", "calc(calc(60%) - 20px)", "nested length-percentage"},
	{"calc-nesting.html", "calc(calc(3 * 25%))", "75%"},
};

// Sources:
//   css/css-values/clamp-none-whitespace.html
constexpr const char* clamp_none_whitespace_cases[] = {
	"clamp(none, 5px, 10px)",
	"clamp(none , 5px, 10px)",
	"clamp(5px, 6px, none)",
	"clamp(5px, 6px, none )",
	"clamp(none, 5px, none)",
	"clamp( none , 5px , none )",
};

// Sources:
//   css/css-values/typed_arithmetic.html
// These are the finite, in-scope dimensional-cancellation vectors. sign()/pow()/NaN/infinity rows
// from the WPT source use unsupported functions and are intentionally omitted.
constexpr CalculationCorpusCase typed_arithmetic_cases[] = {
	{"typed_arithmetic.html", "min(1em, 110px / 10px * 1px)", "font 20px -> 10px"},
	{"typed_arithmetic.html", "max(10px, 110px / 10px * 1px)", "11px"},
	{"typed_arithmetic.html", "max(1em + 2px, 110px / 10px * 1px)", "font 10px -> 12px"},
	{"typed_arithmetic.html", "calc(10em / 1em)", "10"},
	{"typed_arithmetic.html", "calc(10em / 1rem)", "context-equal em/rem -> 10"},
	{"typed_arithmetic.html", "calc(1px * 2deg / 1deg)", "2px"},
	{"typed_arithmetic.html", "calc(1px * 3deg / 1deg / 1px)", "3"},
};

// Exhaustive adapted parser/type rows from the supported WPT math-function corpus. Context-dependent
// rows assert parse/type behavior here and are covered separately by computed/used-value fixtures.
constexpr WptParseCase wpt_valid_parse_cases[] = {
	// css/css-values/minmax-number-computed.html
	{CalculationCorpusTarget::Number, "minmax-number-computed.html", "min(1)"},
	{CalculationCorpusTarget::Number, "minmax-number-computed.html", "max(1)"},
	{CalculationCorpusTarget::Number, "minmax-number-computed.html", "min(0.2, max(0.1, 0.15))"},
	{CalculationCorpusTarget::Number, "minmax-number-computed.html", "max(0.1, min(0.2, 0.15))"},
	{CalculationCorpusTarget::Number, "minmax-number-computed.html", "calc(min(0.1, 0.2) + 0.05)"},
	{CalculationCorpusTarget::Number, "minmax-number-computed.html", "calc(min(0.1, 0.2) - 0.05)"},
	{CalculationCorpusTarget::Number, "minmax-number-computed.html", "calc(min(0.1, 0.2) * 2)"},
	{CalculationCorpusTarget::Number, "minmax-number-computed.html", "calc(min(0.1, 0.2) / 2)"},
	{CalculationCorpusTarget::Number, "minmax-number-computed.html", "calc(max(0.1, 0.2) + 0.05)"},
	{CalculationCorpusTarget::Number, "minmax-number-computed.html", "calc(max(0.1, 0.2) - 0.05)"},
	{CalculationCorpusTarget::Number, "minmax-number-computed.html", "calc(max(0.1, 0.2) * 2)"},
	{CalculationCorpusTarget::Number, "minmax-number-computed.html", "calc(max(0.1, 0.2) / 2)"},
	{CalculationCorpusTarget::Number, "minmax-number-computed.html", "calc(min(0.1, 0.2) + max(0.1, 0.05))"},
	{CalculationCorpusTarget::Number, "minmax-number-computed.html", "calc(min(0.1, 0.2) - max(0.1, 0.05))"},

	// css/css-values/minmax-percentage-computed.html
	{CalculationCorpusTarget::Percentage, "minmax-percentage-computed.html", "min(1%)"},
	{CalculationCorpusTarget::Percentage, "minmax-percentage-computed.html", "max(1%)"},
	{CalculationCorpusTarget::Percentage, "minmax-percentage-computed.html", "min(20%, max(10%, 15%))"},
	{CalculationCorpusTarget::Percentage, "minmax-percentage-computed.html", "max(10%, min(20%, 15%))"},
	{CalculationCorpusTarget::Percentage, "minmax-percentage-computed.html", "calc(min(10%, 20%) + 5%)"},
	{CalculationCorpusTarget::Percentage, "minmax-percentage-computed.html", "calc(min(10%, 20%) - 5%)"},
	{CalculationCorpusTarget::Percentage, "minmax-percentage-computed.html", "calc(min(10%, 20%) * 2)"},
	{CalculationCorpusTarget::Percentage, "minmax-percentage-computed.html", "calc(min(10%, 20%) / 2)"},
	{CalculationCorpusTarget::Percentage, "minmax-percentage-computed.html", "calc(max(10%, 20%) + 5%)"},
	{CalculationCorpusTarget::Percentage, "minmax-percentage-computed.html", "calc(max(10%, 20%) - 5%)"},
	{CalculationCorpusTarget::Percentage, "minmax-percentage-computed.html", "calc(max(10%, 20%) * 2)"},
	{CalculationCorpusTarget::Percentage, "minmax-percentage-computed.html", "calc(max(10%, 20%) / 2)"},
	{CalculationCorpusTarget::Percentage, "minmax-percentage-computed.html", "calc(min(10%, 20%) + max(10%, 5%))"},
	{CalculationCorpusTarget::Percentage, "minmax-percentage-computed.html", "calc(min(11%, 20%) - max(10%, 5%))"},

	// css/css-values/minmax-length-computed.html, excluding unsupported Q/ex/ch/vmin/vmax rows.
	{CalculationCorpusTarget::Length, "minmax-length-computed.html", "min(1px)"},
	{CalculationCorpusTarget::Length, "minmax-length-computed.html", "min(1cm)"},
	{CalculationCorpusTarget::Length, "minmax-length-computed.html", "min(1mm)"},
	{CalculationCorpusTarget::Length, "minmax-length-computed.html", "min(1in)"},
	{CalculationCorpusTarget::Length, "minmax-length-computed.html", "min(1pc)"},
	{CalculationCorpusTarget::Length, "minmax-length-computed.html", "min(1pt)"},
	{CalculationCorpusTarget::Length, "minmax-length-computed.html", "min(1em)"},
	{CalculationCorpusTarget::Length, "minmax-length-computed.html", "min(1rem)"},
	{CalculationCorpusTarget::Length, "minmax-length-computed.html", "min(1vh)"},
	{CalculationCorpusTarget::Length, "minmax-length-computed.html", "min(1vw)"},
	{CalculationCorpusTarget::Length, "minmax-length-computed.html", "max(1px)"},
	{CalculationCorpusTarget::Length, "minmax-length-computed.html", "max(1cm)"},
	{CalculationCorpusTarget::Length, "minmax-length-computed.html", "max(1mm)"},
	{CalculationCorpusTarget::Length, "minmax-length-computed.html", "max(1in)"},
	{CalculationCorpusTarget::Length, "minmax-length-computed.html", "max(1pc)"},
	{CalculationCorpusTarget::Length, "minmax-length-computed.html", "max(1pt)"},
	{CalculationCorpusTarget::Length, "minmax-length-computed.html", "max(1em)"},
	{CalculationCorpusTarget::Length, "minmax-length-computed.html", "max(1rem)"},
	{CalculationCorpusTarget::Length, "minmax-length-computed.html", "max(1vh)"},
	{CalculationCorpusTarget::Length, "minmax-length-computed.html", "max(1vw)"},
	{CalculationCorpusTarget::Length, "minmax-length-computed.html", "min(1px, 2px)"},
	{CalculationCorpusTarget::Length, "minmax-length-computed.html", "min(1cm, 2cm)"},
	{CalculationCorpusTarget::Length, "minmax-length-computed.html", "min(1mm, 2mm)"},
	{CalculationCorpusTarget::Length, "minmax-length-computed.html", "min(1in, 2in)"},
	{CalculationCorpusTarget::Length, "minmax-length-computed.html", "min(1pc, 2pc)"},
	{CalculationCorpusTarget::Length, "minmax-length-computed.html", "min(1pt, 2pt)"},
	{CalculationCorpusTarget::Length, "minmax-length-computed.html", "min(1em, 2em)"},
	{CalculationCorpusTarget::Length, "minmax-length-computed.html", "min(1rem, 2rem)"},
	{CalculationCorpusTarget::Length, "minmax-length-computed.html", "min(1vh, 2vh)"},
	{CalculationCorpusTarget::Length, "minmax-length-computed.html", "min(1vw, 2vw)"},
	{CalculationCorpusTarget::Length, "minmax-length-computed.html", "max(1px, 2px)"},
	{CalculationCorpusTarget::Length, "minmax-length-computed.html", "max(1cm, 2cm)"},
	{CalculationCorpusTarget::Length, "minmax-length-computed.html", "max(1mm, 2mm)"},
	{CalculationCorpusTarget::Length, "minmax-length-computed.html", "max(1in, 2in)"},
	{CalculationCorpusTarget::Length, "minmax-length-computed.html", "max(1pc, 2pc)"},
	{CalculationCorpusTarget::Length, "minmax-length-computed.html", "max(1pt, 2pt)"},
	{CalculationCorpusTarget::Length, "minmax-length-computed.html", "max(1em, 2em)"},
	{CalculationCorpusTarget::Length, "minmax-length-computed.html", "max(1rem, 2rem)"},
	{CalculationCorpusTarget::Length, "minmax-length-computed.html", "max(1vh, 2vh)"},
	{CalculationCorpusTarget::Length, "minmax-length-computed.html", "max(1vw, 2vw)"},
	{CalculationCorpusTarget::Length, "minmax-length-computed.html", "min(95px, 1in)"},
	{CalculationCorpusTarget::Length, "minmax-length-computed.html", "max(95px, 1in)"},
	{CalculationCorpusTarget::Length, "minmax-length-computed.html", "min(15px, 1em)"},
	{CalculationCorpusTarget::Length, "minmax-length-computed.html", "min(25px, 1em)"},
	{CalculationCorpusTarget::Length, "minmax-length-computed.html", "max(15px, 1em)"},
	{CalculationCorpusTarget::Length, "minmax-length-computed.html", "max(25px, 1em)"},
	{CalculationCorpusTarget::Length, "minmax-length-computed.html", "min(15px, 1em)"},
	{CalculationCorpusTarget::Length, "minmax-length-computed.html", "max(15px, 2em)"},
	{CalculationCorpusTarget::Length, "minmax-length-computed.html", "min(25px, max(15px, 1em))"},
	{CalculationCorpusTarget::Length, "minmax-length-computed.html", "max(15px, min(25px, 1em))"},
	{CalculationCorpusTarget::Length, "minmax-length-computed.html", "calc(min(1em, 21px) + 10px)"},
	{CalculationCorpusTarget::Length, "minmax-length-computed.html", "calc(min(1em, 21px) - 10px)"},
	{CalculationCorpusTarget::Length, "minmax-length-computed.html", "calc(min(1em, 21px) * 2"},
	{CalculationCorpusTarget::Length, "minmax-length-computed.html", "calc(min(1em, 21px) / 2"},
	{CalculationCorpusTarget::Length, "minmax-length-computed.html", "calc(max(1em, 19px) + 10px)"},
	{CalculationCorpusTarget::Length, "minmax-length-computed.html", "calc(max(1em, 19px) - 10px)"},
	{CalculationCorpusTarget::Length, "minmax-length-computed.html", "calc(max(1em, 19px) * 2"},
	{CalculationCorpusTarget::Length, "minmax-length-computed.html", "calc(max(1em, 19px) / 2"},
	{CalculationCorpusTarget::Length, "minmax-length-computed.html", "calc(min(1em, 21px) + max(0.9em, 20px))"},
	{CalculationCorpusTarget::Length, "minmax-length-computed.html", "calc(min(1em + 1px, 22px) - max(0.9em, 20px))"},

	// css/css-values/minmax-length-percent-computed.html, same supported-unit filter.
	{CalculationCorpusTarget::LengthPercentage, "minmax-length-percent-computed.html", "min(1px + 1%)"},
	{CalculationCorpusTarget::LengthPercentage, "minmax-length-percent-computed.html", "min(1cm + 1%)"},
	{CalculationCorpusTarget::LengthPercentage, "minmax-length-percent-computed.html", "min(1mm + 1%)"},
	{CalculationCorpusTarget::LengthPercentage, "minmax-length-percent-computed.html", "min(1in + 1%)"},
	{CalculationCorpusTarget::LengthPercentage, "minmax-length-percent-computed.html", "min(1pc + 1%)"},
	{CalculationCorpusTarget::LengthPercentage, "minmax-length-percent-computed.html", "min(1pt + 1%)"},
	{CalculationCorpusTarget::LengthPercentage, "minmax-length-percent-computed.html", "min(1em + 1%)"},
	{CalculationCorpusTarget::LengthPercentage, "minmax-length-percent-computed.html", "min(1rem + 1%)"},
	{CalculationCorpusTarget::LengthPercentage, "minmax-length-percent-computed.html", "min(1vh + 1%)"},
	{CalculationCorpusTarget::LengthPercentage, "minmax-length-percent-computed.html", "min(1vw + 1%)"},
	{CalculationCorpusTarget::LengthPercentage, "minmax-length-percent-computed.html", "max(1px + 1%)"},
	{CalculationCorpusTarget::LengthPercentage, "minmax-length-percent-computed.html", "max(1cm + 1%)"},
	{CalculationCorpusTarget::LengthPercentage, "minmax-length-percent-computed.html", "max(1mm + 1%)"},
	{CalculationCorpusTarget::LengthPercentage, "minmax-length-percent-computed.html", "max(1in + 1%)"},
	{CalculationCorpusTarget::LengthPercentage, "minmax-length-percent-computed.html", "max(1pc + 1%)"},
	{CalculationCorpusTarget::LengthPercentage, "minmax-length-percent-computed.html", "max(1pt + 1%)"},
	{CalculationCorpusTarget::LengthPercentage, "minmax-length-percent-computed.html", "max(1em + 1%)"},
	{CalculationCorpusTarget::LengthPercentage, "minmax-length-percent-computed.html", "max(1rem + 1%)"},
	{CalculationCorpusTarget::LengthPercentage, "minmax-length-percent-computed.html", "max(1vh + 1%)"},
	{CalculationCorpusTarget::LengthPercentage, "minmax-length-percent-computed.html", "max(1vw + 1%)"},
	{CalculationCorpusTarget::LengthPercentage, "minmax-length-percent-computed.html", "min(20px, 10%)"},
	{CalculationCorpusTarget::LengthPercentage, "minmax-length-percent-computed.html", "min(1em, 10%)"},
	{CalculationCorpusTarget::LengthPercentage, "minmax-length-percent-computed.html", "max(20px, 10%)"},
	{CalculationCorpusTarget::LengthPercentage, "minmax-length-percent-computed.html", "max(1em, 10%)"},
	{CalculationCorpusTarget::LengthPercentage, "minmax-length-percent-computed.html", "min(20px, 10%)"},
	{CalculationCorpusTarget::LengthPercentage, "minmax-length-percent-computed.html", "min(1em, 10%)"},
	{CalculationCorpusTarget::LengthPercentage, "minmax-length-percent-computed.html", "max(20px, 10%)"},
	{CalculationCorpusTarget::LengthPercentage, "minmax-length-percent-computed.html", "max(1em, 10%)"},
	{CalculationCorpusTarget::LengthPercentage, "minmax-length-percent-computed.html", "min(30px + 10%, 60px + 5%)"},
	{CalculationCorpusTarget::LengthPercentage, "minmax-length-percent-computed.html", "max(2em + 10%, 1em + 20%)"},
	{CalculationCorpusTarget::LengthPercentage, "minmax-length-percent-computed.html", "calc(min(1.5em, 10%) + 10px)"},
	{CalculationCorpusTarget::LengthPercentage, "minmax-length-percent-computed.html", "calc(min(1.5em, 10%) - 10px)"},
	{CalculationCorpusTarget::LengthPercentage, "minmax-length-percent-computed.html", "calc(min(1.5em, 10%) * 2)"},
	{CalculationCorpusTarget::LengthPercentage, "minmax-length-percent-computed.html", "calc(min(1.5em, 10%) / 2)"},
	{CalculationCorpusTarget::LengthPercentage, "minmax-length-percent-computed.html", "calc(max(1em, 15%) + 10px)"},
	{CalculationCorpusTarget::LengthPercentage, "minmax-length-percent-computed.html", "calc(max(1em, 15%) - 10px)"},
	{CalculationCorpusTarget::LengthPercentage, "minmax-length-percent-computed.html", "calc(max(1em, 15%) * 2)"},
	{CalculationCorpusTarget::LengthPercentage, "minmax-length-percent-computed.html", "calc(max(1em, 15%) / 2)"},
	{CalculationCorpusTarget::LengthPercentage, "minmax-length-percent-computed.html", "calc(min(1.5em, 10%) + max(1em, 15%))"},
	{CalculationCorpusTarget::LengthPercentage, "minmax-length-percent-computed.html", "calc(min(1.5em, 10%) - max(1em, 15%))"},

	// css/css-values/minmax-angle-computed.html, excluding grad/turn rows.
	{CalculationCorpusTarget::Angle, "minmax-angle-computed.html", "min(1deg)"},
	{CalculationCorpusTarget::Angle, "minmax-angle-computed.html", "min(1rad)"},
	{CalculationCorpusTarget::Angle, "minmax-angle-computed.html", "max(1deg)"},
	{CalculationCorpusTarget::Angle, "minmax-angle-computed.html", "max(1rad)"},
	{CalculationCorpusTarget::Angle, "minmax-angle-computed.html", "min(1deg, 2deg)"},
	{CalculationCorpusTarget::Angle, "minmax-angle-computed.html", "min(1rad, 2rad)"},
	{CalculationCorpusTarget::Angle, "minmax-angle-computed.html", "max(1deg, 2deg)"},
	{CalculationCorpusTarget::Angle, "minmax-angle-computed.html", "max(1rad, 2rad)"},
	{CalculationCorpusTarget::Angle, "minmax-angle-computed.html", "min(1.57rad, 95deg)"},
	{CalculationCorpusTarget::Angle, "minmax-angle-computed.html", "max(1.58rad, 90deg)"},
	{CalculationCorpusTarget::Angle, "minmax-angle-computed.html", "calc(min(90deg, 1.58rad) * 1.5"},
	{CalculationCorpusTarget::Angle, "minmax-angle-computed.html", "calc(min(90deg, 1.58rad) / 2"},
	{CalculationCorpusTarget::Angle, "minmax-angle-computed.html", "calc(max(90deg, 1.56rad) * 1.5"},
	{CalculationCorpusTarget::Angle, "minmax-angle-computed.html", "calc(max(90deg, 1.56rad) / 2"},

	// css/css-values/clamp-length-computed.html
	{CalculationCorpusTarget::Length, "clamp-length-computed.html", "clamp(10px, 20px, 30px)"},
	{CalculationCorpusTarget::Length, "clamp-length-computed.html", "clamp(10px, 5px, 30px)"},
	{CalculationCorpusTarget::Length, "clamp-length-computed.html", "clamp(10px, 35px, 30px)"},
	{CalculationCorpusTarget::Length, "clamp-length-computed.html", "clamp(10px, 35px , 30px)"},
	{CalculationCorpusTarget::Length, "clamp-length-computed.html", "clamp(10px, 35px /*foo*/, 30px)"},
	{CalculationCorpusTarget::Length, "clamp-length-computed.html", "clamp(10px /* foo */ , 35px, 30px)"},
	{CalculationCorpusTarget::Length, "clamp-length-computed.html", "clamp(10px , 35px, 30px)"},
	{CalculationCorpusTarget::Length, "clamp-length-computed.html", "clamp(30px, 100px, 20px)"},
	{CalculationCorpusTarget::Length, "clamp-length-computed.html", "clamp(-30px, -20px, -10px)"},
	{CalculationCorpusTarget::Length, "clamp-length-computed.html", "clamp(-30px, -100px, -10px)"},
	{CalculationCorpusTarget::Length, "clamp-length-computed.html", "clamp(-30px, 100px, -10px)"},
	{CalculationCorpusTarget::Length, "clamp-length-computed.html", "clamp(-10px, 100px, -30px)"},
	{CalculationCorpusTarget::Length, "clamp-length-computed.html", "clamp(-10px, -100px, -30px)"},
	{CalculationCorpusTarget::Length, "clamp-length-computed.html", "calc(0px + clamp(10px, 20px, 30px))"},
	{CalculationCorpusTarget::Length, "clamp-length-computed.html", "calc(0px - clamp(10px, 20px, 30px))"},
	{CalculationCorpusTarget::Length, "clamp-length-computed.html", "calc(0px + clamp(30px, 100px, 20px))"},
	{CalculationCorpusTarget::Length, "clamp-length-computed.html", "calc(0px - clamp(30px, 100px, 20px))"},
	{CalculationCorpusTarget::Length, "clamp-length-computed.html", "clamp(none, 30px, 33px)"},
	{CalculationCorpusTarget::Length, "clamp-length-computed.html", "clamp(none, 33px, 30px)"},
	{CalculationCorpusTarget::Length, "clamp-length-computed.html", "clamp(30px, 33px, none)"},
	{CalculationCorpusTarget::Length, "clamp-length-computed.html", "clamp(33px, 30px, none)"},
	{CalculationCorpusTarget::Length, "clamp-length-computed.html", "clamp(none, 30px, none)"},
	{CalculationCorpusTarget::Length, "clamp-length-computed.html", "clamp(1000px, 1em / 1rem * 1px, none)"},
	{CalculationCorpusTarget::Length, "clamp-length-computed.html", "clamp(1600px / 1em * 1px, 1em / 1rem * 1px, none)"},
};

constexpr WptParseCase wpt_invalid_parse_cases[] = {
	// css/css-values/minmax-number-invalid.html, excluding unsupported Hz/dpi/fr rows.
	{CalculationCorpusTarget::Number, "minmax-number-invalid.html", "min()"},
	{CalculationCorpusTarget::Number, "minmax-number-invalid.html", "min( )"},
	{CalculationCorpusTarget::Number, "minmax-number-invalid.html", "min(,)"},
	{CalculationCorpusTarget::Number, "minmax-number-invalid.html", "min(1, )"},
	{CalculationCorpusTarget::Number, "minmax-number-invalid.html", "min(, 1)"},
	{CalculationCorpusTarget::Number, "minmax-number-invalid.html", "min(1 + )"},
	{CalculationCorpusTarget::Number, "minmax-number-invalid.html", "min(1 - )"},
	{CalculationCorpusTarget::Number, "minmax-number-invalid.html", "min(1 * )"},
	{CalculationCorpusTarget::Number, "minmax-number-invalid.html", "min(1 / )"},
	{CalculationCorpusTarget::Number, "minmax-number-invalid.html", "min(1 2)"},
	{CalculationCorpusTarget::Number, "minmax-number-invalid.html", "min(1, , 2)"},
	{CalculationCorpusTarget::Number, "minmax-number-invalid.html", "max()"},
	{CalculationCorpusTarget::Number, "minmax-number-invalid.html", "max( )"},
	{CalculationCorpusTarget::Number, "minmax-number-invalid.html", "max(,)"},
	{CalculationCorpusTarget::Number, "minmax-number-invalid.html", "max(1, )"},
	{CalculationCorpusTarget::Number, "minmax-number-invalid.html", "max(, 1)"},
	{CalculationCorpusTarget::Number, "minmax-number-invalid.html", "max(1 + )"},
	{CalculationCorpusTarget::Number, "minmax-number-invalid.html", "max(1 - )"},
	{CalculationCorpusTarget::Number, "minmax-number-invalid.html", "max(1 * )"},
	{CalculationCorpusTarget::Number, "minmax-number-invalid.html", "max(1 / )"},
	{CalculationCorpusTarget::Number, "minmax-number-invalid.html", "max(1 2)"},
	{CalculationCorpusTarget::Number, "minmax-number-invalid.html", "max(1, , 2)"},
	{CalculationCorpusTarget::Number, "minmax-number-invalid.html", "min(0px)"},
	{CalculationCorpusTarget::Number, "minmax-number-invalid.html", "min(0s)"},
	{CalculationCorpusTarget::Number, "minmax-number-invalid.html", "min(0deg)"},
	{CalculationCorpusTarget::Number, "minmax-number-invalid.html", "min(1, 1%)"},
	{CalculationCorpusTarget::Number, "minmax-number-invalid.html", "min(1, 0px)"},
	{CalculationCorpusTarget::Number, "minmax-number-invalid.html", "min(1, 0s)"},
	{CalculationCorpusTarget::Number, "minmax-number-invalid.html", "min(1, 0deg)"},
	{CalculationCorpusTarget::Number, "minmax-number-invalid.html", "max(0px)"},
	{CalculationCorpusTarget::Number, "minmax-number-invalid.html", "max(0s)"},
	{CalculationCorpusTarget::Number, "minmax-number-invalid.html", "max(0deg)"},
	{CalculationCorpusTarget::Number, "minmax-number-invalid.html", "max(1, 1%)"},
	{CalculationCorpusTarget::Number, "minmax-number-invalid.html", "max(1, 0px)"},
	{CalculationCorpusTarget::Number, "minmax-number-invalid.html", "max(1, 0s)"},
	{CalculationCorpusTarget::Number, "minmax-number-invalid.html", "max(1, 0deg)"},

	// css/css-values/minmax-percentage-invalid.html, excluding unsupported Hz/dpi/fr rows.
	{CalculationCorpusTarget::Percentage, "minmax-percentage-invalid.html", "min()"},
	{CalculationCorpusTarget::Percentage, "minmax-percentage-invalid.html", "min( )"},
	{CalculationCorpusTarget::Percentage, "minmax-percentage-invalid.html", "min(,)"},
	{CalculationCorpusTarget::Percentage, "minmax-percentage-invalid.html", "min(1#)"},
	{CalculationCorpusTarget::Percentage, "minmax-percentage-invalid.html", "min(%1)"},
	{CalculationCorpusTarget::Percentage, "minmax-percentage-invalid.html", "min(1%, )"},
	{CalculationCorpusTarget::Percentage, "minmax-percentage-invalid.html", "min(, 1%)"},
	{CalculationCorpusTarget::Percentage, "minmax-percentage-invalid.html", "min(1% + )"},
	{CalculationCorpusTarget::Percentage, "minmax-percentage-invalid.html", "min(1% - )"},
	{CalculationCorpusTarget::Percentage, "minmax-percentage-invalid.html", "min(1% * )"},
	{CalculationCorpusTarget::Percentage, "minmax-percentage-invalid.html", "min(1% / )"},
	{CalculationCorpusTarget::Percentage, "minmax-percentage-invalid.html", "min(1% 2%)"},
	{CalculationCorpusTarget::Percentage, "minmax-percentage-invalid.html", "min(1%, , 2%)"},
	{CalculationCorpusTarget::Percentage, "minmax-percentage-invalid.html", "max()"},
	{CalculationCorpusTarget::Percentage, "minmax-percentage-invalid.html", "max( )"},
	{CalculationCorpusTarget::Percentage, "minmax-percentage-invalid.html", "max(,)"},
	{CalculationCorpusTarget::Percentage, "minmax-percentage-invalid.html", "max(1#)"},
	{CalculationCorpusTarget::Percentage, "minmax-percentage-invalid.html", "max(%1)"},
	{CalculationCorpusTarget::Percentage, "minmax-percentage-invalid.html", "max(1%, )"},
	{CalculationCorpusTarget::Percentage, "minmax-percentage-invalid.html", "max(, 1%)"},
	{CalculationCorpusTarget::Percentage, "minmax-percentage-invalid.html", "max(1% + )"},
	{CalculationCorpusTarget::Percentage, "minmax-percentage-invalid.html", "max(1% - )"},
	{CalculationCorpusTarget::Percentage, "minmax-percentage-invalid.html", "max(1% * )"},
	{CalculationCorpusTarget::Percentage, "minmax-percentage-invalid.html", "max(1% / )"},
	{CalculationCorpusTarget::Percentage, "minmax-percentage-invalid.html", "max(1% 2%)"},
	{CalculationCorpusTarget::Percentage, "minmax-percentage-invalid.html", "max(1%, , 2%)"},
	{CalculationCorpusTarget::Percentage, "minmax-percentage-invalid.html", "min(0s)"},
	{CalculationCorpusTarget::Percentage, "minmax-percentage-invalid.html", "min(0deg)"},
	{CalculationCorpusTarget::Percentage, "minmax-percentage-invalid.html", "min(1%, 0)"},
	{CalculationCorpusTarget::Percentage, "minmax-percentage-invalid.html", "min(1%, 0s)"},
	{CalculationCorpusTarget::Percentage, "minmax-percentage-invalid.html", "min(1%, 0deg)"},
	{CalculationCorpusTarget::Percentage, "minmax-percentage-invalid.html", "max(0s)"},
	{CalculationCorpusTarget::Percentage, "minmax-percentage-invalid.html", "max(0deg)"},
	{CalculationCorpusTarget::Percentage, "minmax-percentage-invalid.html", "max(1%, 0)"},
	{CalculationCorpusTarget::Percentage, "minmax-percentage-invalid.html", "max(1%, 0s)"},
	{CalculationCorpusTarget::Percentage, "minmax-percentage-invalid.html", "max(1%, 0deg)"},

	// css/css-values/minmax-length-invalid.html, excluding unsupported Hz/dpi/fr rows.
	{CalculationCorpusTarget::Length, "minmax-length-invalid.html", "min()"},
	{CalculationCorpusTarget::Length, "minmax-length-invalid.html", "min( )"},
	{CalculationCorpusTarget::Length, "minmax-length-invalid.html", "min(,)"},
	{CalculationCorpusTarget::Length, "minmax-length-invalid.html", "min(1py)"},
	{CalculationCorpusTarget::Length, "minmax-length-invalid.html", "min(1px, )"},
	{CalculationCorpusTarget::Length, "minmax-length-invalid.html", "min(, 1px)"},
	{CalculationCorpusTarget::Length, "minmax-length-invalid.html", "min(1px + )"},
	{CalculationCorpusTarget::Length, "minmax-length-invalid.html", "min(1px - )"},
	{CalculationCorpusTarget::Length, "minmax-length-invalid.html", "min(1px * )"},
	{CalculationCorpusTarget::Length, "minmax-length-invalid.html", "min(1px / )"},
	{CalculationCorpusTarget::Length, "minmax-length-invalid.html", "min(1px 2px)"},
	{CalculationCorpusTarget::Length, "minmax-length-invalid.html", "min(1px, , 2px)"},
	{CalculationCorpusTarget::Length, "minmax-length-invalid.html", "max()"},
	{CalculationCorpusTarget::Length, "minmax-length-invalid.html", "max( )"},
	{CalculationCorpusTarget::Length, "minmax-length-invalid.html", "max(,)"},
	{CalculationCorpusTarget::Length, "minmax-length-invalid.html", "max(1py)"},
	{CalculationCorpusTarget::Length, "minmax-length-invalid.html", "max(1px, )"},
	{CalculationCorpusTarget::Length, "minmax-length-invalid.html", "max(, 1px)"},
	{CalculationCorpusTarget::Length, "minmax-length-invalid.html", "max(1px + )"},
	{CalculationCorpusTarget::Length, "minmax-length-invalid.html", "max(1px - )"},
	{CalculationCorpusTarget::Length, "minmax-length-invalid.html", "max(1px * )"},
	{CalculationCorpusTarget::Length, "minmax-length-invalid.html", "max(1px / )"},
	{CalculationCorpusTarget::Length, "minmax-length-invalid.html", "max(1px 2px)"},
	{CalculationCorpusTarget::Length, "minmax-length-invalid.html", "max(1px, , 2px)"},
	{CalculationCorpusTarget::Length, "minmax-length-invalid.html", "min(0)"},
	{CalculationCorpusTarget::Length, "minmax-length-invalid.html", "min(0%)"},
	{CalculationCorpusTarget::Length, "minmax-length-invalid.html", "min(0s)"},
	{CalculationCorpusTarget::Length, "minmax-length-invalid.html", "min(1px, 0)"},
	{CalculationCorpusTarget::Length, "minmax-length-invalid.html", "min(1px, 0%)"},
	{CalculationCorpusTarget::Length, "minmax-length-invalid.html", "min(1px, 0s)"},
	{CalculationCorpusTarget::Length, "minmax-length-invalid.html", "max(0)"},
	{CalculationCorpusTarget::Length, "minmax-length-invalid.html", "max(0%)"},
	{CalculationCorpusTarget::Length, "minmax-length-invalid.html", "max(0s)"},
	{CalculationCorpusTarget::Length, "minmax-length-invalid.html", "max(1px, 0)"},
	{CalculationCorpusTarget::Length, "minmax-length-invalid.html", "max(1px, 0%)"},
	{CalculationCorpusTarget::Length, "minmax-length-invalid.html", "max(1px, 0s)"},

	// css/css-values/minmax-length-percent-invalid.html, excluding unsupported Hz/dpi/fr rows.
	{CalculationCorpusTarget::LengthPercentage, "minmax-length-percent-invalid.html", "min(1px, 0)"},
	{CalculationCorpusTarget::LengthPercentage, "minmax-length-percent-invalid.html", "min(1px, 0s)"},
	{CalculationCorpusTarget::LengthPercentage, "minmax-length-percent-invalid.html", "min(1%, 0)"},
	{CalculationCorpusTarget::LengthPercentage, "minmax-length-percent-invalid.html", "min(1%, 0s)"},
	{CalculationCorpusTarget::LengthPercentage, "minmax-length-percent-invalid.html", "max(1px, 0)"},
	{CalculationCorpusTarget::LengthPercentage, "minmax-length-percent-invalid.html", "max(1px, 0s)"},
	{CalculationCorpusTarget::LengthPercentage, "minmax-length-percent-invalid.html", "max(1%, 0)"},
	{CalculationCorpusTarget::LengthPercentage, "minmax-length-percent-invalid.html", "max(1%, 0s)"},

	// css/css-values/minmax-angle-invalid.html, excluding unsupported Hz/dpi/fr rows.
	{CalculationCorpusTarget::Angle, "minmax-angle-invalid.html", "min()"},
	{CalculationCorpusTarget::Angle, "minmax-angle-invalid.html", "min( )"},
	{CalculationCorpusTarget::Angle, "minmax-angle-invalid.html", "min(,)"},
	{CalculationCorpusTarget::Angle, "minmax-angle-invalid.html", "min(1dag)"},
	{CalculationCorpusTarget::Angle, "minmax-angle-invalid.html", "min(1deg, )"},
	{CalculationCorpusTarget::Angle, "minmax-angle-invalid.html", "min(, 1deg)"},
	{CalculationCorpusTarget::Angle, "minmax-angle-invalid.html", "min(1deg + )"},
	{CalculationCorpusTarget::Angle, "minmax-angle-invalid.html", "min(1deg - )"},
	{CalculationCorpusTarget::Angle, "minmax-angle-invalid.html", "min(1deg * )"},
	{CalculationCorpusTarget::Angle, "minmax-angle-invalid.html", "min(1deg / )"},
	{CalculationCorpusTarget::Angle, "minmax-angle-invalid.html", "min(1deg 2deg)"},
	{CalculationCorpusTarget::Angle, "minmax-angle-invalid.html", "min(1deg, , 2deg)"},
	{CalculationCorpusTarget::Angle, "minmax-angle-invalid.html", "max()"},
	{CalculationCorpusTarget::Angle, "minmax-angle-invalid.html", "max( )"},
	{CalculationCorpusTarget::Angle, "minmax-angle-invalid.html", "max(,)"},
	{CalculationCorpusTarget::Angle, "minmax-angle-invalid.html", "max(1dag)"},
	{CalculationCorpusTarget::Angle, "minmax-angle-invalid.html", "max(1deg, )"},
	{CalculationCorpusTarget::Angle, "minmax-angle-invalid.html", "max(, 1deg)"},
	{CalculationCorpusTarget::Angle, "minmax-angle-invalid.html", "max(1deg + )"},
	{CalculationCorpusTarget::Angle, "minmax-angle-invalid.html", "max(1deg - )"},
	{CalculationCorpusTarget::Angle, "minmax-angle-invalid.html", "max(1deg * )"},
	{CalculationCorpusTarget::Angle, "minmax-angle-invalid.html", "max(1deg / )"},
	{CalculationCorpusTarget::Angle, "minmax-angle-invalid.html", "max(1deg 2deg)"},
	{CalculationCorpusTarget::Angle, "minmax-angle-invalid.html", "max(1deg, , 2deg)"},
	{CalculationCorpusTarget::Angle, "minmax-angle-invalid.html", "min(0)"},
	{CalculationCorpusTarget::Angle, "minmax-angle-invalid.html", "min(0%)"},
	{CalculationCorpusTarget::Angle, "minmax-angle-invalid.html", "min(0px)"},
	{CalculationCorpusTarget::Angle, "minmax-angle-invalid.html", "min(0s)"},
	{CalculationCorpusTarget::Angle, "minmax-angle-invalid.html", "min(1deg, 0)"},
	{CalculationCorpusTarget::Angle, "minmax-angle-invalid.html", "min(1deg, 0%)"},
	{CalculationCorpusTarget::Angle, "minmax-angle-invalid.html", "min(1deg, 0px)"},
	{CalculationCorpusTarget::Angle, "minmax-angle-invalid.html", "min(1deg, 0s)"},
	{CalculationCorpusTarget::Angle, "minmax-angle-invalid.html", "max(0)"},
	{CalculationCorpusTarget::Angle, "minmax-angle-invalid.html", "max(0%)"},
	{CalculationCorpusTarget::Angle, "minmax-angle-invalid.html", "max(0px)"},
	{CalculationCorpusTarget::Angle, "minmax-angle-invalid.html", "max(0s)"},
	{CalculationCorpusTarget::Angle, "minmax-angle-invalid.html", "max(1deg, 0)"},
	{CalculationCorpusTarget::Angle, "minmax-angle-invalid.html", "max(1deg, 0%)"},
	{CalculationCorpusTarget::Angle, "minmax-angle-invalid.html", "max(1deg, 0px)"},
	{CalculationCorpusTarget::Angle, "minmax-angle-invalid.html", "max(1deg, 0s)"},

	// css/css-values/clamp-length-invalid.html, excluding the abs() rows owned by O_ADVANCED.
	{CalculationCorpusTarget::Length, "clamp-length-invalid.html", "clamp()"},
	{CalculationCorpusTarget::Length, "clamp-length-invalid.html", "clamp( )"},
	{CalculationCorpusTarget::Length, "clamp-length-invalid.html", "clamp(,)"},
	{CalculationCorpusTarget::Length, "clamp-length-invalid.html", "clamp(1px, )"},
	{CalculationCorpusTarget::Length, "clamp-length-invalid.html", "clamp(, 1px)"},
	{CalculationCorpusTarget::Length, "clamp-length-invalid.html", "clamp(1px, 1px)"},
	{CalculationCorpusTarget::Length, "clamp-length-invalid.html", "clamp(1px, , 1px)"},
	{CalculationCorpusTarget::Length, "clamp-length-invalid.html", "clamp(, 1px, 1px)"},
	{CalculationCorpusTarget::Length, "clamp-length-invalid.html", "clamp(1px, 1px, )"},
	{CalculationCorpusTarget::Length, "clamp-length-invalid.html", "clamp(1px, 1px, 1px, )"},
	{CalculationCorpusTarget::Length, "clamp-length-invalid.html", "clamp(1px 1px 1px)"},
	{CalculationCorpusTarget::Length, "clamp-length-invalid.html", "clamp(0, 10rem, 100%)"},
	{CalculationCorpusTarget::Length, "clamp-length-invalid.html", "clamp(none, none, none)"},
	{CalculationCorpusTarget::Length, "clamp-length-invalid.html", "clamp(none, none + none, none)"},
	{CalculationCorpusTarget::Length, "clamp-length-invalid.html", "clamp(none, none + 1, none)"},
	{CalculationCorpusTarget::Length, "clamp-length-invalid.html", "clamp(-none, 1px, 1px)"},
	{CalculationCorpusTarget::Length, "clamp-length-invalid.html", "clamp(10px, none * none, 20px)"},
	{CalculationCorpusTarget::Length, "clamp-length-invalid.html", "clamp(10px, 2px * none, 20px)"},
	{CalculationCorpusTarget::Length, "clamp-length-invalid.html", "clamp(10px, 2px + none, 20px)"},
	{CalculationCorpusTarget::Length, "clamp-length-invalid.html", "clamp(10px + none, 10px / 2px + none, 20px / none)"},
	{CalculationCorpusTarget::Length, "clamp-length-invalid.html", "clamp(10px, -none, 20px)"},
};

template <typename T, size_t N>
constexpr size_t Count(const T (&)[N])
{
	return N;
}

#endif

} // namespace

#ifndef RMLUI_MATH_EXPRESSIONS

TEST_CASE("Calculation.feature_off.rejects_math_functions")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);
	ElementDocument* document = context->LoadDocumentFromMemory("<rml><head><style>body { display: block; }</style></head><body/></rml>");
	REQUIRE(document);

	CHECK(document->SetProperty("width", "10px"));
	CHECK(document->SetProperty("opacity", "0.5"));

	TestsShell::SetNumExpectedWarnings(5);
	CHECK_FALSE(document->SetProperty("width", "calc(10px + 5px)"));
	CHECK_FALSE(document->SetProperty("width", "min(10px, 20px)"));
	CHECK_FALSE(document->SetProperty("width", "max(10px, 20px)"));
	CHECK_FALSE(document->SetProperty("width", "clamp(10px, 15px, 20px)"));
	CHECK_FALSE(document->SetProperty("opacity", "calc(1 / 2)"));

	document->Close();
	TestsShell::ShutdownShell();
}

#else

namespace {

CalculationParseTarget Target(CalculationCorpusTarget target)
{
	switch (target)
	{
	case CalculationCorpusTarget::Number: return MakeCalculationParseTarget(CalculationFinalType::Number);
	case CalculationCorpusTarget::Percentage: return MakeCalculationParseTarget(CalculationFinalType::Percent);
	case CalculationCorpusTarget::Length: return MakeCalculationParseTarget(CalculationFinalType::Length);
	case CalculationCorpusTarget::LengthPercentage:
		return MakeCalculationParseTarget(CalculationFinalType::Length, CalculationPercentageHint::Length);
	case CalculationCorpusTarget::Angle: return MakeCalculationParseTarget(CalculationFinalType::Angle);
	case CalculationCorpusTarget::Time: return MakeCalculationParseTarget(CalculationFinalType::Time);
	case CalculationCorpusTarget::LineHeight: break;
	}
	return {};
}

bool Parses(CalculationCorpusTarget target, const String& expression, CalculationPtr* result_out = nullptr)
{
	CalculationPtr result;
	const bool parsed = ParseCalculation(expression, Target(target), result);
	if (result_out)
		*result_out = std::move(result);
	return parsed;
}

void CheckEvaluates(CalculationCorpusTarget target, const char* expression, float expected, Unit unit)
{
	CAPTURE(String(expression));
	CalculationPtr calculation;
	if (!Parses(target, expression, &calculation))
	{
		FAIL_CHECK("Expression failed to parse before evaluation");
		return;
	}
	CalculationConstantValue value;
	if (!EvaluateCalculation(*calculation, value))
	{
		FAIL_CHECK("Expression did not reduce to a pure constant");
		return;
	}
	CHECK(value.value == doctest::Approx(expected));
	CHECK(value.unit == unit);
}

void CheckEvaluates(CalculationCorpusTarget target, const CalculationCorpusCase& test)
{
	char* suffix_begin = nullptr;
	float expected = strtof(test.expected, &suffix_begin);
	if (suffix_begin == test.expected)
	{
		FAIL_CHECK("WPT expected value is not a pure numeric constant");
		return;
	}

	const String suffix(suffix_begin);
	Unit unit = Unit::UNKNOWN;
	if (suffix.empty())
		unit = Unit::NUMBER;
	else if (suffix == "%")
		unit = Unit::PERCENT;
	else if (suffix == "px")
		unit = Unit::PX;
	else if (suffix == "in")
	{
		expected *= 96.f;
		unit = Unit::PX;
	}
	else if (suffix == "cm")
	{
		expected *= 96.f / 2.54f;
		unit = Unit::PX;
	}
	else if (suffix == "mm")
	{
		expected *= 96.f / 25.4f;
		unit = Unit::PX;
	}
	else if (suffix == "pt")
	{
		expected *= 96.f / 72.f;
		unit = Unit::PX;
	}
	else if (suffix == "pc")
	{
		expected *= 16.f;
		unit = Unit::PX;
	}
	else if (suffix == "deg")
		unit = Unit::DEG;
	else if (suffix == "rad")
	{
		expected *= 180.f / float(3.14159265358979323846);
		unit = Unit::DEG;
	}
	else
	{
		FAIL_CHECK("WPT expected value requires later contextual resolution or an unsupported unit");
		return;
	}

	CAPTURE(String(test.source));
	CheckEvaluates(target, test.expression, expected, unit);
}

void CheckEvaluatesTime(const CalculationCorpusCase& test)
{
	CAPTURE(String(test.source));
	CAPTURE(String(test.expression));
	CalculationPtr calculation;
	REQUIRE(ParseCalculation(test.expression, MakeCalculationParseTarget(CalculationFinalType::Time), calculation));
	float seconds = 0.f;
	REQUIRE(EvaluateCalculationTime(*calculation, seconds));
	char* suffix_begin = nullptr;
	float expected = strtof(test.expected, &suffix_begin);
	REQUIRE(suffix_begin != test.expected);
	const String suffix(suffix_begin);
	if (suffix == "ms")
		expected *= 0.001f;
	else
		REQUIRE(suffix == "s");
	CHECK(seconds == doctest::Approx(expected));
}

String MakeSum(size_t terms)
{
	String result = "calc(";
	for (size_t i = 0; i < terms; ++i)
	{
		if (i)
			result += " + ";
		result += "1";
	}
	return result + ')';
}

String MakeNested(size_t depth)
{
	String result = "calc(";
	result.append(depth, '(');
	result += "1px";
	result.append(depth, ')');
	return result + ')';
}

String MakeNestedPercent(size_t depth)
{
	String result = "calc(";
	result.append(depth, '(');
	result += "100%";
	result.append(depth, ')');
	return result + ')';
}

String MakeArguments(size_t arguments)
{
	String result = "max(";
	for (size_t i = 0; i < arguments; ++i)
	{
		if (i)
			result += ", ";
		result += CreateString("%zu", i + 1);
	}
	return result + ')';
}

} // namespace

TEST_CASE("Calculation.parser.wpt_number_and_angle")
{
	// Sources: calc-numbers.html, calc-nesting.html, minmax-number-computed.html.
	for (const auto& test : pure_number_cases)
	{
		CAPTURE(String(test.source));
		CAPTURE(String(test.expression));
		CheckEvaluates(CalculationCorpusTarget::Number, test);
	}

	// Source: minmax-percentage-computed.html. Raw percentages have no external basis dependency.
	for (const auto& test : pure_percentage_cases)
		CheckEvaluates(CalculationCorpusTarget::Percentage, test);

	// Source: calc-angle-values.html. Only the deg/rad rows are in scope.
	for (const auto& test : angle_cases)
	{
		CAPTURE(String(test.source));
		CAPTURE(String(test.expression));
		CheckEvaluates(CalculationCorpusTarget::Angle, test);
	}
	for (const auto& test : pure_minmax_angle_cases)
		CheckEvaluates(CalculationCorpusTarget::Angle, test);
}

TEST_CASE("Calculation.parser.wpt_exhaustive_parse_rows")
{
	for (const auto& test : wpt_valid_parse_cases)
	{
		CAPTURE(String(test.source));
		CAPTURE(String(test.expression));
		CHECK(Parses(test.target, test.expression));
	}
	for (const auto& test : wpt_invalid_parse_cases)
	{
		CAPTURE(String(test.source));
		CAPTURE(String(test.expression));
		CHECK_FALSE(Parses(test.target, test.expression));
	}
}

TEST_CASE("Calculation.parser.wpt_core_remaining_rows")
{
	// calc-in-calc.html, calc-in-max.html, calc-nesting.html.
	const char* nested_values[] = {
		"calc(calc(100%))",
		"max(calc(100%))",
		"calc(20px + calc(80px))",
		"calc(calc(100px))",
		"calc(calc(2) * calc(50px))",
		"calc(calc(150px*2/3))",
		"calc(calc(2 * calc(calc(3)) + 4) * 10px)",
		"calc(50px + calc(40%))",
	};
	for (const char* expression : nested_values)
	{
		CAPTURE(String(expression));
		CHECK(Parses(CalculationCorpusTarget::LengthPercentage, expression));
	}
	CheckEvaluates(CalculationCorpusTarget::LengthPercentage, "calc(calc(100%))", 100.f, Unit::PERCENT);
	CheckEvaluates(CalculationCorpusTarget::LengthPercentage, "max(calc(100%))", 100.f, Unit::PERCENT);
	CheckEvaluates(CalculationCorpusTarget::LengthPercentage, "calc(20px + calc(80px))", 100.f, Unit::PX);
	CheckEvaluates(CalculationCorpusTarget::LengthPercentage, "calc(calc(100px))", 100.f, Unit::PX);
	CheckEvaluates(CalculationCorpusTarget::LengthPercentage, "calc(calc(2) * calc(50px))", 100.f, Unit::PX);
	CheckEvaluates(CalculationCorpusTarget::LengthPercentage, "calc(calc(150px*2/3))", 100.f, Unit::PX);
	CheckEvaluates(CalculationCorpusTarget::LengthPercentage, "calc(calc(2 * calc(calc(3)) + 4) * 10px)", 100.f, Unit::PX);

	// calc-parenthesis-stack.html requires exactly 32 nested pairs inside calc().
	CHECK(Parses(CalculationCorpusTarget::LengthPercentage, MakeNestedPercent(32)));
	CheckEvaluates(CalculationCorpusTarget::LengthPercentage, MakeNestedPercent(32).c_str(), 100.f, Unit::PERCENT);

	// calc-rounding-001.html has a pure parser/type row. The var()-dependent rows in
	// calc-rounding-002/003 require variable substitution and are covered by the integration tests.
	CHECK(Parses(CalculationCorpusTarget::LengthPercentage, "calc((100% - 4.5em) / 4)"));

	// max-20-arguments.html, min-length-percent-001.html, max-length-percent-001.html.
	CHECK(Parses(CalculationCorpusTarget::LengthPercentage,
		"max(5%, 10%, 15%, 20%, 25%, 30%, 35%, 40%, 45%, 50%, 55%, 60%, 65%, 70%, 75%, 80%, 85%, 90%, 95%, 100%)"));
	CHECK(Parses(CalculationCorpusTarget::LengthPercentage, "min(300px, 25% + 100px, 50px + 50%)"));
	CHECK(Parses(CalculationCorpusTarget::LengthPercentage, "max(100px, 25% + 100px, 150px + 10%)"));
	CHECK_FALSE(Parses(CalculationCorpusTarget::LengthPercentage, "min(0, 100%)"));

	// calc-unit-analysis.html.
	CHECK(Parses(CalculationCorpusTarget::Length, "calc(0px)"));
	CHECK(Parses(CalculationCorpusTarget::Length, "calc(2px * 2)"));
	CHECK(Parses(CalculationCorpusTarget::Length, "calc(2 * 2px)"));
	CheckEvaluates(CalculationCorpusTarget::Length, "calc(0px)", 0.f, Unit::PX);
	CheckEvaluates(CalculationCorpusTarget::Length, "calc(2px * 2)", 4.f, Unit::PX);
	CheckEvaluates(CalculationCorpusTarget::Length, "calc(2 * 2px)", 4.f, Unit::PX);
	CHECK_FALSE(Parses(CalculationCorpusTarget::Length, "calc(0)"));
	CHECK_FALSE(Parses(CalculationCorpusTarget::Length, "calc(1px + 2)"));
	CHECK_FALSE(Parses(CalculationCorpusTarget::Length, "calc(2 + 1px)"));
	CHECK_FALSE(Parses(CalculationCorpusTarget::Length, "calc(1px - 2)"));
	CHECK_FALSE(Parses(CalculationCorpusTarget::Length, "calc(2 - 1px)"));
	CHECK_FALSE(Parses(CalculationCorpusTarget::Length, "calc(2px * 1px)"));
	CHECK_FALSE(Parses(CalculationCorpusTarget::Length, "calc(20 / 0.75rem)"));

	// calc-numbers.html's pure numeric rows and applicable trailing-token rejection.
	CheckEvaluates(CalculationCorpusTarget::Number, "calc(2 * 3)", 6.f, Unit::NUMBER);
	CheckEvaluates(CalculationCorpusTarget::Number, "calc(2 * -4)", -8.f, Unit::NUMBER);
	CheckEvaluates(CalculationCorpusTarget::Number, "calc(2 / 4)", 0.5f, Unit::NUMBER);
	CheckEvaluates(CalculationCorpusTarget::LengthPercentage,
		"max(5%, 10%, 15%, 20%, 25%, 30%, 35%, 40%, 45%, 50%, 55%, 60%, 65%, 70%, 75%, 80%, 85%, 90%, 95%, 100%)", 100.f, Unit::PERCENT);
	CHECK_FALSE(Parses(CalculationCorpusTarget::Number, "calc(2 / 4) * 1px"));
	CHECK_FALSE(Parses(CalculationCorpusTarget::Number, "calc(1 + 1px)"));
	CHECK_FALSE(Parses(CalculationCorpusTarget::Number, "calc(1 + 100%)"));
	CHECK_FALSE(Parses(CalculationCorpusTarget::Number, "calc(10px) bla"));
	CHECK_FALSE(Parses(CalculationCorpusTarget::Number, "calc(bla) 10px"));
}

TEST_CASE("Calculation.parser.wpt_typed_arithmetic_remaining_rows")
{
	const CalculationParseTarget length_percentage = MakeCalculationParseTarget(CalculationFinalType::Length, CalculationPercentageHint::Length);
	const CalculationParseTarget line_height = {
		CalculationFinalType::Number | CalculationFinalType::Length,
		CalculationPercentageHint::Length,
	};
	CalculationPtr calculation;

	const char* length_rows[] = {
		"min(1em, 110px / 10px * 1px)",
		"max(10px, 110px / 10px * 1px)",
		"max(1em + 2px, 110px / 10px * 1px)",
	};
	for (const char* expression : length_rows)
	{
		CAPTURE(String(expression));
		CHECK(ParseCalculation(expression, MakeCalculationParseTarget(CalculationFinalType::Length), calculation));
	}

	const char* length_percentage_rows[] = {
		"max(1em + 2%, 110px / 10px * 1px)",
		"clamp(110px / 10px * 1px, 1em + 200%, 200% * 1% / 1em)",
		"calc((10% * 1%) / 1px)",
	};
	for (const char* expression : length_percentage_rows)
	{
		CAPTURE(String(expression));
		CHECK(ParseCalculation(expression, length_percentage, calculation));
	}

	const char* number_rows[] = {
		"calc(10em / 1em)",
		"calc(10em / 1rem)",
		"calc(10em / 1px)",
	};
	for (const char* expression : number_rows)
	{
		CAPTURE(String(expression));
		CHECK(ParseCalculation(expression, MakeCalculationParseTarget(CalculationFinalType::Number), calculation));
	}

	const char* line_height_rows[] = {
		"calc(10% / 1px)",
		"calc(1% * 100% / 10%)",
		"calc(10% / 10%)",
		"calc(10% * 10% / 1px * 10deg / 1deg / 10px)",
		"calc(10% * 10% / 1px * 1deg / 1deg)",
		"calc(1px * 2deg / 1deg)",
		"calc(1px * 3deg / 1deg / 1px)",
	};
	for (const char* expression : line_height_rows)
	{
		CAPTURE(String(expression));
		CHECK(ParseCalculation(expression, line_height, calculation));
	}

	CHECK_FALSE(ParseCalculation("calc((1% * 1deg) / 1px)", length_percentage, calculation));
	CHECK_FALSE(ParseCalculation("calc((1% * 1% * 1%) / 1px)", length_percentage, calculation));

	String excessive_product = "calc(1px * 1px";
	String excessive_sum = "calc(1px + 1em";
	for (size_t i = 0; i < 200; ++i)
	{
		excessive_product += " * 2";
		excessive_sum += " + 1em";
	}
	excessive_product += ')';
	excessive_sum += ')';
	CHECK_FALSE(ParseCalculation(excessive_product, length_percentage, calculation));
	CHECK_FALSE(ParseCalculation(excessive_sum, length_percentage, calculation));
}

TEST_CASE("Calculation.parser.wpt_length_percentage_and_comparisons")
{
	for (const auto& test : pure_absolute_length_cases)
		CheckEvaluates(CalculationCorpusTarget::Length, test);
	for (const auto& test : pure_clamp_length_cases)
		CheckEvaluates(CalculationCorpusTarget::Length, test);

	// Sources: minmax-length-computed.html, clamp-length-computed.html.
	for (const auto& test : definite_length_cases)
	{
		CAPTURE(String(test.source));
		CAPTURE(String(test.expression));
		CHECK(Parses(CalculationCorpusTarget::Length, test.expression));
	}
	CheckEvaluates(CalculationCorpusTarget::Length, "min(95px, 1in)", 95.f, Unit::PX);
	CheckEvaluates(CalculationCorpusTarget::Length, "max(95px, 1in)", 96.f, Unit::PX);
	CheckEvaluates(CalculationCorpusTarget::Length, "clamp(30px, 100px, 20px)", 30.f, Unit::PX);
	CheckEvaluates(CalculationCorpusTarget::Length, "clamp(none, 33px, 30px)", 30.f, Unit::PX);
	CheckEvaluates(CalculationCorpusTarget::Length, "clamp(33px, 30px, none)", 33.f, Unit::PX);
	CheckEvaluates(CalculationCorpusTarget::Length, "clamp(none, 30px, none)", 30.f, Unit::PX);

	// Sources: minmax-length-percent-computed.html, max-20-arguments.html, calc-nesting.html.
	for (const auto& test : used_length_percentage_cases)
	{
		CAPTURE(String(test.source));
		CAPTURE(String(test.expression));
		CHECK(Parses(CalculationCorpusTarget::LengthPercentage, test.expression));
	}
	for (const char* expression : clamp_none_whitespace_cases)
	{
		CAPTURE(expression);
		CHECK(Parses(CalculationCorpusTarget::LengthPercentage, expression));
	}
	CheckEvaluates(CalculationCorpusTarget::LengthPercentage, "max(5%, 10%, 15%, 20%)", 20.f, Unit::PERCENT);
}

TEST_CASE("Calculation.parser.wpt_invalid_and_type_algebra")
{
	// Sources: calc-unit-analysis.html, calc-invalid-parsing.html and the min/max/clamp invalid suites.
	for (const auto& test : pure_invalid_cases)
	{
		CAPTURE(String(test.source));
		CAPTURE(String(test.expression));
		CHECK_FALSE(Parses(test.target, test.expression));
	}
	CHECK_FALSE(Parses(CalculationCorpusTarget::Length, "calc(2px * 1px)"));
	CHECK_FALSE(Parses(CalculationCorpusTarget::Length, "calc(20 / 0.75rem)"));
	CHECK_FALSE(Parses(CalculationCorpusTarget::Length, "min(0, 100%)"));

	// Source: typed_arithmetic.html. Finite dimensional products may carry powers which later cancel.
	for (const auto& test : typed_arithmetic_cases)
	{
		CAPTURE(String(test.source));
		CAPTURE(String(test.expression));
		const CalculationCorpusTarget target =
			String(test.expected).find("px") != String::npos ? CalculationCorpusTarget::Length : CalculationCorpusTarget::Number;
		CHECK(Parses(target, test.expression));
	}
	CheckEvaluates(CalculationCorpusTarget::Length, "calc(1px * 2deg / 1deg)", 2.f, Unit::PX);
	CheckEvaluates(CalculationCorpusTarget::Number, "calc(1px * 3deg / 1deg / 1px)", 3.f, Unit::NUMBER);
	CHECK_FALSE(Parses(CalculationCorpusTarget::Length, "calc((1% * 1deg) / 1px)"));
}

TEST_CASE("Calculation.parser.targets_whitespace_and_limits")
{
	// Supplemental parser boundary cases not represented exactly in the adapted WPT corpus.
	CHECK(Parses(CalculationCorpusTarget::Number, "calc(1 + 2)"));
	CHECK(Parses(CalculationCorpusTarget::Number, "calc(1 - -2)"));
	CHECK(Parses(CalculationCorpusTarget::Number, "calc(2*-4)"));
	CHECK(Parses(CalculationCorpusTarget::Number, "calc(.5 + 1e2)"));
	CHECK_FALSE(Parses(CalculationCorpusTarget::Number, "calc(1+ 2)"));
	CHECK_FALSE(Parses(CalculationCorpusTarget::Number, "calc(1 +2)"));
	CHECK_FALSE(Parses(CalculationCorpusTarget::Number, "calc(1+2)"));
	CHECK_FALSE(Parses(CalculationCorpusTarget::Number, "calc(0x1)"));
	CHECK_FALSE(Parses(CalculationCorpusTarget::Number, "calc(0x1p2)"));
	CHECK_FALSE(Parses(CalculationCorpusTarget::Number, "calc(1.)"));
	CHECK_FALSE(Parses(CalculationCorpusTarget::Number, "calc (1 + 2)"));
	CHECK_FALSE(Parses(CalculationCorpusTarget::Number, "min (1, 2)"));
	CHECK_FALSE(Parses(CalculationCorpusTarget::Number, "calc(1/*comment*/+/*comment*/2)"));
	CHECK(Parses(CalculationCorpusTarget::Number, "calc(1 /*comment*/ + /*comment*/ 2)"));
	CHECK_FALSE(Parses(CalculationCorpusTarget::Number, "calc/**/(1 + 2)"));
	CHECK_FALSE(Parses(CalculationCorpusTarget::Length, "calc(1/**/px)"));
	CHECK(Parses(CalculationCorpusTarget::Number, "calc(1\f+\f2)"));

	CHECK(Parses(CalculationCorpusTarget::Number, MakeSum(31)));
	CHECK(Parses(CalculationCorpusTarget::Number, MakeSum(32)));
	CHECK_FALSE(Parses(CalculationCorpusTarget::Number, MakeSum(33)));
	CHECK(Parses(CalculationCorpusTarget::Length, MakeNested(31)));
	CHECK(Parses(CalculationCorpusTarget::Length, MakeNested(32)));
	CHECK_FALSE(Parses(CalculationCorpusTarget::Length, MakeNested(33)));
	CHECK(Parses(CalculationCorpusTarget::Number, MakeArguments(31)));
	CHECK(Parses(CalculationCorpusTarget::Number, MakeArguments(32)));
	CHECK_FALSE(Parses(CalculationCorpusTarget::Number, MakeArguments(33)));
	String max_arguments_trailing_comma = MakeArguments(32);
	max_arguments_trailing_comma.pop_back();
	max_arguments_trailing_comma += ',';
	CHECK_FALSE(Parses(CalculationCorpusTarget::Number, max_arguments_trailing_comma));

	CalculationPtr calculation;
	CHECK(ParseCalculation("calc(50% / 2)", {CalculationFinalType::Percent | CalculationFinalType::Number, CalculationPercentageHint::None},
		calculation));
	CHECK_FALSE(ParseCalculation("calc(0.25 + 25%)", {CalculationFinalType::Percent | CalculationFinalType::Number, CalculationPercentageHint::None},
		calculation));
	CHECK(ParseCalculation("calc(50% + 10px)", MakeCalculationParseTarget(CalculationFinalType::Length, CalculationPercentageHint::Length),
		calculation));
	CHECK(ParseCalculation("calc(25% + 10deg)", MakeCalculationParseTarget(CalculationFinalType::Angle, CalculationPercentageHint::Angle),
		calculation));
	CHECK_FALSE(ParseCalculation("calc(50% + 10px)", MakeCalculationParseTarget(CalculationFinalType::Length), calculation));
	CHECK_FALSE(Parses(CalculationCorpusTarget::Length, "calc(0)"));

	const CalculationParseTarget line_height = {
		CalculationFinalType::Number | CalculationFinalType::Length,
		CalculationPercentageHint::Length,
	};
	CHECK(ParseCalculation("calc(20% / 10px)", line_height, calculation));
	CHECK(ParseCalculation("calc(20% + 10px)", line_height, calculation));
	CHECK_FALSE(ParseCalculation("calc(20% + 10deg)", line_height, calculation));

	CHECK(ParseCalculation("calc(2x * 3)", MakeCalculationParseTarget(CalculationFinalType::Resolution), calculation));
	CalculationConstantValue resolution;
	CHECK(EvaluateCalculation(*calculation, resolution));
	CHECK(resolution.value == doctest::Approx(6.f));
	CHECK(resolution.unit == Unit::X);

	CHECK_FALSE(ParseCalculation("calc(10px)", {}, calculation));
	CHECK_FALSE(ParseCalculation("calc(25% + 1)", {CalculationFinalType::Number | CalculationFinalType::Percent, CalculationPercentageHint::None},
		calculation));

	const char* rmlui_length_units[] = {"px", "dp", "vw", "vh", "em", "rem", "in", "cm", "mm", "pt", "pc"};
	for (const char* unit : rmlui_length_units)
	{
		CAPTURE(String(unit));
		CHECK(ParseCalculation(String("calc(1") + unit + ')', MakeCalculationParseTarget(CalculationFinalType::Length), calculation));
	}
}

TEST_CASE("Calculation.parser.non_finite_and_unsupported")
{
	CHECK_FALSE(Parses(CalculationCorpusTarget::Number, "calc(1 / 0)"));
	CHECK_FALSE(Parses(CalculationCorpusTarget::Number, "calc(1 / (2 - 2))"));
	CHECK_FALSE(Parses(CalculationCorpusTarget::Number, "calc(1e1000)"));
	CHECK_FALSE(Parses(CalculationCorpusTarget::Length, "calc(3e38in)"));
	CHECK_FALSE(Parses(CalculationCorpusTarget::Angle, "calc(3e38rad)"));
	CHECK_FALSE(Parses(CalculationCorpusTarget::Number, "calc(infinity)"));
	CHECK_FALSE(Parses(CalculationCorpusTarget::Number, "calc(-infinity)"));
	CHECK_FALSE(Parses(CalculationCorpusTarget::Number, "calc(NaN)"));
	CHECK_FALSE(Parses(CalculationCorpusTarget::Number, "round(1, 1)"));
	CHECK_FALSE(Parses(CalculationCorpusTarget::Number, "calc(pow(2, 3))"));
	CHECK_FALSE(Parses(CalculationCorpusTarget::Length, "calc(1Q)"));
	CHECK_FALSE(Parses(CalculationCorpusTarget::Angle, "calc(1turn)"));
}

TEST_CASE("Calculation.serialization.round_trip")
{
	CalculationPtr first;
	if (!Parses(CalculationCorpusTarget::LengthPercentage, "calc((50% - 20px) * 2)", &first))
	{
		FAIL_CHECK("Round-trip source failed to parse");
		return;
	}
	const String serialized = first->ToString();
	CHECK(serialized.find(" - ") != String::npos);

	CalculationPtr reparsed;
	if (!ParseCalculation(serialized, Target(CalculationCorpusTarget::LengthPercentage), reparsed))
	{
		FAIL_CHECK("Serialized calculation failed to parse");
		return;
	}
	CHECK(*first == *reparsed);

	Property property(first, Unit::CALCULATION);
	CHECK(property.ToString() == serialized);

	PropertyDefinition definition(PropertyId::Width, "auto", false, true);
	property.definition = &definition;
	String definition_value;
	CHECK(definition.GetValue(definition_value, property));
	CHECK(definition_value == serialized);
	CHECK(property.ToString() == serialized);
}

TEST_CASE("Calculation.parser.pure_simplification")
{
	CalculationPtr calculation;
	REQUIRE(ParseCalculation("calc((2 + 3) * 4)", MakeCalculationParseTarget(CalculationFinalType::Number), calculation));
	CHECK(calculation->GetRoot()->kind == Calculation::Kind::Value);
	CHECK(calculation->GetRoot()->value == doctest::Approx(20.f));
	CHECK(calculation->GetRoot()->unit == Unit::NUMBER);

	REQUIRE(ParseCalculation("calc(1in + 48px)", MakeCalculationParseTarget(CalculationFinalType::Length), calculation));
	// Physical lengths remain lexical until computed-value resolution because RmlUi scales them by
	// the mutable dp ratio. Pure evaluation can still prove the context-free 96ppi result.
	CalculationConstantValue physical_value;
	REQUIRE(EvaluateCalculation(*calculation, physical_value));
	CHECK(physical_value.value == doctest::Approx(144.f));
	CHECK(physical_value.unit == Unit::PX);
	CHECK(Any(calculation->GetDependencyMask() & Unit::INCH));

	REQUIRE(ParseCalculation("calc(1em + 20px + 10px)", MakeCalculationParseTarget(CalculationFinalType::Length), calculation));
	CHECK(calculation->GetRoot()->kind == Calculation::Kind::Sum);
	CHECK(calculation->GetRoot()->children.size() == 3);
	CHECK(calculation->GetRoot()->children[1]->kind == Calculation::Kind::Value);
	CHECK(calculation->GetRoot()->children[2]->kind == Calculation::Kind::Value);

	CalculationPtr simplified;
	CHECK(SimplifyCalculation(*calculation, simplified));
	CHECK(*simplified == *calculation);

	CheckEvaluates(CalculationCorpusTarget::Number, "calc(2 + 3 * 4)", 14.f, Unit::NUMBER);
	CheckEvaluates(CalculationCorpusTarget::Number, "calc((2 + 3) * 4)", 20.f, Unit::NUMBER);
	CheckEvaluates(CalculationCorpusTarget::Number, "calc(20 / 5 * 2)", 8.f, Unit::NUMBER);
	CheckEvaluates(CalculationCorpusTarget::Number, "calc(20 / (5 * 2))", 2.f, Unit::NUMBER);

	CalculationPtr equivalent;
	REQUIRE(ParseCalculation("calc(3)", MakeCalculationParseTarget(CalculationFinalType::Number), equivalent));
	CalculationPtr folded;
	REQUIRE(ParseCalculation("calc(1 + 2)", MakeCalculationParseTarget(CalculationFinalType::Number), folded));
	CHECK(*equivalent == *folded);

	// A percentage hint changes the CSS numeric type, but it does not supply the percentage basis.
	// Pure simplification must preserve that dependency even when dimensional exponents cancel.
	const CalculationParseTarget line_height = {
		CalculationFinalType::Number | CalculationFinalType::Length,
		CalculationPercentageHint::Length,
	};
	REQUIRE(ParseCalculation("calc(10% / 1px)", line_height, calculation));
	CalculationConstantValue unresolved_value;
	CHECK_FALSE(EvaluateCalculation(*calculation, unresolved_value));
	CHECK(calculation->ToString().find('%') != String::npos);

	const CalculationParseTarget length_percentage = MakeCalculationParseTarget(CalculationFinalType::Length, CalculationPercentageHint::Length);
	REQUIRE(ParseCalculation("calc((10% * 1%) / 1px)", length_percentage, calculation));
	CHECK_FALSE(EvaluateCalculation(*calculation, unresolved_value));
	CHECK(calculation->ToString().find('%') != String::npos);

	REQUIRE(ParseCalculation("calc(10% / 10%)", line_height, calculation));
	REQUIRE(EvaluateCalculation(*calculation, unresolved_value));
	CHECK(unresolved_value.value == doctest::Approx(1.f));
	CHECK(unresolved_value.unit == Unit::NUMBER);
}

TEST_CASE("Calculation.computed.scalar_parser_and_values")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);
	context->SetDimensions(Vector2i(800, 400));
	context->SetDensityIndependentPixelRatio(1.f);
	ElementDocument* document = context->CreateDocument();
	REQUIRE(document);

	// Central scalar-parser entry point and exact shared parse-target behavior.
	CHECK(document->SetProperty("opacity", "calc(1 / 2)"));
	CHECK(document->SetProperty("width", "calc(1em + 1rem + 1vw + 1vh + 1dp + 1in)"));
	CHECK(document->SetProperty("width", "calc(50% - 20px)"));
	CHECK(document->SetProperty("width", "CaLc(10px + 5px)"));
	CHECK(document->SetProperty("width", "MiN(10px, 20px)"));
	CHECK(document->SetProperty("width", "MaX(10px, 20px)"));
	CHECK(document->SetProperty("width", "ClAmP(10px, 15px, 20px)"));
	TestsShell::SetNumExpectedWarnings(2);
	CHECK_FALSE(document->SetProperty("width", "calc(1 + 2)"));
	CHECK_FALSE(document->SetProperty("line-height", "calc(20% + 10deg)"));

	// Generic number_length_percent is intentionally not a calculation grammar.
	PropertyParser* legacy_parser = StyleSheetSpecification::GetParser("number_length_percent");
	REQUIRE(legacy_parser);
	Property legacy_property;
	CHECK_FALSE(legacy_parser->ParseValue(legacy_property, "calc(10px + 10%)", {}));

	struct ParserCase {
		const char* parser;
		const char* valid;
		const char* invalid;
	};
	const ParserCase parser_cases[] = {
		{"number", "calc(1 / 2)", "calc(1px)"},
		{"length", "calc(1px + 2px)", "calc(1% + 2px)"},
		{"length_percent", "calc(1% + 2px)", "calc(1 + 2px)"},
		{"number_percent", "calc(50% / 2)", "calc(0.25 + 25%)"},
		{"angle", "calc(45deg + 0.5rad)", "calc(1% + 10deg)"},
		{"resolution", "calc(2x * 3)", "calc(1px)"},
		{"line_height", "calc(20% + 10px)", "calc(20% + 10deg)"},
	};
	for (const auto& test : parser_cases)
	{
		CAPTURE(String(test.parser));
		PropertyParser* parser = StyleSheetSpecification::GetParser(test.parser);
		REQUIRE(parser);
		Property parsed;
		CHECK(parser->ParseValue(parsed, test.valid, {}));
		CHECK(parsed.unit == Unit::CALCULATION);
		CHECK_FALSE(parser->ParseValue(parsed, test.invalid, {}));
	}

	// WPT-derived computed length rows. A 20px font is the fixture context used by these rows.
	CHECK(document->SetProperty("font-size", "20px"));
	for (const auto& test : definite_length_cases)
	{
		CAPTURE(String(test.source));
		CAPTURE(String(test.expression));
		REQUIRE(document->SetProperty("width", test.expression));
		document->UpdateDocument();
		const auto width = document->GetComputedValues().width();
		REQUIRE(width.type == Style::LengthPercentageAuto::Length);
		CHECK(width.value == doctest::Approx(strtof(test.expected, nullptr)));
	}

	// WPT line-height computed rows and the plan-required Number-vs-Length inheritance split.
	CHECK(document->SetProperty("font-size", "40px"));
	CHECK(document->SetProperty("line-height", "200%"));
	document->UpdateDocument();
	CHECK(document->GetComputedValues().line_height().value == doctest::Approx(80.f));
	CHECK(document->GetComputedValues().line_height().inherit_type == Style::LineHeight::Number);
	CHECK(document->SetProperty("line-height", "calc(200% + 10px)"));
	document->UpdateDocument();
	CHECK(document->GetComputedValues().line_height().value == doctest::Approx(90.f));
	CHECK(document->GetComputedValues().line_height().inherit_type == Style::LineHeight::Length);
	CHECK(document->SetProperty("line-height", "calc(10px + 0.5em)"));
	document->UpdateDocument();
	CHECK(document->GetComputedValues().line_height().value == doctest::Approx(30.f));
	CHECK(document->GetComputedValues().line_height().inherit_type == Style::LineHeight::Length);
	CHECK(document->SetProperty("line-height", "calc(10px - 0.5em)"));
	document->UpdateDocument();
	CHECK(document->GetComputedValues().line_height().value == doctest::Approx(-10.f));
	CHECK(document->GetComputedValues().line_height().inherit_type == Style::LineHeight::Length);
	CHECK(document->SetProperty("line-height", "calc(200%)"));
	document->UpdateDocument();
	CHECK(document->GetComputedValues().line_height().value == doctest::Approx(80.f));
	CHECK(document->GetComputedValues().line_height().inherit_type == Style::LineHeight::Length);
	CHECK(document->SetProperty("line-height", "calc(2)"));
	document->UpdateDocument();
	CHECK(document->GetComputedValues().line_height().value == doctest::Approx(80.f));
	CHECK(document->GetComputedValues().line_height().inherit_type == Style::LineHeight::Number);

	Element* child = document->AppendChild(document->CreateElement("div"));
	REQUIRE(child);
	CHECK(child->SetProperty("font-size", "calc(50% + 2px)"));
	document->UpdateDocument();
	CHECK(child->GetComputedValues().font_size() == doctest::Approx(22.f));
	CHECK(child->SetProperty("width", "calc(1em + 1rem + 1vw + 1vh + 1dp + 1in)"));
	document->UpdateDocument();
	CHECK(child->GetComputedValues().width().type == Style::LengthPercentageAuto::Length);
	CHECK(child->GetComputedValues().width().value == doctest::Approx(171.f));

	// Physical-unit canonicalization must preserve the existing RmlUi dp-scaled ordinary semantics.
	context->SetDensityIndependentPixelRatio(2.f);
	CHECK(child->SetProperty("width", "1in"));
	document->UpdateDocument();
	const float ordinary_physical = child->GetComputedValues().width().value;
	CHECK(child->SetProperty("width", "calc(1in)"));
	document->UpdateDocument();
	CHECK(child->GetComputedValues().width().type == Style::LengthPercentageAuto::Length);
	CHECK(child->GetComputedValues().width().value == doctest::Approx(ordinary_physical));
	CHECK(ordinary_physical == doctest::Approx(192.f));

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("Calculation.computed.residual_storage_and_var_reparse")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);
	ElementDocument* document = context->CreateDocument();
	REQUIRE(document);

	CHECK(document->SetProperty("width", "calc(50% - 20px)"));
	document->UpdateDocument();
	auto width = document->GetComputedValues().width();
	CHECK(width.type == Style::LengthPercentageAuto::Calculation);
	REQUIRE(bool(width.calculation));
	CHECK(width.calculation->GetResidualForm() == Calculation::ResidualForm::LinearLengthPercentage);
	CHECK(width.calculation->GetLinearPx() == doctest::Approx(-20.f));
	CHECK(width.calculation->GetLinearPercent() == doctest::Approx(50.f));

	CHECK(document->SetProperty("width", "max(50%, 300px)"));
	document->UpdateDocument();
	width = document->GetComputedValues().width();
	CHECK(width.type == Style::LengthPercentageAuto::Calculation);
	REQUIRE(bool(width.calculation));
	CHECK(width.calculation->GetResidualForm() == Calculation::ResidualForm::Tree);

	// Replacing a residual by a definite value must erase the stale sidecar entry.
	CHECK(document->SetProperty("width", "calc(10px + 20px)"));
	document->UpdateDocument();
	width = document->GetComputedValues().width();
	CHECK(width.type == Style::LengthPercentageAuto::Length);
	CHECK(width.value == doctest::Approx(30.f));
	CHECK_FALSE(bool(width.calculation));

	// Existing var() substitution reparses through the calculation-aware owning property definition.
	CHECK(document->SetProperty("--w", "80px"));
	CHECK(document->SetProperty("width", "calc(var(--w) / 2)"));
	document->UpdateDocument();
	width = document->GetComputedValues().width();
	CHECK(width.type == Style::LengthPercentageAuto::Length);
	CHECK(width.value == doctest::Approx(40.f));

	CHECK(document->SetProperty("--w", "100px"));
	document->UpdateDocument();
	width = document->GetComputedValues().width();
	CHECK(width.type == Style::LengthPercentageAuto::Length);
	CHECK(width.value == doctest::Approx(50.f));

	CHECK(document->SetProperty("width", "calc(var(--missing, 60px) / 2)"));
	document->UpdateDocument();
	width = document->GetComputedValues().width();
	CHECK(width.type == Style::LengthPercentageAuto::Length);
	CHECK(width.value == doctest::Approx(30.f));

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("Calculation.computed.wpt_length_percentage_forms")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);
	ElementDocument* document = context->CreateDocument();
	REQUIRE(document);
	CHECK(document->SetProperty("font-size", "20px"));

	for (const auto& test : used_length_percentage_cases)
	{
		CAPTURE(String(test.source));
		CAPTURE(String(test.expression));
		REQUIRE(document->SetProperty("width", test.expression));
		document->UpdateDocument();
		const auto width = document->GetComputedValues().width();
		if (String(test.expression) == "max(5%, 10%, 15%, 20%, 25%, 30%, 35%, 40%, 45%, 50%, 55%, 60%, 65%, 70%, 75%, 80%, 85%, 90%, 95%, 100%)")
		{
			CHECK(width.type == Style::LengthPercentageAuto::Percentage);
			CHECK(width.value == doctest::Approx(100.f));
		}
		else if (String(test.expression) == "calc(calc(3 * 25%))")
		{
			CHECK(width.type == Style::LengthPercentageAuto::Percentage);
			CHECK(width.value == doctest::Approx(75.f));
		}
		else
		{
			CHECK(width.type == Style::LengthPercentageAuto::Calculation);
			REQUIRE(bool(width.calculation));
			if (String(test.expression) == "calc(calc(60%) - 20px)")
				CHECK(width.calculation->GetResidualForm() == Calculation::ResidualForm::LinearLengthPercentage);
		}
	}

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("Calculation.computed.resolver_canonicalization_and_dependencies")
{
	CalculationPtr calculation;
	const CalculationParseTarget length = MakeCalculationParseTarget(CalculationFinalType::Length);
	REQUIRE(ParseCalculation("calc(1in + 1dp + 1vw + 1vh + 1em + 1rem)", length, calculation));
	REQUIRE(bool(calculation));
	const Units dependencies = calculation->GetDependencyMask();
	CHECK(Any(dependencies & Unit::INCH));
	CHECK(Any(dependencies & Unit::DP));
	CHECK(Any(dependencies & Unit::VW));
	CHECK(Any(dependencies & Unit::VH));
	CHECK(Any(dependencies & Unit::EM));
	CHECK(Any(dependencies & Unit::REM));

	CalculationResolverContext context;
	context.font_size = 20.f;
	context.parent_font_size = 30.f;
	context.document_font_size = 16.f;
	context.viewport_dimensions = Vector2f(800.f, 400.f);
	context.dp_ratio = 2.f;
	ResolvedCalculation resolved;
	REQUIRE(ResolveCalculation(*calculation, context, resolved));
	CHECK(resolved.form == Calculation::ResidualForm::Constant);
	CHECK(resolved.is_constant);
	CHECK(resolved.unit == Unit::PX);
	CHECK(resolved.value == doctest::Approx(242.f));

	REQUIRE(ParseCalculation("calc(50% + 2px)", MakeCalculationParseTarget(CalculationFinalType::Length, CalculationPercentageHint::Length),
		calculation));
	context.relative_target = RelativeTarget::ParentFontSize;
	REQUIRE(ResolveCalculation(*calculation, context, resolved));
	CHECK(resolved.is_constant);
	CHECK(resolved.value == doctest::Approx(17.f));

	REQUIRE(ParseCalculation("calc(90deg + 0.5rad)", MakeCalculationParseTarget(CalculationFinalType::Angle), calculation));
	REQUIRE(ResolveCalculation(*calculation, context, resolved));
	CHECK(resolved.is_constant);
	CHECK(resolved.unit == Unit::RAD);
	CHECK(resolved.value == doctest::Approx(float(3.14159265358979323846 * 0.5 + 0.5)));

	REQUIRE(ParseCalculation("calc(2x * 3)", MakeCalculationParseTarget(CalculationFinalType::Resolution), calculation));
	REQUIRE(ResolveCalculation(*calculation, context, resolved));
	CHECK(resolved.is_constant);
	CHECK(resolved.unit == Unit::X);
	CHECK(resolved.value == doctest::Approx(6.f));

	// Dependency collection is from the raw parsed tree, before pure/context simplification.
	REQUIRE(ParseCalculation("calc(1em - 1em + 5px)", length, calculation));
	CHECK(Any(calculation->GetDependencyMask() & Unit::EM));
}

TEST_CASE("Calculation.used_values.wpt_basis_sensitive")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);
	ElementDocument* document = context->CreateDocument();
	REQUIRE(document);
	CHECK(document->SetProperty("font-size", "20px"));
	document->UpdateDocument();

	struct Case {
		const char* expression;
		float basis;
		float expected;
	};
	const Case cases[] = {
		{"min(20px, 10%)", 400.f, 20.f},
		{"min(20px, 10%)", 100.f, 10.f},
		{"max(20px, 10%)", 400.f, 40.f},
		{"max(20px, 10%)", 100.f, 20.f},
		{"min(30px + 10%, 60px + 5%)", 400.f, 70.f},
		{"max(2em + 10%, 1em + 20%)", 400.f, 100.f},
		{"calc(min(1.5em, 10%) + 10px)", 400.f, 40.f},
		{"calc(min(1.5em, 10%) - 10px)", 400.f, 20.f},
		{"calc(min(1.5em, 10%) * 2)", 400.f, 60.f},
		{"calc(min(1.5em, 10%) / 2)", 400.f, 15.f},
		{"calc(max(1em, 15%) + 10px)", 400.f, 70.f},
		{"calc(max(1em, 15%) / 2)", 400.f, 30.f},
		{"calc(calc(60%) - 20px)", 400.f, 220.f},
	};

	for (const Case& test : cases)
	{
		CAPTURE(String(test.expression));
		REQUIRE(document->SetProperty("width", test.expression));
		document->UpdateDocument();
		const auto width = document->GetComputedValues().width();
		CHECK(ResolveValue(width, test.basis) == doctest::Approx(test.expected));
	}

	// Invalid used-value bases take the existing fallback without mutating the calculation.
	REQUIRE(document->SetProperty("width", "max(50%, 300px)"));
	document->UpdateDocument();
	const auto width = document->GetComputedValues().width();
	REQUIRE(width.type == Style::LengthPercentageAuto::Calculation);
	REQUIRE(bool(width.calculation));
	CHECK(ResolveValueOr(width, -1.f, 123.f) == doctest::Approx(123.f));

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("Calculation.used_values.layout_reuses_residual_with_current_geometry")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);
	const String rml = R"RML(
<rml><head><link type="text/rcss" href="/../Tests/Data/style.rcss"/><style>
body { margin: 0; }
#parent { width: 400px; height: 300px; position: relative; }
#child { width: calc(50% - 20px); height: calc(50% - 10px); min-width: calc(25% + 10px); max-width: calc(75% - 10px);
         margin-left: calc(10% + 2px); margin-top: calc(10% + 2px); padding-left: calc(5% + 1px); padding-top: calc(5% + 1px);
         position: relative; left: calc(25% - 5px); }
#absolute { position: absolute; width: 10px; height: 10px; left: calc(25% - 5px); top: calc(50% - 10px); }
</style></head><body><div id="parent"><div id="child"/><div id="absolute"/></div></body></rml>)RML";
	ElementDocument* document = context->LoadDocumentFromMemory(rml);
	REQUIRE(document);
	document->Show();
	TestsShell::RenderLoop();

	Element* parent = document->GetElementById("parent");
	Element* child = document->GetElementById("child");
	Element* absolute = document->GetElementById("absolute");
	REQUIRE(parent);
	REQUIRE(child);
	REQUIRE(absolute);
	const auto initial_width = child->GetComputedValues().width();
	REQUIRE(initial_width.type == Style::LengthPercentageAuto::Calculation);
	REQUIRE(bool(initial_width.calculation));
	const Calculation* calculation_identity = initial_width.calculation.get();
	CHECK(child->GetBox().GetSize().x == doctest::Approx(180.f));
	CHECK(child->GetBox().GetSize().y == doctest::Approx(140.f));
	CHECK(child->GetBox().GetEdge(BoxArea::Margin, BoxEdge::Left) == doctest::Approx(42.f));
	CHECK(child->GetBox().GetEdge(BoxArea::Margin, BoxEdge::Top) == doctest::Approx(32.f));
	CHECK(child->GetBox().GetEdge(BoxArea::Padding, BoxEdge::Left) == doctest::Approx(21.f));
	CHECK(child->GetBox().GetEdge(BoxArea::Padding, BoxEdge::Top) == doctest::Approx(21.f));
	CHECK(absolute->GetOffsetLeft() == doctest::Approx(95.f));
	CHECK(absolute->GetOffsetTop() == doctest::Approx(140.f));

	REQUIRE(parent->SetProperty("width", "1000px"));
	REQUIRE(parent->SetProperty("height", "600px"));
	TestsShell::RenderLoop();
	const auto resized_width = child->GetComputedValues().width();
	REQUIRE(bool(resized_width.calculation));
	CHECK(resized_width.calculation.get() == calculation_identity);
	CHECK(child->GetBox().GetSize().x == doctest::Approx(480.f));
	CHECK(child->GetBox().GetSize().y == doctest::Approx(290.f));
	CHECK(child->GetBox().GetEdge(BoxArea::Margin, BoxEdge::Left) == doctest::Approx(102.f));
	CHECK(child->GetBox().GetEdge(BoxArea::Margin, BoxEdge::Top) == doctest::Approx(62.f));
	CHECK(child->GetBox().GetEdge(BoxArea::Padding, BoxEdge::Left) == doctest::Approx(51.f));
	CHECK(child->GetBox().GetEdge(BoxArea::Padding, BoxEdge::Top) == doctest::Approx(51.f));
	CHECK(absolute->GetOffsetLeft() == doctest::Approx(245.f));
	CHECK(absolute->GetOffsetTop() == doctest::Approx(290.f));

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("Calculation.used_values.flex_table_gap_and_origins")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);
	const String rml = R"RML(
<rml><head><link type="text/rcss" href="/../Tests/Data/style.rcss"/><style>
body { margin: 0; }
#flex { display: flex; width: 400px; height: 100px; column-gap: calc(10% + 2px); }
#flex > div { flex: 0 0 calc(25% - 5px); height: 20px; }
#table { display: table; width: 400px; }
#row { display: table-row; }
#cell { display: table-cell; width: calc(50% + 1px); height: 20px; }
#origin { width: 200px; height: 100px; transform-origin: calc(50% + 10px) calc(25% - 5px); perspective-origin: calc(25% + 5px) calc(75% - 5px); }
#origin-wpt-one { width: 200px; height: 100px; transform-origin: calc(50px + 50%) calc(100% - 30px); }
#origin-wpt-two { width: 200px; height: 100px; transform-origin: calc(-12.5% + 3px) calc(-10px - 50%); }
</style></head><body>
<div id="flex"><div id="f1"/><div id="f2"/></div>
<div id="table"><div id="row"><div id="cell"/></div></div>
<div id="origin"/>
<div id="origin-wpt-one"/>
<div id="origin-wpt-two"/>
</body></rml>)RML";
	ElementDocument* document = context->LoadDocumentFromMemory(rml);
	REQUIRE(document);
	document->Show();
	TestsShell::RenderLoop();

	Element* flex = document->GetElementById("flex");
	Element* f1 = document->GetElementById("f1");
	Element* f2 = document->GetElementById("f2");
	Element* cell = document->GetElementById("cell");
	Element* origin = document->GetElementById("origin");
	Element* origin_wpt_one = document->GetElementById("origin-wpt-one");
	Element* origin_wpt_two = document->GetElementById("origin-wpt-two");
	REQUIRE(flex);
	REQUIRE(f1);
	REQUIRE(f2);
	REQUIRE(cell);
	REQUIRE(origin);
	REQUIRE(origin_wpt_one);
	REQUIRE(origin_wpt_two);
	CHECK(f1->GetBox().GetSize().x == doctest::Approx(95.f));
	CHECK(f2->GetAbsoluteLeft() - (f1->GetAbsoluteLeft() + f1->GetBox().GetSize(BoxArea::Border).x) == doctest::Approx(42.f));
	CHECK(ResolveValue(cell->GetComputedValues().width(), 400.f) == doctest::Approx(201.f));
	CHECK(cell->GetBox().GetSize().x > 0.f);

	const auto& computed = origin->GetComputedValues();
	CHECK(ResolveValue(computed.transform_origin_x(), 200.f) == doctest::Approx(110.f));
	CHECK(ResolveValue(computed.transform_origin_y(), 100.f) == doctest::Approx(20.f));
	CHECK(ResolveValue(computed.perspective_origin_x(), 200.f) == doctest::Approx(55.f));
	CHECK(ResolveValue(computed.perspective_origin_y(), 100.f) == doctest::Approx(70.f));
	const auto& wpt_one = origin_wpt_one->GetComputedValues();
	CHECK(ResolveValue(wpt_one.transform_origin_x(), 200.f) == doctest::Approx(150.f));
	CHECK(ResolveValue(wpt_one.transform_origin_y(), 100.f) == doctest::Approx(70.f));
	const auto& wpt_two = origin_wpt_two->GetComputedValues();
	CHECK(ResolveValue(wpt_two.transform_origin_x(), 200.f) == doctest::Approx(-22.f));
	CHECK(ResolveValue(wpt_two.transform_origin_y(), 100.f) == doctest::Approx(-60.f));

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("Calculation.used_values.wpt_layout_expression_matrix")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);
	ElementDocument* document = context->CreateDocument();
	REQUIRE(document);

	enum class Accessor { Width, Height, MinWidth, MaxWidth, MinHeight, MaxHeight, Left, Right, Top, Bottom, MarginLeft, PaddingLeft };
	struct Case {
		const char* source;
		const char* property;
		const char* expression;
		Accessor accessor;
		float basis;
		float expected;
	};

	const Case cases[] = {
		// calc-width-block-1.html, calc-min-width-block-1.html, calc-max-width-block-1.html.
		{"calc-width-block-1.html", "width", "calc(50% - 3px)", Accessor::Width, 500.f, 247.f},
		{"calc-width-block-1.html", "width", "calc(25% - 3px + 25%)", Accessor::Width, 500.f, 247.f},
		{"calc-width-block-1.html", "width", "calc(25% - 3px + 12.5% * 2)", Accessor::Width, 500.f, 247.f},
		{"calc-width-block-1.html", "width", "calc(25% - 3px + 12.5%*2)", Accessor::Width, 500.f, 247.f},
		{"calc-width-block-1.html", "width", "calc(25% - 3px + 2*12.5%)", Accessor::Width, 500.f, 247.f},
		{"calc-width-block-1.html", "width", "calc(25% - 3px + 2 * 12.5%)", Accessor::Width, 500.f, 247.f},
		{"calc-width-block-1.html", "width", "calc(30% + 20%)", Accessor::Width, 500.f, 250.f},
		{"calc-min-width-block-1.html", "min-width", "calc(50% - 3px)", Accessor::MinWidth, 500.f, 247.f},
		{"calc-min-width-block-1.html", "min-width", "calc(25% - 3px + 25%)", Accessor::MinWidth, 500.f, 247.f},
		{"calc-min-width-block-1.html", "min-width", "calc(25% - 3px + 12.5% * 2)", Accessor::MinWidth, 500.f, 247.f},
		{"calc-min-width-block-1.html", "min-width", "calc(25% - 3px + 12.5%*2)", Accessor::MinWidth, 500.f, 247.f},
		{"calc-min-width-block-1.html", "min-width", "calc(25% - 3px + 2*12.5%)", Accessor::MinWidth, 500.f, 247.f},
		{"calc-min-width-block-1.html", "min-width", "calc(25% - 3px + 2 * 12.5%)", Accessor::MinWidth, 500.f, 247.f},
		{"calc-min-width-block-1.html", "min-width", "calc(30% + 20%)", Accessor::MinWidth, 500.f, 250.f},
		{"calc-max-width-block-1.html", "max-width", "calc(50% - 3px)", Accessor::MaxWidth, 500.f, 247.f},
		{"calc-max-width-block-1.html", "max-width", "calc(25% - 3px + 25%)", Accessor::MaxWidth, 500.f, 247.f},
		{"calc-max-width-block-1.html", "max-width", "calc(25% - 3px + 12.5% * 2)", Accessor::MaxWidth, 500.f, 247.f},
		{"calc-max-width-block-1.html", "max-width", "calc(25% - 3px + 12.5%*2)", Accessor::MaxWidth, 500.f, 247.f},
		{"calc-max-width-block-1.html", "max-width", "calc(25% - 3px + 2*12.5%)", Accessor::MaxWidth, 500.f, 247.f},
		{"calc-max-width-block-1.html", "max-width", "calc(25% - 3px + 2 * 12.5%)", Accessor::MaxWidth, 500.f, 247.f},
		{"calc-max-width-block-1.html", "max-width", "calc(30% + 20%)", Accessor::MaxWidth, 500.f, 250.f},

		// calc-height-block-1.html and min/max-height variants share the same six arithmetic rows.
		{"calc-height-block-1.html", "height", "calc(50px)", Accessor::Height, 100.f, 50.f},
		{"calc-height-block-1.html", "height", "calc(50%)", Accessor::Height, 100.f, 50.f},
		{"calc-height-block-1.html", "height", "calc(25px + 50%)", Accessor::Height, 100.f, 75.f},
		{"calc-height-block-1.html", "height", "calc(150% / 2 - 30px)", Accessor::Height, 100.f, 45.f},
		{"calc-height-block-1.html", "height", "calc(40px + 10% - 20% / 2)", Accessor::Height, 100.f, 40.f},
		{"calc-height-block-1.html", "height", "calc(40px - 10%)", Accessor::Height, 100.f, 30.f},
		{"calc-min-height-block-1.html", "min-height", "calc(50%)", Accessor::MinHeight, 100.f, 50.f},
		{"calc-min-height-block-1.html", "min-height", "calc(25px + 50%)", Accessor::MinHeight, 100.f, 75.f},
		{"calc-min-height-block-1.html", "min-height", "calc(150% / 2 - 30px)", Accessor::MinHeight, 100.f, 45.f},
		{"calc-min-height-block-1.html", "min-height", "calc(40px + 10% - 20% / 2)", Accessor::MinHeight, 100.f, 40.f},
		{"calc-min-height-block-1.html", "min-height", "calc(40px - 10%)", Accessor::MinHeight, 100.f, 30.f},
		{"calc-max-height-block-1.html", "max-height", "calc(50%)", Accessor::MaxHeight, 100.f, 50.f},
		{"calc-max-height-block-1.html", "max-height", "calc(25px + 50%)", Accessor::MaxHeight, 100.f, 75.f},
		{"calc-max-height-block-1.html", "max-height", "calc(150% / 2 - 30px)", Accessor::MaxHeight, 100.f, 45.f},
		{"calc-max-height-block-1.html", "max-height", "calc(40px + 10% - 20% / 2)", Accessor::MaxHeight, 100.f, 40.f},
		{"calc-max-height-block-1.html", "max-height", "calc(40px - 10%)", Accessor::MaxHeight, 100.f, 30.f},
		{"calc-min-height.html", "min-height", "calc(50% - 100px)", Accessor::MinHeight, 400.f, 100.f},

		// Absolute/relative offset WPT arithmetic. The owning positioning mode is exercised separately by the layout fixture.
		{"calc-offsets-absolute-left-1.html", "left", "calc(50px)", Accessor::Left, 100.f, 50.f},
		{"calc-offsets-absolute-left-1.html", "left", "calc(-25%)", Accessor::Left, 100.f, -25.f},
		{"calc-offsets-absolute-left-1.html", "left", "calc(25px + 25%)", Accessor::Left, 100.f, 50.f},
		{"calc-offsets-absolute-left-1.html", "left", "calc(-75% / 2 + 30px)", Accessor::Left, 100.f, -7.5f},
		{"calc-offsets-absolute-left-1.html", "left", "calc(40px + 5% - 10% / 2)", Accessor::Left, 100.f, 40.f},
		{"calc-offsets-absolute-left-1.html", "left", "calc(5% - 40px)", Accessor::Left, 100.f, -35.f},
		{"calc-offsets-absolute-right-1.html", "right", "calc(-50px)", Accessor::Right, 100.f, -50.f},
		{"calc-offsets-absolute-right-1.html", "right", "calc(25%)", Accessor::Right, 100.f, 25.f},
		{"calc-offsets-absolute-right-1.html", "right", "calc(-25px - 25%)", Accessor::Right, 100.f, -50.f},
		{"calc-offsets-absolute-right-1.html", "right", "calc(75% / 2 - 30px)", Accessor::Right, 100.f, 7.5f},
		{"calc-offsets-absolute-right-1.html", "right", "calc(-40px - 5% + 10% / 2)", Accessor::Right, 100.f, -40.f},
		{"calc-offsets-absolute-right-1.html", "right", "calc(-5% + 40px)", Accessor::Right, 100.f, 35.f},
		{"calc-offsets-absolute-top-1.html", "top", "calc(50px)", Accessor::Top, 100.f, 50.f},
		{"calc-offsets-absolute-top-1.html", "top", "calc(50%)", Accessor::Top, 100.f, 50.f},
		{"calc-offsets-absolute-top-1.html", "top", "calc(25px + 50%)", Accessor::Top, 100.f, 75.f},
		{"calc-offsets-absolute-top-1.html", "top", "calc(150% / 2 - 30px)", Accessor::Top, 100.f, 45.f},
		{"calc-offsets-absolute-top-1.html", "top", "calc(40px + 10% - 20% / 2)", Accessor::Top, 100.f, 40.f},
		{"calc-offsets-absolute-top-1.html", "top", "calc(40px - 10%)", Accessor::Top, 100.f, 30.f},
		{"calc-offsets-absolute-bottom-1.html", "bottom", "calc(-50px)", Accessor::Bottom, 100.f, -50.f},
		{"calc-offsets-absolute-bottom-1.html", "bottom", "calc(-50%)", Accessor::Bottom, 100.f, -50.f},
		{"calc-offsets-absolute-bottom-1.html", "bottom", "calc(-25px - 50%)", Accessor::Bottom, 100.f, -75.f},
		{"calc-offsets-absolute-bottom-1.html", "bottom", "calc(-150% / 2 + 30px)", Accessor::Bottom, 100.f, -45.f},
		{"calc-offsets-absolute-bottom-1.html", "bottom", "calc(-40px - 10% + 20% / 2)", Accessor::Bottom, 100.f, -40.f},
		{"calc-offsets-absolute-bottom-1.html", "bottom", "calc(-40px + 10%)", Accessor::Bottom, 100.f, -30.f},
		{"calc-offsets-relative-left-1.html", "left", "calc(50px)", Accessor::Left, 100.f, 50.f},
		{"calc-offsets-relative-left-1.html", "left", "calc(-50%)", Accessor::Left, 100.f, -50.f},
		{"calc-offsets-relative-left-1.html", "left", "calc(25px + 50%)", Accessor::Left, 100.f, 75.f},
		{"calc-offsets-relative-left-1.html", "left", "calc(-150% / 2 + 30px)", Accessor::Left, 100.f, -45.f},
		{"calc-offsets-relative-left-1.html", "left", "calc(40px + 10% - 20% / 2)", Accessor::Left, 100.f, 40.f},
		{"calc-offsets-relative-left-1.html", "left", "calc(10% - 40px)", Accessor::Left, 100.f, -30.f},
		{"calc-offsets-relative-right-1.html", "right", "calc(-50px)", Accessor::Right, 100.f, -50.f},
		{"calc-offsets-relative-right-1.html", "right", "calc(50%)", Accessor::Right, 100.f, 50.f},
		{"calc-offsets-relative-right-1.html", "right", "calc(-25px - 50%)", Accessor::Right, 100.f, -75.f},
		{"calc-offsets-relative-right-1.html", "right", "calc(150% / 2 - 30px)", Accessor::Right, 100.f, 45.f},
		{"calc-offsets-relative-right-1.html", "right", "calc(-40px - 10% + 20% / 2)", Accessor::Right, 100.f, -40.f},
		{"calc-offsets-relative-right-1.html", "right", "calc(-10% + 40px)", Accessor::Right, 100.f, 30.f},
		{"calc-offsets-relative-top-1.html", "top", "calc(50px)", Accessor::Top, 100.f, 50.f},
		{"calc-offsets-relative-top-1.html", "top", "calc(50%)", Accessor::Top, 100.f, 50.f},
		{"calc-offsets-relative-top-1.html", "top", "calc(25px + 50%)", Accessor::Top, 100.f, 75.f},
		{"calc-offsets-relative-top-1.html", "top", "calc(150% / 2 - 30px)", Accessor::Top, 100.f, 45.f},
		{"calc-offsets-relative-top-1.html", "top", "calc(40px + 10% - 20% / 2)", Accessor::Top, 100.f, 40.f},
		{"calc-offsets-relative-top-1.html", "top", "calc(40px - 10%)", Accessor::Top, 100.f, 30.f},
		{"calc-offsets-relative-bottom-1.html", "bottom", "calc(-50px)", Accessor::Bottom, 100.f, -50.f},
		{"calc-offsets-relative-bottom-1.html", "bottom", "calc(-50%)", Accessor::Bottom, 100.f, -50.f},
		{"calc-offsets-relative-bottom-1.html", "bottom", "calc(-25px - 50%)", Accessor::Bottom, 100.f, -75.f},
		{"calc-offsets-relative-bottom-1.html", "bottom", "calc(-150% / 2 + 30px)", Accessor::Bottom, 100.f, -45.f},
		{"calc-offsets-relative-bottom-1.html", "bottom", "calc(-40px - 10% + 20% / 2)", Accessor::Bottom, 100.f, -40.f},
		{"calc-offsets-relative-bottom-1.html", "bottom", "calc(-40px + 10%)", Accessor::Bottom, 100.f, -30.f},

		// Margin and padding percentages use containing-block width on every side in RmlUi/CSS.
		{"calc-margin-block-1.html", "margin-left", "calc(10px + 1%)", Accessor::MarginLeft, 500.f, 15.f},
		{"calc-margin-block-1.html", "margin-left", "calc(30px - 1%)", Accessor::MarginLeft, 500.f, 25.f},
		{"calc-padding-block-1.html", "padding-left", "calc(10px + 1%)", Accessor::PaddingLeft, 500.f, 15.f},
		{"calc-padding-block-1.html", "padding-left", "calc(30px - 1%)", Accessor::PaddingLeft, 500.f, 25.f},
	};

	for (const Case& test : cases)
	{
		CAPTURE(String(test.source));
		CAPTURE(String(test.property));
		CAPTURE(String(test.expression));
		REQUIRE(document->SetProperty(test.property, test.expression));
		document->UpdateDocument();
		const auto& computed = document->GetComputedValues();
		float value = 0.f;
		switch (test.accessor)
		{
		case Accessor::Width: value = ResolveValue(computed.width(), test.basis); break;
		case Accessor::Height: value = ResolveValue(computed.height(), test.basis); break;
		case Accessor::MinWidth: value = ResolveValue(computed.min_width(), test.basis); break;
		case Accessor::MaxWidth: value = ResolveValue(computed.max_width(), test.basis); break;
		case Accessor::MinHeight: value = ResolveValue(computed.min_height(), test.basis); break;
		case Accessor::MaxHeight: value = ResolveValue(computed.max_height(), test.basis); break;
		case Accessor::Left: value = ResolveValue(computed.left(), test.basis); break;
		case Accessor::Right: value = ResolveValue(computed.right(), test.basis); break;
		case Accessor::Top: value = ResolveValue(computed.top(), test.basis); break;
		case Accessor::Bottom: value = ResolveValue(computed.bottom(), test.basis); break;
		case Accessor::MarginLeft: value = ResolveValue(computed.margin_left(), test.basis); break;
		case Accessor::PaddingLeft: value = ResolveValue(computed.padding_left(), test.basis); break;
		}
		CHECK(value == doctest::Approx(test.expected));
	}

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("Calculation.used_values.wpt_intrinsic_width_vectors")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);
	const String rml = R"RML(
<rml><head><link type="text/rcss" href="/../Tests/Data/style.rcss"/><style>
body { width: 500px; font-size: 10px; }
.outer { float: left; clear: left; height: 5px; }
.inner { width: 200px; }
</style></head><body>
<div id="w1" class="outer" style="width: calc(50% - 3px)"><div class="inner"/></div>
<div id="w2" class="outer" style="width: calc(5em - 3px)"><div class="inner"/></div>
<div id="w3" class="outer" style="width: calc(5em - 0%)"><div class="inner"/></div>
<div id="w4" class="outer" style="width: calc(50%)"><div class="inner"/></div>
<div id="w5" class="outer" style="width: calc(50px)"><div class="inner"/></div>
<div id="w6" class="outer" style="width: calc(25% + 25%)"><div class="inner"/></div>
<div id="max1" class="outer" style="max-width: calc(50% - 3px)"><div class="inner"/></div>
<div id="max2" class="outer" style="max-width: calc(5em - 3px)"><div class="inner"/></div>
<div id="max3" class="outer" style="max-width: calc(5em - 0%)"><div class="inner"/></div>
<div id="max4" class="outer" style="max-width: calc(50%)"><div class="inner"/></div>
<div id="max5" class="outer" style="max-width: calc(50px)"><div class="inner"/></div>
<div id="max6" class="outer" style="max-width: calc(25% + 25%)"><div class="inner"/></div>
<div id="min1" class="outer" style="min-width: calc(50% - 3px)"><div class="inner"/></div>
<div id="min2" class="outer" style="min-width: calc(5em - 3px)"><div class="inner"/></div>
<div id="min3" class="outer" style="min-width: calc(5em - 0%)"><div class="inner"/></div>
<div id="min4" class="outer" style="min-width: calc(50%)"><div class="inner"/></div>
<div id="min5" class="outer" style="min-width: calc(50px)"><div class="inner"/></div>
<div id="min6" class="outer" style="min-width: calc(25% + 25%)"><div class="inner"/></div>
</body></rml>)RML";
	ElementDocument* document = context->LoadDocumentFromMemory(rml);
	REQUIRE(document);
	document->Show();
	TestsShell::RenderLoop();

	const char* ids[] = {"w1", "w2", "w3", "w4", "w5", "w6", "max1", "max2", "max3", "max4", "max5", "max6", "min1", "min2", "min3", "min4", "min5",
		"min6"};
	const float expected[] = {247.f, 47.f, 50.f, 250.f, 50.f, 250.f, 200.f, 47.f, 50.f, 200.f, 50.f, 200.f, 247.f, 200.f, 200.f, 250.f, 200.f, 250.f};
	for (size_t i = 0; i < Count(ids); ++i)
	{
		CAPTURE(String(ids[i]));
		Element* element = document->GetElementById(ids[i]);
		REQUIRE(element);
		CHECK(element->GetBox().GetSize().x == doctest::Approx(expected[i]));
	}

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("Calculation.properties.direct_scalar_and_shorthand_coverage")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);
	context->SetDimensions(Vector2i(800, 400));
	context->SetDensityIndependentPixelRatio(1.f);
	ElementDocument* document = context->CreateDocument();
	REQUIRE(document);

	// Source: css/css-values/calc-numbers.html. The in-scope authored opacity rows use Number math.
	CHECK(document->SetProperty("opacity", "calc(2 / 4)"));
	document->UpdateDocument();
	CHECK(document->GetComputedValues().opacity() == doctest::Approx(0.5f));

	// Direct scalar owners must resolve calculations before reaching legacy scalar extraction paths.
	CHECK(document->SetProperty("clip", "calc(1 + 2)"));
	CHECK(document->SetProperty("font-weight", "calc(300 + 100)"));
	CHECK(document->SetProperty("flex-grow", "calc(1 + 2)"));
	CHECK(document->SetProperty("flex-shrink", "calc(4 / 2)"));
	CHECK(document->SetProperty("font-size", "20px"));
	CHECK(document->SetProperty("letter-spacing", "calc(1em + 2px)"));
	document->UpdateDocument();
	CHECK(document->GetComputedValues().clip().GetNumber() == 3);
	CHECK(document->GetComputedValues().font_weight() == static_cast<Style::FontWeight>(400));
	CHECK(document->GetComputedValues().flex_grow() == doctest::Approx(3.f));
	CHECK(document->GetComputedValues().flex_shrink() == doctest::Approx(2.f));
	CHECK(document->GetComputedValues().letter_spacing() == doctest::Approx(22.f));

	// Each registered longhand below must accept math through its
	// existing parser category; this deliberately does not broaden any property's grammar.
	struct DirectPropertyCase {
		const char* property;
		const char* expression;
	};
	const DirectPropertyCase direct_property_cases[] = {
		{"margin-top", "calc(1px + 1%)"},
		{"margin-right", "calc(1px + 1%)"},
		{"margin-bottom", "calc(1px + 1%)"},
		{"margin-left", "calc(1px + 1%)"},
		{"padding-top", "calc(1px + 1%)"},
		{"padding-right", "calc(1px + 1%)"},
		{"padding-bottom", "calc(1px + 1%)"},
		{"padding-left", "calc(1px + 1%)"},
		{"border-top-width", "calc(1px + 2px)"},
		{"border-right-width", "calc(1px + 2px)"},
		{"border-bottom-width", "calc(1px + 2px)"},
		{"border-left-width", "calc(1px + 2px)"},
		{"border-top-left-radius", "calc(1px + 2px)"},
		{"border-top-right-radius", "calc(1px + 2px)"},
		{"border-bottom-right-radius", "calc(1px + 2px)"},
		{"border-bottom-left-radius", "calc(1px + 2px)"},
		{"top", "calc(1px + 1%)"},
		{"right", "calc(1px + 1%)"},
		{"bottom", "calc(1px + 1%)"},
		{"left", "calc(1px + 1%)"},
		{"z-index", "calc(1 + 2)"},
		{"width", "calc(1px + 1%)"},
		{"min-width", "calc(1px + 1%)"},
		{"max-width", "calc(1px + 1%)"},
		{"height", "calc(1px + 1%)"},
		{"min-height", "calc(1px + 1%)"},
		{"max-height", "calc(1px + 1%)"},
		{"line-height", "calc(1em + 1px)"},
		{"vertical-align", "calc(1px + 1%)"},
		{"clip", "calc(1 + 2)"},
		{"opacity", "calc(1 / 2)"},
		{"font-weight", "calc(300 + 100)"},
		{"font-size", "calc(1em + 1px)"},
		{"letter-spacing", "calc(1em + 1px)"},
		{"row-gap", "calc(1px + 1%)"},
		{"column-gap", "calc(1px + 1%)"},
		{"scrollbar-margin", "calc(1px + 2px)"},
		{"perspective", "calc(1px + 2px)"},
		{"perspective-origin-x", "calc(1px + 1%)"},
		{"perspective-origin-y", "calc(1px + 1%)"},
		{"transform-origin-x", "calc(1px + 1%)"},
		{"transform-origin-y", "calc(1px + 1%)"},
		{"transform-origin-z", "calc(1px + 2px)"},
		{"flex-basis", "calc(1px + 1%)"},
		{"flex-grow", "calc(1 + 2)"},
		{"flex-shrink", "calc(4 / 2)"},
	};
	for (const DirectPropertyCase& test : direct_property_cases)
	{
		CAPTURE(String(test.property));
		CAPTURE(String(test.expression));
		CHECK(document->SetProperty(test.property, test.expression));
	}
	document->UpdateDocument();
	CHECK(document->GetComputedValues().vertical_align().type == Style::VerticalAlign::Length);

	// Required shorthands stay on PropertySpecification::ParsePropertyValues(). Nested function
	// whitespace and commas must remain inside the shorthand component rather than being retokenized.
	CHECK(document->SetProperty("margin", "calc(10px + min(5px, 10px)) calc(20px + max(1px, 2px))"));
	CHECK(document->SetProperty("padding", "calc(1px + min(2px, 3px)) calc(4px + max(5px, 6px))"));
	CHECK(document->SetProperty("border-width", "calc(1px + min(2px, 3px)) calc(4px + max(5px, 6px))"));
	CHECK(document->SetProperty("border-top", "calc(1px + min(2px, 3px)) #fff"));
	CHECK(document->SetProperty("border-right", "calc(1px + min(2px, 3px)) #fff"));
	CHECK(document->SetProperty("border-bottom", "calc(1px + min(2px, 3px)) #fff"));
	CHECK(document->SetProperty("border-left", "calc(1px + min(2px, 3px)) #fff"));
	CHECK(document->SetProperty("border", "calc(1px + min(2px, 3px)) #fff"));
	CHECK(document->SetProperty("border-radius", "calc(1px + min(2px, 3px)) calc(4px + max(5px, 6px))"));
	CHECK(document->SetProperty("inset", "calc(1px + min(2px, 3px)) calc(4px + max(5px, 6px))"));
	CHECK(document->SetProperty("font", "normal calc(300 + 100) calc(10px + min(10px, 20px)) LatoLatin"));
	CHECK(document->SetProperty("gap", "calc(1px + min(2px, 3px)) calc(4px + max(5px, 6px))"));
	CHECK(document->SetProperty("perspective-origin", "calc(25% + min(1px, 2px)) calc(75% - max(1px, 2px))"));
	CHECK(document->SetProperty("transform-origin", "calc(25% + min(1px, 2px)) calc(75% - max(1px, 2px)) calc(1px + min(2px, 3px))"));
	CHECK(document->SetProperty("flex", "calc(1 + 2) calc(4 / 2) calc(25% + min(1px, 2px))"));
	document->UpdateDocument();
	CHECK(ResolveValue(document->GetComputedValues().margin_top(), 100.f) == doctest::Approx(15.f));
	CHECK(ResolveValue(document->GetComputedValues().margin_right(), 100.f) == doctest::Approx(22.f));
	CHECK(ResolveValue(document->GetComputedValues().padding_top(), 100.f) == doctest::Approx(3.f));
	CHECK(document->GetComputedValues().flex_grow() == doctest::Approx(3.f));
	CHECK(document->GetComputedValues().flex_shrink() == doctest::Approx(2.f));
	CHECK(ResolveValue(document->GetComputedValues().flex_basis(), 100.f) == doctest::Approx(26.f));
	CHECK(document->GetComputedValues().border_top_width() == doctest::Approx(3.f));
	CHECK(document->GetComputedValues().border_top_left_radius() == doctest::Approx(3.f));
	CHECK(document->GetComputedValues().font_weight() == static_cast<Style::FontWeight>(400));
	CHECK(document->GetComputedValues().font_size() == doctest::Approx(20.f));

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("Calculation.properties.custom_property_and_context_invalidation")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);
	context->SetDimensions(Vector2i(800, 400));
	context->SetDensityIndependentPixelRatio(1.f);
	const String rml = R"RML(
<rml><head><link type="text/rcss" href="/../Tests/Data/style.rcss"/></head><body>
<div id="parent" style="font-size: 20px; width: 400px">
<div id="direct" style="width: calc(1em + 1rem + 1vw + 1vh + 1dp + 1in)"/>
<div id="variable" style="--w: calc(1em + 1rem + 1vw + 1vh + 1dp + 1in); width: var(--w)"/>
<div id="shorthand" style="--m: calc(1em + 1vw); margin: var(--m)"/>
<div id="residual" style="width: calc(50% - 10px)"/>
</div>
</body></rml>)RML";
	ElementDocument* document = context->LoadDocumentFromMemory(rml);
	REQUIRE(document);
	document->Show();
	TestsShell::RenderLoop();

	Element* parent = document->GetElementById("parent");
	Element* direct = document->GetElementById("direct");
	Element* variable = document->GetElementById("variable");
	Element* shorthand = document->GetElementById("shorthand");
	Element* residual = document->GetElementById("residual");
	REQUIRE(parent);
	REQUIRE(direct);
	REQUIRE(variable);
	REQUIRE(shorthand);
	REQUIRE(residual);

	auto direct_width = [&]() { return direct->GetComputedValues().width().value; };
	auto variable_width = [&]() { return variable->GetComputedValues().width().value; };
	CHECK(direct_width() == doctest::Approx(variable_width()));
	const float initial = direct_width();
	const float shorthand_initial = ResolveValue(shorthand->GetComputedValues().margin_left(), 400.f);

	// Parent/local font-size changes must invalidate both direct and var-backed em calculations.
	REQUIRE(parent->SetProperty("font-size", "30px"));
	TestsShell::RenderLoop();
	CHECK(direct_width() == doctest::Approx(variable_width()));
	CHECK(direct_width() == doctest::Approx(initial + 10.f));
	CHECK(ResolveValue(shorthand->GetComputedValues().margin_left(), 400.f) == doctest::Approx(shorthand_initial + 10.f));

	// Root font-size changes invalidate rem hidden by var() as well as direct calculations.
	const float before_root_font = direct_width();
	REQUIRE(document->SetProperty("font-size", "24px"));
	TestsShell::RenderLoop();
	CHECK(direct_width() == doctest::Approx(variable_width()));
	CHECK(direct_width() > before_root_font);

	// Viewport and dp-ratio invalidation must inspect calculation dependencies rather than the
	// outer Unit::CALCULATION / Unit::VAR_EXPRESSION token.
	const float before_viewport = direct_width();
	context->SetDimensions(Vector2i(1000, 500));
	TestsShell::RenderLoop();
	CHECK(direct_width() == doctest::Approx(variable_width()));
	CHECK(direct_width() > before_viewport);
	CHECK(ResolveValue(shorthand->GetComputedValues().margin_left(), 400.f) > shorthand_initial + 10.f);
	const float before_dp = direct_width();
	context->SetDensityIndependentPixelRatio(2.f);
	TestsShell::RenderLoop();
	CHECK(direct_width() == doctest::Approx(variable_width()));
	CHECK(direct_width() > before_dp);

	// A variable change reparses through the normal property path and must replace the cached
	// dependency with the newly resolved px-only value.
	REQUIRE(variable->SetProperty("--w", "50px"));
	TestsShell::RenderLoop();
	CHECK(variable_width() == doctest::Approx(50.f));
	context->SetDimensions(Vector2i(1200, 600));
	context->SetDensityIndependentPixelRatio(1.f);
	TestsShell::RenderLoop();
	CHECK(variable_width() == doctest::Approx(50.f));

	// Pure containing-block changes re-evaluate residual used values without forcing computed-value
	// canonicalization or reparsing.
	const auto residual_before = residual->GetComputedValues().width();
	REQUIRE(residual_before.type == Style::LengthPercentageAuto::Calculation);
	REQUIRE(bool(residual_before.calculation));
	const Calculation* residual_identity = residual_before.calculation.get();
	CHECK(residual->GetBox().GetSize().x == doctest::Approx(190.f));
	REQUIRE(parent->SetProperty("width", "600px"));
	TestsShell::RenderLoop();
	const auto residual_after = residual->GetComputedValues().width();
	REQUIRE(bool(residual_after.calculation));
	CHECK(residual_after.calculation.get() == residual_identity);
	CHECK(residual->GetBox().GetSize().x == doctest::Approx(290.f));

	// Existing fallback and invalid-substitution behavior stays in the normal var() resolver.
	CHECK(variable->SetProperty("width", "calc(var(--missing, 60px) / 2)"));
	TestsShell::RenderLoop();
	CHECK(variable_width() == doctest::Approx(30.f));
	CHECK(variable->SetProperty("--bad", "red"));
	CHECK(variable->SetProperty("width", "calc(var(--bad) + 1px)"));
	TestsShell::SetNumExpectedWarnings(1);
	TestsShell::RenderLoop();

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("Calculation.time.wpt")
{
	for (const auto& test : time_cases)
		CheckEvaluatesTime(test);
	for (const auto& test : time_clamp_cases)
		CheckEvaluatesTime(test);
	for (const char* expression : time_invalid_cases)
	{
		CAPTURE(String(expression));
		CHECK_FALSE(Parses(CalculationCorpusTarget::Time, expression));
	}

	// Time units are lexical only for time-accepting calculation targets.
	CHECK_FALSE(Parses(CalculationCorpusTarget::Number, "calc(1s)"));
	CHECK_FALSE(Parses(CalculationCorpusTarget::Length, "calc(1ms)"));
	CHECK_FALSE(Parses(CalculationCorpusTarget::Time, "calc(50%)"));
	CHECK_FALSE(Parses(CalculationCorpusTarget::Time, "calc(1px)"));
}

TEST_CASE("Calculation.time.animation_transition_integration")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);
	ElementDocument* document = context->CreateDocument();
	REQUIRE(document);

	REQUIRE(document->SetProperty("animation", "spin calc(4s + 1ms) calc(500ms) linear-in calc(2)"));
	const Property* animation_property = document->GetLocalProperty("animation");
	REQUIRE(animation_property);
	REQUIRE(animation_property->unit == Unit::ANIMATION);
	const AnimationList animation_list = animation_property->value.GetReference<AnimationList>();
	REQUIRE(animation_list.size() == 1);
	CHECK(animation_list[0].duration == doctest::Approx(4.001f));
	CHECK(animation_list[0].delay == doctest::Approx(0.5f));
	CHECK(animation_list[0].num_iterations == 2);
	CHECK(animation_list[0].name == "spin");

	// Nested spaces/commas stay inside the math component, while a top-level comma still separates list items.
	REQUIRE(document->SetProperty("animation", "spin min(1s, max(500ms, 2s)), other clamp(none, 1500ms, 2s)"));
	animation_property = document->GetLocalProperty("animation");
	REQUIRE(animation_property);
	const AnimationList nested_animations = animation_property->value.GetReference<AnimationList>();
	REQUIRE(nested_animations.size() == 2);
	CHECK(nested_animations[0].duration == doctest::Approx(1.f));
	CHECK(nested_animations[1].duration == doctest::Approx(1.5f));
	CHECK(nested_animations[1].name == "other");

	// The shared splitter is quote-aware even though the outer inline-declaration parser does not
	// admit quoted animation names. Exercise the property parser directly so a comma/space inside a
	// quoted component cannot be mistaken for either kind of top-level separator.
	PropertyParserAnimation direct_animation_parser(PropertyParserAnimation::ANIMATION_PARSER);
	Property quoted_animation_property;
	REQUIRE(direct_animation_parser.ParseValue(quoted_animation_property, "\"quoted, name\" min(1s, max(500ms, 2s)), other 1s", ParameterMap{}));
	const AnimationList quoted_animations = quoted_animation_property.value.GetReference<AnimationList>();
	REQUIRE(quoted_animations.size() == 2);
	CHECK(quoted_animations[0].name == "\"quoted, name\"");
	CHECK(quoted_animations[0].duration == doctest::Approx(1.f));

	REQUIRE(document->SetProperty("transition", "opacity calc(1s - 200ms) calc(-250ms) linear-in 0.5"));
	const Property* transition_property = document->GetLocalProperty("transition");
	REQUIRE(transition_property);
	REQUIRE(transition_property->unit == Unit::TRANSITION);
	const TransitionList transition_list = transition_property->value.GetReference<TransitionList>();
	REQUIRE(transition_list.transitions.size() == 1);
	CHECK(transition_list.transitions[0].duration == doctest::Approx(0.8f));
	CHECK(transition_list.transitions[0].delay == doctest::Approx(-0.25f));
	CHECK(transition_list.transitions[0].reverse_adjustment_factor == doctest::Approx(0.5f));

	REQUIRE(document->SetProperty("transition", "opacity min(1s, max(500ms, 2s)), width clamp(none, 1500ms, 2s)"));
	transition_property = document->GetLocalProperty("transition");
	REQUIRE(transition_property);
	const TransitionList nested_transitions = transition_property->value.GetReference<TransitionList>();
	REQUIRE(nested_transitions.transitions.size() == 2);
	CHECK(nested_transitions.transitions[0].duration == doctest::Approx(1.f));
	CHECK(nested_transitions.transitions[1].duration == doctest::Approx(1.5f));

	// Final Number math in animation uses exactly the ordinary iteration-count rounding and validity policy.
	REQUIRE(document->SetProperty("animation", "spin 1s 2.6"));
	animation_property = document->GetLocalProperty("animation");
	REQUIRE(animation_property);
	const int ordinary_iterations = animation_property->value.GetReference<AnimationList>()[0].num_iterations;
	REQUIRE(document->SetProperty("animation", "spin 1s calc(2.6)"));
	animation_property = document->GetLocalProperty("animation");
	REQUIRE(animation_property);
	CHECK(animation_property->value.GetReference<AnimationList>()[0].num_iterations == ordinary_iterations);

	TestsShell::SetNumExpectedWarnings(12);
	CHECK_FALSE(document->SetProperty("animation", "spin calc(50%)"));
	CHECK_FALSE(document->SetProperty("animation", "spin calc(1px)"));
	CHECK_FALSE(document->SetProperty("animation", "calc(1px) 1s"));
	CHECK_FALSE(document->SetProperty("animation", "calc(1s + ) 1s"));
	CHECK_FALSE(document->SetProperty("transition", "opacity calc(50%)"));
	CHECK_FALSE(document->SetProperty("transition", "opacity calc(1px)"));
	CHECK_FALSE(document->SetProperty("transition", "opacity calc(2)"));
	CHECK_FALSE(document->SetProperty("animation", "spin 0s"));
	CHECK_FALSE(document->SetProperty("animation", "spin calc(0s)"));
	CHECK_FALSE(document->SetProperty("transition", "opacity calc(0s)"));
	CHECK_FALSE(document->SetProperty("animation", "spin 1s 0.4"));
	CHECK_FALSE(document->SetProperty("animation", "spin 1s calc(0.4)"));

	// Existing delay policy is unchanged: negative delays are accepted for both ordinary and calculated time values.
	CHECK(document->SetProperty("animation", "spin 1s -0.5s"));
	CHECK(document->SetProperty("animation", "spin calc(1s) calc(-500ms)"));

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("Calculation.compound.angle_transform_wpt_and_arguments")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);
	ElementDocument* document = context->CreateDocument();
	REQUIRE(document);

	// Sources: css/css-values/calc-angle-values.html and minmax-angle-computed.html.
	// RmlUi does not support grad/turn units here; every adapted deg/rad row is exercised through the actual transform owner.
	for (const CalculationCorpusCase& test : angle_cases)
	{
		CAPTURE(String(test.source));
		CAPTURE(String(test.expression));
		CHECK(document->SetProperty("transform", CreateString("rotate(%s)", test.expression)));
	}
	for (const CalculationCorpusCase& test : pure_minmax_angle_cases)
	{
		CAPTURE(String(test.source));
		CAPTURE(String(test.expression));
		String expression = test.expression;
		int parenthesis_depth = 0;
		for (const char c : expression)
			parenthesis_depth += (c == '(' ? 1 : (c == ')' ? -1 : 0));
		while (parenthesis_depth-- > 0)
			expression += ')';
		CHECK(document->SetProperty("transform", "rotate(" + expression + ")"));
	}
	// Every existing numeric transform argument accepts an appropriately typed calculation. Nested commas
	// remain inside min/max/clamp rather than being mistaken for transform argument separators.
	const String transform_values[] = {
		"matrix(calc(1), calc(0), calc(0), calc(1), calc(10), calc(20))",
		String("matrix3d(calc(1),calc(0),calc(0),calc(0),calc(0),calc(1),calc(0),calc(0),calc(0),calc(0),calc(1),calc(0),calc(10),calc(20),calc(30),"
			   "calc(1)") +
			")",
		"translateX(calc(10px + 5%))",
		"translateY(calc(10px + 5%))",
		"translateZ(calc(10px + 2px))",
		"translate(min(10px, max(5px, 20px)), calc(25% + 1em))",
		"translate3d(calc(10% + 1px), calc(20% + 2px), calc(3px + 4px))",
		"scaleX(calc(1 + 1))",
		"scaleY(calc(1 + 1))",
		"scaleZ(calc(1 + 1))",
		"scale(calc(1 + 1))",
		"scale(calc(1 + 1), max(2, 3))",
		"scale3d(calc(1 + 1), min(2, 3), clamp(1, 2, 3))",
		"rotateX(calc(45deg + 45deg))",
		"rotateY(max(1rad, 45deg))",
		"rotateZ(min(90deg, 2rad))",
		"rotate(calc(90deg / 2))",
		"rotate3d(calc(1), calc(0), calc(0), calc(45deg + 45deg))",
		"skewX(calc(10deg + 5deg))",
		"skewY(calc(10deg + 5deg))",
		"skew(calc(10deg + 5deg), max(10deg, 5deg))",
		"perspective(calc(100px + 1em))",
	};
	for (const String& value : transform_values)
	{
		CAPTURE(value);
		CHECK(document->SetProperty("transform", value));
	}

	TestsShell::SetNumExpectedWarnings(4);
	CHECK_FALSE(document->SetProperty("transform", "translateX(calc(1 + 2))"));
	CHECK_FALSE(document->SetProperty("transform", "scaleX(calc(1px + 2px))"));
	CHECK_FALSE(document->SetProperty("transform", "rotate(calc(1px + 2px))"));
	CHECK_FALSE(document->SetProperty("transform", "perspective(calc(50%))"));

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("Calculation.compound.gradient_wpt")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);
	ElementDocument* document = context->CreateDocument();
	REQUIRE(document);

	auto get_declaration = [&]() -> const DecoratorDeclaration* {
		const Property* property = document->GetLocalProperty("decorator");
		if (!property || property->unit != Unit::DECORATOR)
			return nullptr;
		const DecoratorsPtr declarations = property->value.Get<DecoratorsPtr>();
		return declarations && declarations->list.size() == 1 ? &declarations->list[0] : nullptr;
	};
	auto find_color_stops = [](const DecoratorDeclaration& declaration) -> const ColorStopList* {
		for (const auto& pair : declaration.properties.GetProperties())
			if (pair.second.unit == Unit::COLORSTOPLIST)
				return &pair.second.value.GetReference<ColorStopList>();
		return nullptr;
	};
	auto resolve_stop = [&](const ColorStop& stop, float percentage_basis, float& result) {
		if (stop.position_calculation)
			return ResolveElementCalculation(*stop.position_calculation, *document, percentage_basis, result);
		if (stop.position.unit == Unit::PERCENT)
		{
			result = stop.position.number * .01f * percentage_basis;
			return true;
		}
		if (Any(stop.position.unit & Unit::LENGTH))
		{
			result = document->ResolveLength(stop.position);
			return true;
		}
		return false;
	};

	// Source: css/css-values/calc-background-image-gradient-1.html.
	CHECK(document->SetProperty("decorator", "radial-gradient(circle farthest-side at calc(50px + 50%) calc(100% - 30px), red, green)"));
	const DecoratorDeclaration* radial_position_declaration = get_declaration();
	REQUIRE(radial_position_declaration);
	Vector<std::pair<int, CalculationPtr>> radial_position_calculations;
	for (const auto& pair : radial_position_declaration->properties.GetProperties())
	{
		if (pair.second.unit == Unit::CALCULATION)
			radial_position_calculations.emplace_back(int(pair.first), pair.second.value.Get<CalculationPtr>());
	}
	std::sort(radial_position_calculations.begin(), radial_position_calculations.end(),
		[](const auto& lhs, const auto& rhs) { return lhs.first < rhs.first; });
	REQUIRE(radial_position_calculations.size() == 2);
	float radial_position_x = 0.f;
	float radial_position_y = 0.f;
	REQUIRE(ResolveElementCalculation(*radial_position_calculations[0].second, *document, 200.f, radial_position_x));
	REQUIRE(ResolveElementCalculation(*radial_position_calculations[1].second, *document, 50.f, radial_position_y));
	CHECK(radial_position_x == doctest::Approx(150.f));
	CHECK(radial_position_y == doctest::Approx(20.f));

	// Source: css/css-values/calc-background-linear-gradient-1.html. All five pinned gradient vectors are adapted
	// directly to RmlUi's decorator property while preserving every math stop expression and expected
	// 100px gradient-line position from the WPT reference rendering.
	struct LinearGradientCase {
		const char* value;
		Array<float, 5> expected_positions;
		int num_positions;
	};
	const LinearGradientCase linear_gradient_cases[] = {
		{"linear-gradient(lime 0px, lime calc(100% - 10px), blue calc(100% - 10px), blue 100%)", {0.f, 90.f, 90.f, 100.f}, 4},
		{"linear-gradient(blue calc(100% - 100px), green calc(10% + 20px), red 40px, white calc(100% - 40px), lime 80px)",
			{0.f, 30.f, 40.f, 60.f, 80.f}, 5},
		{"linear-gradient(blue calc(0px), purple calc(20%), red calc(10px + 10px + 20px), blue calc(30% + 30px), lime calc(180% - 100px))",
			{0.f, 20.f, 40.f, 60.f, 80.f}, 5},
		{"linear-gradient(blue calc(0% + 0px), green calc(10% + 20px), red 40px, blue calc(200% / 2 - 40px), yellow 80px)",
			{0.f, 30.f, 40.f, 60.f, 80.f}, 5},
		{"linear-gradient(red calc(100% - 100px), green calc(10% + 20px))", {0.f, 30.f}, 2},
	};
	for (const LinearGradientCase& test : linear_gradient_cases)
	{
		CAPTURE(String(test.value));
		REQUIRE(document->SetProperty("decorator", test.value));
		const DecoratorDeclaration* declaration = get_declaration();
		REQUIRE(declaration);
		const ColorStopList* stops = find_color_stops(*declaration);
		REQUIRE(stops);
		REQUIRE(int(stops->size()) == test.num_positions);
		for (int i = 0; i < test.num_positions; ++i)
		{
			float position = 0.f;
			REQUIRE(resolve_stop((*stops)[i], 100.f, position));
			CHECK(position == doctest::Approx(test.expected_positions[i]));
		}
	}

	// Source: css/css-values/calc-linear-radial-conic-gradient-001.html. grad/turn units are unsupported.
	REQUIRE(document->SetProperty("decorator", "linear-gradient(green calc(0%), blue)"));
	const DecoratorDeclaration* linear_stop_declaration = get_declaration();
	REQUIRE(linear_stop_declaration);
	const ColorStopList* linear_stops = find_color_stops(*linear_stop_declaration);
	REQUIRE(linear_stops);
	float linear_first_stop = -1.f;
	REQUIRE(resolve_stop((*linear_stops)[0], 100.f, linear_first_stop));
	CHECK(linear_first_stop == doctest::Approx(0.f));

	for (const char* value : {"linear-gradient(calc(90deg), green, blue)", "linear-gradient(calc(90deg), green calc(0%), blue)"})
	{
		REQUIRE(document->SetProperty("decorator", value));
		const DecoratorDeclaration* declaration = get_declaration();
		REQUIRE(declaration);
		CalculationPtr angle_calculation;
		for (const auto& pair : declaration->properties.GetProperties())
			if (pair.second.unit == Unit::CALCULATION)
				angle_calculation = pair.second.value.Get<CalculationPtr>();
		REQUIRE(bool(angle_calculation));
		float angle = 0.f;
		REQUIRE(ResolveElementCalculation(*angle_calculation, *document, 0.f, angle));
		CHECK(angle == doctest::Approx(Math::RMLUI_PI * .5f));
		if (String(value).find("calc(0%)") != String::npos)
		{
			const ColorStopList* stops = find_color_stops(*declaration);
			REQUIRE(stops);
			float first_stop = -1.f;
			REQUIRE(resolve_stop((*stops)[0], 100.f, first_stop));
			CHECK(first_stop == doctest::Approx(0.f));
		}
	}

	REQUIRE(document->SetProperty("decorator", "radial-gradient(green calc(10% + 20%), blue calc(30% + 40%))"));
	const DecoratorDeclaration* radial_stop_declaration = get_declaration();
	REQUIRE(radial_stop_declaration);
	const ColorStopList* radial_stops = find_color_stops(*radial_stop_declaration);
	REQUIRE(radial_stops);
	REQUIRE(radial_stops->size() == 2);
	for (int i = 0; i < 2; ++i)
	{
		float position = 0.f;
		REQUIRE(resolve_stop((*radial_stops)[i], 100.f, position));
		CHECK(position == doctest::Approx(i == 0 ? 30.f : 70.f));
	}
	// Keep the parsed stop calculation and change only its owning gradient geometry basis. This is the
	// Keep the parsed calculation alive for lazy compound resolution rather than reparsing the declaration.
	float resized_radial_stop = 0.f;
	REQUIRE(resolve_stop((*radial_stops)[0], 200.f, resized_radial_stop));
	CHECK(resized_radial_stop == doctest::Approx(60.f));

	REQUIRE(document->SetProperty("decorator", "conic-gradient(green calc(50% + 10%), blue calc(60% + 20%))"));
	const DecoratorDeclaration* conic_stop_declaration = get_declaration();
	REQUIRE(conic_stop_declaration);
	const ColorStopList* conic_stops = find_color_stops(*conic_stop_declaration);
	REQUIRE(conic_stops);
	REQUIRE(conic_stops->size() == 2);
	const float conic_basis = 2.f * Math::RMLUI_PI;
	for (int i = 0; i < 2; ++i)
	{
		float position = 0.f;
		REQUIRE(resolve_stop((*conic_stops)[i], conic_basis, position));
		CHECK(position / conic_basis == doctest::Approx(i == 0 ? .6f : .8f));
	}

	// Distinct percentage hints are type checked by the owning color-stop parser.
	TestsShell::SetNumExpectedWarnings(2);
	CHECK_FALSE(document->SetProperty("decorator", "linear-gradient(red calc(10deg + 5deg), blue)"));
	CHECK_FALSE(document->SetProperty("decorator", "conic-gradient(red calc(10px + 5px), blue)"));

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("Calculation.compound.media_wpt")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);
	context->SetDimensions(Vector2i(100, 10));
	context->SetDensityIndependentPixelRatio(2.f);

	// Sources: calc-in-media-queries-001/002.html and all twelve support documents driven by
	// calc-in-media-queries-with-mixed-units.html. The style parser must retain every exact math expression.
	const char* media_conditions[] = {
		"min-width: calc(100px)",
		"min-width: calc(-100px)",
		"width: calc(116px - 1em)",
		"width: calc(200vh + 5em)",
		"height: calc(100vw - 5.625em)",
		"width: calc(10vw + 900vh)",
		"width: calc(900vh + 10px)",
		"width: calc(90vw + 10px)",
		"width: calc(100px / 1em * 1em)",
		"width: calc((50vh * 5em) / 4px)",
		"height: calc(50vw / 0.3125em * 10px)",
		"width: calc(10vw / 10px * 1000vh)",
		"width: calc(8000vh * 1vw / 1em * 8px / 40vh)",
		"width: calc(100vw * 10vh * 1px * 0.0625em / 1px / 1px / 1px)",
	};
	// The WPT support documents use the browser's 16px initial font size, while RmlUi media queries
	// deliberately use DefaultComputedValues() (12px). Preserve every pinned expression and assert
	// its exact RmlUi match result under that existing media-query context instead of merely parsing it.
	constexpr bool expected_initial_matches[] = {
		true,  // calc-in-media-queries-001.html
		true,  // calc-in-media-queries-002.html
		false, // mixed-units-01.html: 116px - 1em = 104px
		false, // mixed-units-02.html: 200vh + 5em = 80px
		false, // mixed-units-03.html: 100vw - 5.625em = 32.5px
		true,  // mixed-units-04.html
		true,  // mixed-units-05.html
		true,  // mixed-units-06.html
		true,  // mixed-units-07.html
		false, // mixed-units-08.html: (50vh * 5em) / 4px = 75px
		false, // mixed-units-09.html: 50vw / 0.3125em * 10px = 133.333...px
		true,  // mixed-units-10.html
		false, // mixed-units-11.html: evaluates to 133.333...px
		false, // mixed-units-12.html: evaluates to 75px
	};
	static_assert(Count(expected_initial_matches) == Count(media_conditions));
	String styles = "div { width: 1px; height: 1px; }";
	for (int i = 0; i < int(Count(media_conditions)); ++i)
		styles += CreateString("@media (%s) { #m%d { width: %dpx; } }", media_conditions[i], i, i + 2);
	// Resolution is an existing RmlUi media feature not supplied by these three WPT sources.
	styles += "@media (min-resolution: calc(1x + 1x)) { #resolution { width: 77px; } }";
	styles += "@media (resolution: calc(1x / 41 * 82)) { #resolution-equality { width: 88px; } }";

	String body;
	for (int i = 0; i < int(Count(media_conditions)); ++i)
		body += CreateString("<div id='m%d'/>", i);
	body += "<div id='resolution'/><div id='resolution-equality'/>";
	const String rml = "<rml><head><style>" + styles + "</style></head><body>" + body + "</body></rml>";
	ElementDocument* document = context->LoadDocumentFromMemory(rml);
	REQUIRE(document);
	document->Show();
	TestsShell::RenderLoop();

	// Assert every WPT-owned condition, not just representative parsing. The first two source cases
	// match; the mixed-unit expectations above reflect RmlUi's explicitly preserved 12px media font.
	for (int i = 0; i < int(Count(media_conditions)); ++i)
	{
		CAPTURE(i);
		const float expected_width = expected_initial_matches[i] ? float(i + 2) : 1.f;
		CHECK(document->GetElementById(CreateString("m%d", i))->GetComputedValues().width().value == doctest::Approx(expected_width));
	}
	CHECK(document->GetElementById("resolution")->GetComputedValues().width().value == doctest::Approx(77.f));
	// Mathematically 1 / 41 * 82 == 2. The calculation path accumulates float roundoff, so exact
	// resolution equality must use the same calculated-value tolerance as width/height equality.
	CHECK(document->GetElementById("resolution-equality")->GetComputedValues().width().value == doctest::Approx(88.f));

	// Media calculations are re-evaluated against current viewport/dp state when the stylesheet is recompiled.
	context->SetDimensions(Vector2i(116, 10));
	context->SetDensityIndependentPixelRatio(1.f);
	TestsShell::RenderLoop();
	CHECK(document->GetElementById("m0")->GetComputedValues().width().value == doctest::Approx(2.f));
	CHECK(document->GetElementById("m5")->GetComputedValues().width().value == doctest::Approx(1.f));
	CHECK(document->GetElementById("resolution")->GetComputedValues().width().value == doctest::Approx(1.f));
	CHECK(document->GetElementById("resolution-equality")->GetComputedValues().width().value == doctest::Approx(1.f));

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("Calculation.compound.shadow_filter_and_lazy_lifecycle")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);
	context->SetDimensions(Vector2i(800, 400));
	context->SetDensityIndependentPixelRatio(1.f);
	const String rml =
		R"RML(<rml><body style="font-size: 16px"><div id="target" style="display: block; width: 200px; height: 100px; font-size: 20px"/></body></rml>)RML";
	ElementDocument* document = context->LoadDocumentFromMemory(rml);
	REQUIRE(document);
	document->Show();
	TestsShell::RenderLoop();
	Element* target = document->GetElementById("target");
	REQUIRE(target);
	Box initial_box;
	initial_box.SetContent(Vector2f(200.f, 100.f));
	target->SetBox(initial_box);

	CHECK(target->SetProperty("box-shadow", "#000 calc(1em + 1px) calc(1rem + 2px) calc(1vw + 3px) calc(1dp + 4px)"));
	auto resolve_box_shadow = [&]() {
		const BoxShadowGeometryInfo geometry =
			GeometryBoxShadow::Resolve(target, CornerSizes{}, ColourbPremultiplied{}, Array<ColourbPremultiplied, 4>{}, 1.f);
		REQUIRE(geometry.shadow_list.size() == 1);
		return geometry.shadow_list[0];
	};
	const BoxShadow initial_shadow = resolve_box_shadow();
	for (const char* property : {"filter", "backdrop-filter"})
	{
		CAPTURE(String(property));
		REQUIRE(target->SetProperty(property,
			"brightness(calc(50%)) contrast(calc(1 + 1)) hue-rotate(calc(45deg + 45deg)) blur(calc(1em + 1vw)) drop-shadow(#000 calc(1em) calc(1rem) "
			"calc(1vw))"));
		const Property* compound_filter_property = target->GetLocalProperty(property);
		REQUIRE(compound_filter_property);
		const FiltersPtr compound_filter_declarations = compound_filter_property->value.Get<FiltersPtr>();
		REQUIRE(bool(compound_filter_declarations));
		REQUIRE(compound_filter_declarations->list.size() == 5);
		for (const FilterDeclaration& declaration : compound_filter_declarations->list)
		{
			REQUIRE(declaration.instancer);
			CHECK(bool(declaration.instancer->InstanceFilter(declaration.type, declaration.properties)));
		}
		CHECK(target->SetProperty(property, "brightness(50%)"));
		CHECK(target->SetProperty(property, "brightness(calc(50%))"));
	}

	// Number-or-Percent keeps raw Percent until FilterBasic normalizes it; mixed Number + raw Percent is invalid.
	TestsShell::SetNumExpectedWarnings(2);
	CHECK_FALSE(target->SetProperty("filter", "brightness(calc(1 + 50%))"));
	CHECK_FALSE(target->SetProperty("backdrop-filter", "opacity(calc(1 + 50%))"));

	// Retain actual lazy filter instances across context changes. Their ExtendInkOverflow() owner path
	// must resolve the same parsed calculations against the current element instead of cached scalars.
	REQUIRE(target->SetProperty("filter", "blur(calc(1em + 1vw)) drop-shadow(#000 calc(1em) calc(1rem) calc(1vw))"));
	const Property* lazy_filter_property = target->GetLocalProperty("filter");
	REQUIRE(lazy_filter_property);
	const FiltersPtr lazy_filter_declarations = lazy_filter_property->value.Get<FiltersPtr>();
	REQUIRE(bool(lazy_filter_declarations));
	REQUIRE(lazy_filter_declarations->list.size() == 2);
	Vector<SharedPtr<Filter>> lazy_filters;
	for (const FilterDeclaration& declaration : lazy_filter_declarations->list)
	{
		REQUIRE(declaration.instancer);
		SharedPtr<Filter> filter = declaration.instancer->InstanceFilter(declaration.type, declaration.properties);
		REQUIRE(bool(filter));
		lazy_filters.push_back(std::move(filter));
	}
	auto filter_overflow = [&](int index) {
		Rectanglef overflow = Rectanglef::FromSize(Vector2f(100.f));
		lazy_filters[index]->ExtendInkOverflow(target, overflow);
		return overflow;
	};
	const Rectanglef initial_blur_overflow = filter_overflow(0);
	const Rectanglef initial_drop_shadow_overflow = filter_overflow(1);

	// A single translation exercises all lazy compound dependencies. ResolvePrimitive() is deliberately independent
	// of ElementStyle's direct-property dependency cache and must observe each current context/basis on every call.
	REQUIRE(target->SetProperty("transform", "translateX(calc(50% + 1em + 1rem + 1vw + 1vh + 1dp))"));
	const Property* transform_property = target->GetLocalProperty("transform");
	REQUIRE(transform_property);
	TransformPtr transform = transform_property->value.Get<TransformPtr>();
	REQUIRE(bool(transform));
	auto resolved_x = [&]() {
		const TransformPrimitive primitive = transform->ResolvePrimitive(0, *target);
		REQUIRE(primitive.type == TransformPrimitive::TRANSLATEX);
		return primitive.translate_x.values[0].number;
	};
	const float initial = resolved_x();

	REQUIRE(target->SetProperty("font-size", "30px"));
	TestsShell::RenderLoop();
	CHECK(resolved_x() != doctest::Approx(initial));
	CHECK(resolve_box_shadow().offset_x.number != doctest::Approx(initial_shadow.offset_x.number));
	CHECK(filter_overflow(0).Width() > initial_blur_overflow.Width());
	CHECK(filter_overflow(1).Width() > initial_drop_shadow_overflow.Width());
	const float after_font = resolved_x();
	const BoxShadow after_font_shadow = resolve_box_shadow();
	const Rectanglef after_font_drop_shadow_overflow = filter_overflow(1);
	REQUIRE(document->SetProperty("font-size", "24px"));
	TestsShell::RenderLoop();
	CHECK(resolved_x() != doctest::Approx(after_font));
	CHECK(resolve_box_shadow().offset_y.number != doctest::Approx(after_font_shadow.offset_y.number));
	CHECK(filter_overflow(1).Height() != doctest::Approx(after_font_drop_shadow_overflow.Height()));
	const float after_root = resolved_x();
	const BoxShadow after_root_shadow = resolve_box_shadow();
	const Rectanglef after_root_blur_overflow = filter_overflow(0);
	const Rectanglef after_root_drop_shadow_overflow = filter_overflow(1);
	context->SetDimensions(Vector2i(1000, 500));
	TestsShell::RenderLoop();
	CHECK(resolved_x() != doctest::Approx(after_root));
	CHECK(resolve_box_shadow().blur_radius.number != doctest::Approx(after_root_shadow.blur_radius.number));
	CHECK(filter_overflow(0).Width() != doctest::Approx(after_root_blur_overflow.Width()));
	CHECK(filter_overflow(1).Width() != doctest::Approx(after_root_drop_shadow_overflow.Width()));
	const float after_viewport = resolved_x();
	const BoxShadow after_viewport_shadow = resolve_box_shadow();
	context->SetDensityIndependentPixelRatio(2.f);
	TestsShell::RenderLoop();
	CHECK(resolved_x() != doctest::Approx(after_viewport));
	CHECK(resolve_box_shadow().spread_distance.number != doctest::Approx(after_viewport_shadow.spread_distance.number));
	const float after_dp = resolved_x();
	const float geometry_before = target->GetBox().GetSize(BoxArea::Border).x;
	Box enlarged_box;
	enlarged_box.SetContent(Vector2f(400.f, 100.f));
	target->SetBox(enlarged_box);
	const float geometry_after = target->GetBox().GetSize(BoxArea::Border).x;
	CHECK(geometry_after > geometry_before);
	CHECK(resolved_x() > after_dp + 0.4f * (geometry_after - geometry_before));
	CHECK(target->GetLocalProperty("transform")->value.Get<TransformPtr>().get() == transform.get());

	// The sidecar follows normal Transform value semantics. A copied transform retains its calculation
	// ownership after the parsed property and its original Transform owner are released.
	const float before_copy_release = resolved_x();
	Transform copied_transform = *transform;
	REQUIRE(target->SetProperty("transform", "none"));
	transform.reset();
	const TransformPrimitive copied_primitive = copied_transform.ResolvePrimitive(0, *target);
	REQUIRE(copied_primitive.type == TransformPrimitive::TRANSLATEX);
	CHECK(copied_primitive.translate_x.values[0].number == doctest::Approx(before_copy_release));

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("Calculation.compound.rmlui_extension_rejection")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);
	ElementDocument* document = context->LoadDocumentFromMemory("<rml><head><style>body { display: block; }</style></head><body/></rml>");
	REQUIRE(document);

	// Font effects are eagerly instanced and must reject calculations before scalar extraction.
	TestsShell::SetNumExpectedWarnings(8);
	CHECK_FALSE(document->SetProperty("font-effect", "blur(calc(1px + 1px) #000)"));
	CHECK_FALSE(document->SetProperty("font-effect", "outline(calc(1px + 1px) #000)"));
	CHECK_FALSE(document->SetProperty("font-effect", "shadow(calc(1px) calc(2px) #000)"));
	CHECK_FALSE(document->SetProperty("font-effect", "glow(calc(1px) calc(2px) calc(3px) calc(4px) #000)"));

	// Decorators are declaration-deferred. Drive the actual instancer lifecycle and require their private numeric
	// slots to reject before any NumericValue extraction.
	for (const char* value : {
			 "text(\"x\" #fff calc(1px + 1px) center)",
			 "image(/assets/invader.tga fill calc(1px + 1px) center)",
			 "radial-gradient(circle calc(50%), red, blue)",
		 })
	{
		CAPTURE(String(value));
		CHECK(document->SetProperty("decorator", value));
		const Property* property = document->GetLocalProperty("decorator");
		REQUIRE(property);
		const DecoratorsPtr declarations = property->value.Get<DecoratorsPtr>();
		REQUIRE(bool(declarations));
		REQUIRE(document->GetStyleSheet());
		REQUIRE(document->GetRenderManager());
		TestsShell::SetNumExpectedWarnings(1);
		const DecoratorPtrList& decorators = document->GetStyleSheet()->InstanceDecorators(*document->GetRenderManager(), *declarations, nullptr);
		CHECK(decorators.empty());
	}
	TestsShell::SetNumExpectedWarnings(1);
	CHECK_FALSE(document->SetProperty("decorator", "ninepatch(outer, inner, calc(1px + 1px))"));

	// The private ElementHandle edge-margin parser rejects calculation values instead of exposing them as NumericValue.
	ElementPtr handle = Factory::InstanceElement(document, "handle", "handle", XMLAttributes{});
	REQUIRE(handle);
	document->AppendChild(std::move(handle));
	Element* handle_element = document->GetLastChild();
	REQUIRE(handle_element);
	handle_element->SetAttribute("edge_margin", "calc(1px + 1px)");
	TestsShell::SetNumExpectedWarnings(1);
	handle_element->DispatchEvent("click", Dictionary{});

	// Stylesheet-only extensions must reject calculation values while parsing their private numeric descriptors.
	const char extension_styles[] = R"RCSS(
@spritesheet private_math {
	src: /assets/high_scores_alien_3.tga;
	bad-rect: calc(1px + 1px) 0px 10px 10px;
	resolution: calc(1x + 1x);
}
@font-face {
	font-family: LatoLatin;
	src: /assets/LatoLatin-Regular.ttf;
	font-weight: calc(300 + 100);
	-rmlui-face-index: calc(0 + 1);
}
)RCSS";
	StyleSheetContainer extension_container;
	StreamMemory extension_stream{reinterpret_cast<const byte*>(extension_styles), sizeof(extension_styles) - 1};
	TestsShell::SetNumExpectedWarnings(5);
	CHECK(extension_container.LoadStyleSheetContainer(&extension_stream, 1));

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("Calculation.animation.wpt_interpolation")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);
	context->SetDimensions(Vector2i(800, 600));
	const String rml =
		R"RML(<rml><body><div id="container" style="position: relative; width: 50px; height: 50px"><div id="target" style="position: absolute"/></div><div id="square-container" style="width: 200px; height: 200px"><div id="square"/></div></body></rml>)RML";
	ElementDocument* document = context->LoadDocumentFromMemory(rml);
	REQUIRE(document);
	document->Show();
	TestsShell::RenderLoop();
	Element* container = document->GetElementById("container");
	REQUIRE(container);
	Box container_box;
	container_box.SetContent(Vector2f(50.f, 50.f));
	container->SetBox(container_box);
	Element* square_container = document->GetElementById("square-container");
	REQUIRE(square_container);
	Box square_container_box;
	square_container_box.SetContent(Vector2f(200.f, 200.f));
	square_container->SetBox(square_container_box);

	Element* target = document->GetElementById("target");
	REQUIRE(target);
	target->SetOffset(Vector2f(0.f), container);
	REQUIRE(target->SetProperty("left", "calc(50% - 25px)"));
	const Property left_from = *target->GetLocalProperty("left");
	REQUIRE(target->SetProperty("left", "calc(100% - 10px)"));
	const Property left_to = *target->GetLocalProperty("left");
	// Source: animations/calc-interpolation.html. RmlUi clamps interpolation to the key interval, so
	// RmlUi's animation model clamps to the key interval, so exercise every in-range WPT sample
	// (0, .25, .5, .75, 1) and omit the source's extrapolation probes.
	const CalculationPtr left_from_calculation = left_from.value.Get<CalculationPtr>();
	REQUIRE(bool(left_from_calculation));
	float left_start = -1.f;
	REQUIRE(ResolveElementCalculation(*left_from_calculation, *target, 50.f, left_start, RelativeTarget::ContainingBlockWidth));
	CHECK(left_start == doctest::Approx(0.f));

	ElementAnimation left_animation(PropertyId::Left, ElementAnimationOrigin::User, left_from, *target, 0.0, 0.4f, 1, false);
	REQUIRE(left_animation.AddKey(0.4f, left_to, *target, Tween{}, false));
	constexpr float sample_times[] = {0.1f, 0.2f, 0.3f, 0.4f};
	constexpr float expected_values[] = {10.f, 20.f, 30.f, 40.f};
	for (size_t i = 0; i < Count(sample_times); ++i)
	{
		const Property sample = left_animation.UpdateAndGetProperty(sample_times[i], *target);
		REQUIRE(sample.unit == Unit::PX);
		CHECK(sample.Get<float>() == doctest::Approx(expected_values[i]));
	}

	Element* square = document->GetElementById("square");
	REQUIRE(square);
	square->SetOffset(Vector2f(0.f), square_container);
	auto midpoint = [&](const char* property_name, PropertyId property_id, const char* from, const char* to) {
		REQUIRE(square->SetProperty(property_name, from));
		const Property p0 = *square->GetLocalProperty(property_name);
		REQUIRE(square->SetProperty(property_name, to));
		const Property p1 = *square->GetLocalProperty(property_name);
		ElementAnimation animation(property_id, ElementAnimationOrigin::User, p0, *square, 0.0, 0.2f, 1, false);
		REQUIRE(animation.AddKey(0.2f, p1, *square, Tween{}, false));
		return animation.UpdateAndGetProperty(0.1, *square);
	};

	const Property width_midpoint = midpoint("width", PropertyId::Width, "min(50px, 30%)", "max(75%, 100px)");
	CHECK(width_midpoint.unit == Unit::PX);
	CHECK(width_midpoint.Get<float>() == doctest::Approx(100.f));
	const Property height_midpoint = midpoint("height", PropertyId::Height, "min(75%, 160px)", "max(50px, 20%)");
	CHECK(height_midpoint.unit == Unit::PX);
	CHECK(height_midpoint.Get<float>() == doctest::Approx(100.f));

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("Calculation.animation.rmlui_interpolation_lifecycle")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);
	context->SetDimensions(Vector2i(800, 600));
	const String rml = R"RML(<rml><body><div id="container"><div id="target"/></div></body></rml>)RML";
	ElementDocument* document = context->LoadDocumentFromMemory(rml);
	REQUIRE(document);
	document->Show();
	TestsShell::RenderLoop();
	Element* container = document->GetElementById("container");
	Element* target = document->GetElementById("target");
	REQUIRE(container);
	REQUIRE(target);
	Box container_box;
	container_box.SetContent(Vector2f(200.f, 100.f));
	container->SetBox(container_box);
	target->SetOffset(Vector2f(0.f), container);

	// RmlUi-specific lifecycle gap: fixed-to-calculated interpolation must reevaluate the calculated
	// endpoint against the current containing block instead of freezing the basis at key insertion.
	REQUIRE(target->SetProperty("left", "0px"));
	const Property left_from = *target->GetLocalProperty("left");
	REQUIRE(target->SetProperty("left", "calc(100% - 20px)"));
	const Property left_to = *target->GetLocalProperty("left");
	ElementAnimation left_animation(PropertyId::Left, ElementAnimationOrigin::User, left_from, *target, 0.0, 0.2f, 1, false);
	REQUIRE(left_animation.AddKey(0.2f, left_to, *target, Tween{}, false));
	CHECK(left_animation.UpdateAndGetProperty(0.05, *target).Get<float>() == doctest::Approx(45.f));
	container_box.SetContent(Vector2f(400.f, 100.f));
	container->SetBox(container_box);
	const Property basis_changed_midpoint = left_animation.UpdateAndGetProperty(0.1, *target);
	CHECK(basis_changed_midpoint.unit == Unit::PX);
	CHECK(basis_changed_midpoint.Get<float>() == doctest::Approx(190.f));

	// Number endpoints use the same scalar interpolation path as ordinary numeric properties.
	REQUIRE(target->SetProperty("opacity", "calc(0.25 + 0.25)"));
	const Property opacity_from = *target->GetLocalProperty("opacity");
	REQUIRE(target->SetProperty("opacity", "calc(1)"));
	const Property opacity_to = *target->GetLocalProperty("opacity");
	ElementAnimation opacity_animation(PropertyId::Opacity, ElementAnimationOrigin::User, opacity_from, *target, 0.0, 0.2f, 1, false);
	REQUIRE(opacity_animation.AddKey(0.2f, opacity_to, *target, Tween{}, false));
	const Property opacity_midpoint = opacity_animation.UpdateAndGetProperty(0.1, *target);
	CHECK(opacity_midpoint.unit == Unit::NUMBER);
	CHECK(opacity_midpoint.Get<float>() == doctest::Approx(0.75f));

	// Final interpolation audit regression: ordinary different-unit lengths must resolve both endpoints.
	REQUIRE(target->SetProperty("font-size", "20px"));
	TestsShell::RenderLoop();
	REQUIRE(target->SetProperty("left", "1em"));
	const Property ordinary_length_from = *target->GetLocalProperty("left");
	REQUIRE(target->SetProperty("left", "40px"));
	const Property ordinary_length_to = *target->GetLocalProperty("left");
	ElementAnimation ordinary_length_animation(PropertyId::Left, ElementAnimationOrigin::User, ordinary_length_from, *target, 0.0, 0.2f, 1, false);
	REQUIRE(ordinary_length_animation.AddKey(0.2f, ordinary_length_to, *target, Tween{}, false));
	const Property ordinary_length_midpoint = ordinary_length_animation.UpdateAndGetProperty(0.1, *target);
	CHECK(ordinary_length_midpoint.unit == Unit::PX);
	CHECK(ordinary_length_midpoint.Get<float>() == doctest::Approx(30.f));

	// Transform supplies the angle case and compound geometry case. Calculated arguments must be
	// resolved from each endpoint at update time and interpolated as ordinary canonical primitives.
	Box target_box;
	target_box.SetContent(Vector2f(200.f, 100.f));
	target->SetBox(target_box);
	REQUIRE(target->SetProperty("transform", "translateX(0px) rotate(0deg) scaleX(1)"));
	const Property transform_from = *target->GetLocalProperty("transform");
	REQUIRE(target->SetProperty("transform", "translateX(calc(100% - 20px)) rotate(calc(90deg + 90deg)) scaleX(calc(1 + 2))"));
	const Property transform_to = *target->GetLocalProperty("transform");
	ElementAnimation transform_animation(PropertyId::Transform, ElementAnimationOrigin::User, transform_from, *target, 0.0, 0.2f, 1, false);
	REQUIRE(transform_animation.AddKey(0.2f, transform_to, *target, Tween{}, false));
	const Property transform_quarter = transform_animation.UpdateAndGetProperty(0.05, *target);
	const TransformPtr transform_quarter_value = transform_quarter.value.Get<TransformPtr>();
	REQUIRE(bool(transform_quarter_value));
	CHECK(transform_quarter_value->ResolvePrimitive(0, *target).translate_x.values[0].number == doctest::Approx(45.f));
	target_box.SetContent(Vector2f(400.f, 100.f));
	target->SetBox(target_box);
	const Property transform_midpoint = transform_animation.UpdateAndGetProperty(0.1, *target);
	REQUIRE(transform_midpoint.unit == Unit::TRANSFORM);
	const TransformPtr transform = transform_midpoint.value.Get<TransformPtr>();
	REQUIRE(bool(transform));
	REQUIRE(transform->GetNumPrimitives() == 3);
	const TransformPrimitive translate = transform->ResolvePrimitive(0, *target);
	const TransformPrimitive rotate = transform->ResolvePrimitive(1, *target);
	const TransformPrimitive scale = transform->ResolvePrimitive(2, *target);
	CHECK(translate.translate_x.values[0].unit == Unit::PX);
	CHECK(translate.translate_x.values[0].number == doctest::Approx(190.f));
	CHECK(rotate.rotate_2d.values[0] == doctest::Approx(Math::RMLUI_PI * 0.5f));
	CHECK(scale.scale_x.values[0] == doctest::Approx(2.f));

	// Gradient geometry owns the percentage bases for positions, radii, and color stops. Keep those
	// expressions deferred through decorator interpolation so the instancer can resolve the current geometry.
	REQUIRE(target->SetProperty("decorator", "radial-gradient(circle at calc(50% + 10px) center, red calc(10%), blue)"));
	const Property decorator_from = *target->GetLocalProperty("decorator");
	REQUIRE(target->SetProperty("decorator", "radial-gradient(circle at calc(100% - 20px) center, red calc(30%), blue)"));
	const Property decorator_to = *target->GetLocalProperty("decorator");
	ElementAnimation decorator_animation(PropertyId::Decorator, ElementAnimationOrigin::User, decorator_from, *target, 0.0, 0.2f, 1, false);
	REQUIRE(decorator_animation.AddKey(0.2f, decorator_to, *target, Tween{}, false));
	const Property decorator_midpoint = decorator_animation.UpdateAndGetProperty(0.1, *target);
	REQUIRE(decorator_midpoint.unit == Unit::DECORATOR);
	const DecoratorsPtr midpoint_decorators = decorator_midpoint.value.Get<DecoratorsPtr>();
	REQUIRE(bool(midpoint_decorators));
	REQUIRE(midpoint_decorators->list.size() == 1);
	const PropertyDictionary& midpoint_properties = midpoint_decorators->list[0].properties;

	CalculationPtr midpoint_position;
	const ColorStopList* midpoint_stops = nullptr;
	for (const auto& pair : midpoint_properties.GetProperties())
	{
		if (pair.second.unit == Unit::CALCULATION)
			midpoint_position = pair.second.value.Get<CalculationPtr>();
		else if (pair.second.unit == Unit::COLORSTOPLIST)
			midpoint_stops = &pair.second.value.GetReference<ColorStopList>();
	}
	REQUIRE(bool(midpoint_position));
	float resolved_midpoint_position = 0.f;
	REQUIRE(ResolveElementCalculation(*midpoint_position, *target, 200.f, resolved_midpoint_position));
	CHECK(resolved_midpoint_position == doctest::Approx(145.f));
	REQUIRE(midpoint_stops);
	REQUIRE(midpoint_stops->size() == 2);
	REQUIRE(bool((*midpoint_stops)[0].position_calculation));
	float resolved_midpoint_stop = 0.f;
	REQUIRE(ResolveElementCalculation(*(*midpoint_stops)[0].position_calculation, *target, 100.f, resolved_midpoint_stop));
	CHECK(resolved_midpoint_stop == doctest::Approx(20.f));

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("Calculation.animation.calculated_time_drives_animation")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);
	const String rml = R"RML(<rml><body><div id="target"/></body></rml>)RML";
	ElementDocument* document = context->LoadDocumentFromMemory(rml);
	REQUIRE(document);
	document->Show();
	Element* target = document->GetElementById("target");
	REQUIRE(target);

	REQUIRE(target->SetProperty("animation", "fade calc(100ms + 100ms) linear"));
	const Property* animation_property = target->GetLocalProperty("animation");
	REQUIRE(animation_property);
	const AnimationList animation_list = animation_property->value.Get<AnimationList>();
	REQUIRE(animation_list.size() == 1);
	CHECK(animation_list[0].duration == doctest::Approx(0.2f));

	REQUIRE(target->SetProperty("opacity", "0"));
	const Property opacity_from = *target->GetLocalProperty("opacity");
	REQUIRE(target->SetProperty("opacity", "1"));
	const Property opacity_to = *target->GetLocalProperty("opacity");
	ElementAnimation animation(PropertyId::Opacity, ElementAnimationOrigin::Animation, opacity_from, *target, 0.0, animation_list[0].duration, 1,
		false);
	REQUIRE(animation.AddKey(animation_list[0].duration, opacity_to, *target, animation_list[0].tween, false));
	const Property midpoint = animation.UpdateAndGetProperty(0.1, *target);
	CHECK(midpoint.unit == Unit::NUMBER);
	CHECK(midpoint.Get<float>() == doctest::Approx(0.5f));

	document->Close();
	TestsShell::ShutdownShell();
}

#endif
