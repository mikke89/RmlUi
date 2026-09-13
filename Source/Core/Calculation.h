#pragma once

#include "../../Include/RmlUi/Core/Types.h"
#include "../../Include/RmlUi/Core/Unit.h"
#include <array>

namespace Rml {

enum class CalculationDimension : uint8_t { Length, Angle, Time, Resolution, Percent, Count };

struct CalculationNumericType {
	std::array<int8_t, size_t(CalculationDimension::Count)> exponents = {};

	bool operator==(const CalculationNumericType& other) const { return exponents == other.exponents; }
	bool operator!=(const CalculationNumericType& other) const { return !(*this == other); }
};

// Internal immutable representation. Nodes are only exposed through shared pointers to const data;
// parser construction is the only production path which mutates a node before publication.
class Calculation {
public:
	enum class Kind : uint8_t { Value, Sum, Product, Negate, Invert, Min, Max, Clamp };
	enum class ResidualForm : uint8_t { Tree, Constant, LinearLengthPercentage };
	enum class ValueUnit : uint8_t { Public, Seconds };

	struct Node {
		Kind kind = Kind::Value;
		float value = 0;
		Unit unit = Unit::NUMBER;
		ValueUnit value_unit = ValueUnit::Public;
		CalculationNumericType type;
		Vector<SharedPtr<const Node>> children;
	};

	explicit Calculation(SharedPtr<const Node> root, Units dependency_mask = Unit::UNKNOWN, ResidualForm residual_form = ResidualForm::Tree,
		float linear_px = 0.f, float linear_percent = 0.f) :
		root(std::move(root)), dependency_mask(dependency_mask), residual_form(residual_form), linear_px(linear_px), linear_percent(linear_percent)
	{}

	const SharedPtr<const Node>& GetRoot() const { return root; }
	const CalculationNumericType& GetType() const { return root->type; }
	Units GetDependencyMask() const { return dependency_mask; }
	ResidualForm GetResidualForm() const { return residual_form; }
	float GetLinearPx() const { return linear_px; }
	float GetLinearPercent() const { return linear_percent; }
	bool operator==(const Calculation& other) const;
	bool operator!=(const Calculation& other) const { return !(*this == other); }
	String ToString() const;

	static CalculationPtr MakeValue(float value, Unit unit);
	static CalculationPtr MakeOperation(Kind kind, Vector<CalculationPtr> operands);

private:
	SharedPtr<const Node> root;
	Units dependency_mask = Unit::UNKNOWN;
	ResidualForm residual_form = ResidualForm::Tree;
	float linear_px = 0.f;
	float linear_percent = 0.f;
};

} // namespace Rml
