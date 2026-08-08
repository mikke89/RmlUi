#include <RmlUi/Core/ComputedValues.h>
#include <RmlUi/Core/NumericValue.h>
#include <RmlUi/Core/Property.h>
#include <RmlUi/Core/StyleTypes.h>
#include <RmlUi/Core/Variant.h>
#include <doctest.h>

#ifdef RMLUI_MATH_EXPRESSIONS
	#include "../../../Source/Core/Calculation.h"
#endif

using namespace Rml;

#ifdef RMLUI_MATH_EXPRESSIONS

TEST_CASE("Calculation.types.numeric_value_boundary")
{
	CHECK(sizeof(NumericValue::number) == sizeof(float));
	CHECK(sizeof(NumericValue::unit) == sizeof(Unit));
	CHECK(!Any(Unit::CALCULATION & Unit::NUMERIC));
	CHECK(Variant(CalculationPtr()).GetType() == Variant::CALCULATIONPTR);
}

TEST_CASE("Calculation.types.returned_value_owns_residual")
{
	CalculationPtr calculation =
		Calculation::MakeOperation(Calculation::Kind::Sum, {Calculation::MakeValue(50.f, Unit::PERCENT), Calculation::MakeValue(-20.f, Unit::PX)});
	Style::LengthPercentageAuto returned_auto;
	Style::LengthPercentage returned_plain;
	{
		ComputedValues values(nullptr);
		values.width(Style::LengthPercentageAuto(calculation));
		values.padding_left(Style::LengthPercentage(calculation));
		returned_auto = values.width();
		returned_plain = values.padding_left();
		CHECK(returned_auto.type == Style::LengthPercentageAuto::Calculation);
		CHECK(returned_plain.type == Style::LengthPercentage::Calculation);
	}
	calculation.reset();
	REQUIRE(bool(returned_auto.calculation));
	REQUIRE(bool(returned_plain.calculation));
	CHECK(*returned_auto.calculation == *returned_plain.calculation);
	CHECK(returned_auto.calculation->ToString() == "calc(50% + -20px)");
}

TEST_CASE("Calculation.types.variant_and_property_structural_equality")
{
	CalculationPtr first =
		Calculation::MakeOperation(Calculation::Kind::Sum, {Calculation::MakeValue(10.f, Unit::PX), Calculation::MakeValue(25.f, Unit::PERCENT)});
	CalculationPtr equivalent =
		Calculation::MakeOperation(Calculation::Kind::Sum, {Calculation::MakeValue(10.f, Unit::PX), Calculation::MakeValue(25.f, Unit::PERCENT)});
	CalculationPtr different =
		Calculation::MakeOperation(Calculation::Kind::Sum, {Calculation::MakeValue(11.f, Unit::PX), Calculation::MakeValue(25.f, Unit::PERCENT)});
	REQUIRE(bool(first != equivalent));

	Variant a(first), b(equivalent), c(different), null_a{CalculationPtr()}, null_b{CalculationPtr()};
	CHECK(a == b);
	CHECK(a != c);
	CHECK(null_a == null_b);
	CHECK(a != null_a);
	CHECK(bool(a.Get<CalculationPtr>() == first));

	Variant copied = a;
	Variant moved = std::move(copied);
	CHECK(moved == a);
	moved.Clear();
	CHECK(moved.GetType() == Variant::NONE);

	Property p1(first, Unit::CALCULATION);
	Property p2(equivalent, Unit::CALCULATION);
	Property p3(different, Unit::CALCULATION);
	CHECK(p1 == p2);
	CHECK(p1 != p3);
}

TEST_CASE("Calculation.types.computed_value_storage_lifecycle")
{
	ComputedValues values(nullptr);
	CHECK(values.width().type == Style::LengthPercentageAuto::Auto);
	CHECK(values.padding_left().type == Style::LengthPercentage::Length);

	CalculationPtr width = Calculation::MakeValue(25.f, Unit::PERCENT);
	CalculationPtr padding = Calculation::MakeValue(3.f, Unit::PX);
	values.width(Style::LengthPercentageAuto(width));
	CHECK(bool(values.width().calculation == width));
	values.padding_left(Style::LengthPercentage(padding));

	ComputedValues copy(nullptr);
	copy.CopyNonInherited(values);
	CHECK(bool(copy.width().calculation == width));
	CHECK(bool(copy.padding_left().calculation == padding));

	values.width(Style::LengthPercentageAuto(Style::LengthPercentageAuto::Length, 42.f));
	CHECK(values.width().type == Style::LengthPercentageAuto::Length);
	CHECK(values.width().value == doctest::Approx(42.f));
	CHECK(!values.width().calculation);
	values.padding_left(Style::LengthPercentage(Style::LengthPercentage::Percentage, 10.f));
	CHECK(values.padding_left().type == Style::LengthPercentage::Percentage);
	CHECK(!values.padding_left().calculation);

	copy.CopyNonInherited(values);
	CHECK(copy.width().type == Style::LengthPercentageAuto::Length);
	CHECK(!copy.width().calculation);
	CHECK(copy.padding_left().type == Style::LengthPercentage::Percentage);
	CHECK(!copy.padding_left().calculation);
}

TEST_CASE("Calculation.types.scalar_paths_reject_calculation")
{
	Property ordinary_length(12.f, Unit::PX);
	const NumericValue numeric_value = ordinary_length.GetNumericValue();
	CHECK(numeric_value.number == doctest::Approx(12.f));
	CHECK(numeric_value.unit == Unit::PX);

	CalculationPtr calculation = Calculation::MakeValue(12.f, Unit::PX);
	Property calculated(calculation, Unit::CALCULATION);
	CHECK(calculated.value.GetType() == Variant::CALCULATIONPTR);
	CHECK(bool(calculated.Get<CalculationPtr>() == calculation));
	CHECK(calculated.GetNumericValue() == NumericValue());
	CHECK(!Any(Unit::CALCULATION & Unit::NUMERIC));
}

#endif
