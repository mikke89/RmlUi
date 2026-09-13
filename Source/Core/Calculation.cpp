#include "Calculation.h"
#include "../../Include/RmlUi/Core/StringUtilities.h"
#include "../../Include/RmlUi/Core/TypeConverter.h"
#include <cmath>

namespace Rml {
namespace {

	bool EqualNodes(const Calculation::Node& lhs, const Calculation::Node& rhs)
	{
		if (lhs.kind != rhs.kind || lhs.value != rhs.value || lhs.unit != rhs.unit || lhs.value_unit != rhs.value_unit || lhs.type != rhs.type ||
			lhs.children.size() != rhs.children.size())
			return false;
		for (size_t i = 0; i < lhs.children.size(); ++i)
		{
			if (bool(lhs.children[i]) != bool(rhs.children[i]))
				return false;
			if (lhs.children[i] && !EqualNodes(*lhs.children[i], *rhs.children[i]))
				return false;
		}
		return true;
	}

	int Precedence(Calculation::Kind kind)
	{
		switch (kind)
		{
		case Calculation::Kind::Sum: return 1;
		case Calculation::Kind::Product: return 2;
		case Calculation::Kind::Negate:
		case Calculation::Kind::Invert: return 3;
		default: return 4;
		}
	}

	String ValueToString(const Calculation::Node& node)
	{
		if (node.value_unit == Calculation::ValueUnit::Seconds)
			return CreateString("%.9g", node.value) + "s";
		return CreateString("%.9g", node.value) + Rml::ToString(node.unit);
	}

	String NodeToString(const Calculation::Node& node, int parent_precedence = 0)
	{
		if (node.kind == Calculation::Kind::Value)
			return ValueToString(node);

		if (node.kind == Calculation::Kind::Min || node.kind == Calculation::Kind::Max || node.kind == Calculation::Kind::Clamp)
		{
			const char* name = node.kind == Calculation::Kind::Min ? "min" : node.kind == Calculation::Kind::Max ? "max" : "clamp";
			String result = String(name) + "(";
			for (size_t i = 0; i < node.children.size(); ++i)
			{
				if (i)
					result += ", ";
				result += node.children[i] ? NodeToString(*node.children[i]) : "none";
			}
			return result + ')';
		}

		if (node.kind == Calculation::Kind::Negate)
		{
			RMLUI_ASSERT(node.children.size() == 1 && node.children[0]);
			return String("-") + NodeToString(*node.children[0], Precedence(node.kind));
		}

		if (node.kind == Calculation::Kind::Invert)
		{
			RMLUI_ASSERT(node.children.size() == 1 && node.children[0]);
			return String("1 / ") + NodeToString(*node.children[0], Precedence(node.kind));
		}

		const int precedence = Precedence(node.kind);
		String result;
		for (size_t i = 0; i < node.children.size(); ++i)
		{
			const auto& child = node.children[i];
			RMLUI_ASSERT(child);
			if (i)
			{
				if (node.kind == Calculation::Kind::Sum && child->kind == Calculation::Kind::Negate)
				{
					result += " - ";
					result += NodeToString(*child->children[0], precedence + 1);
					continue;
				}
				if (node.kind == Calculation::Kind::Product && child->kind == Calculation::Kind::Invert)
				{
					result += " / ";
					result += NodeToString(*child->children[0], precedence + 1);
					continue;
				}
				result += node.kind == Calculation::Kind::Sum ? " + " : " * ";
			}
			result += NodeToString(*child, precedence);
		}
		if (precedence < parent_precedence)
			return String("(") + result + ')';
		return result;
	}

	CalculationNumericType TypeForUnit(Unit unit)
	{
		CalculationNumericType type;
		if (Any(unit & Unit::LENGTH))
			type.exponents[size_t(CalculationDimension::Length)] = 1;
		else if (Any(unit & Unit::ANGLE))
			type.exponents[size_t(CalculationDimension::Angle)] = 1;
		else if (unit == Unit::X)
			type.exponents[size_t(CalculationDimension::Resolution)] = 1;
		else if (unit == Unit::PERCENT)
			type.exponents[size_t(CalculationDimension::Percent)] = 1;
		return type;
	}

} // namespace

bool Calculation::operator==(const Calculation& other) const
{
	if (root == other.root)
		return true;
	if (!root || !other.root)
		return false;
	return EqualNodes(*root, *other.root);
}

String Calculation::ToString() const
{
	return root ? String("calc(") + NodeToString(*root) + ')' : String();
}

CalculationPtr Calculation::MakeValue(float value, Unit unit)
{
	auto node = MakeShared<Node>();
	node->value = value;
	node->unit = unit;
	node->type = TypeForUnit(unit);
	return MakeShared<Calculation>(std::move(node));
}

CalculationPtr Calculation::MakeOperation(Kind kind, Vector<CalculationPtr> operands)
{
	auto node = MakeShared<Node>();
	node->kind = kind;
	for (const CalculationPtr& operand : operands)
		node->children.push_back(operand ? operand->GetRoot() : nullptr);
	return MakeShared<Calculation>(std::move(node));
}

} // namespace Rml
