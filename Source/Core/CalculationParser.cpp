#include "CalculationParser.h"
#include "../../Include/RmlUi/Core/StringUtilities.h"
#include <cmath>
#include <cstdlib>
#include <limits>

namespace Rml {
namespace {

	constexpr size_t MaxTerms = 32;
	constexpr size_t MaxDepth = 32;
	constexpr size_t MaxArguments = 32;

	using NodePtr = SharedPtr<const Calculation::Node>;

	CalculationNumericType TypeForUnit(Unit unit, CalculationPercentageHint hint)
	{
		CalculationNumericType type;
		if (Any(unit & Unit::LENGTH))
			type.exponents[size_t(CalculationDimension::Length)] = 1;
		else if (Any(unit & Unit::ANGLE))
			type.exponents[size_t(CalculationDimension::Angle)] = 1;
		else if (unit == Unit::X)
			type.exponents[size_t(CalculationDimension::Resolution)] = 1;
		else if (unit == Unit::PERCENT)
		{
			if (hint == CalculationPercentageHint::Length)
				type.exponents[size_t(CalculationDimension::Length)] = 1;
			else if (hint == CalculationPercentageHint::Angle)
				type.exponents[size_t(CalculationDimension::Angle)] = 1;
			else
				type.exponents[size_t(CalculationDimension::Percent)] = 1;
		}
		return type;
	}

	CalculationNumericType TimeType()
	{
		CalculationNumericType type;
		type.exponents[size_t(CalculationDimension::Time)] = 1;
		return type;
	}

	CalculationNumericType MultiplyTypes(CalculationNumericType lhs, const CalculationNumericType& rhs, int sign = 1)
	{
		for (size_t i = 0; i < lhs.exponents.size(); ++i)
			lhs.exponents[i] = int8_t(lhs.exponents[i] + sign * rhs.exponents[i]);
		return lhs;
	}

	CalculationTypeMask FinalTypeMask(const CalculationNumericType& type)
	{
		int dimension = -1;
		for (size_t i = 0; i < type.exponents.size(); ++i)
		{
			const int8_t exponent = type.exponents[i];
			if (exponent == 0)
				continue;
			if (exponent != 1 || dimension >= 0)
				return 0;
			dimension = int(i);
		}
		if (dimension < 0)
			return CalculationTypeMask(CalculationFinalType::Number);

		switch (CalculationDimension(dimension))
		{
		case CalculationDimension::Length: return CalculationTypeMask(CalculationFinalType::Length);
		case CalculationDimension::Angle: return CalculationTypeMask(CalculationFinalType::Angle);
		case CalculationDimension::Time: return CalculationTypeMask(CalculationFinalType::Time);
		case CalculationDimension::Resolution: return CalculationTypeMask(CalculationFinalType::Resolution);
		case CalculationDimension::Percent: return CalculationTypeMask(CalculationFinalType::Percent);
		default: return 0;
		}
	}

	bool IsWhitespace(char c)
	{
		return c == '\f' || StringUtilities::IsWhitespace(c);
	}

	bool IsDigit(char c)
	{
		return c >= '0' && c <= '9';
	}

	template <size_t N>
	bool EqualsAsciiCaseInsensitive(StringView string, const char (&literal)[N])
	{
		if (string.size() != N - 1)
			return false;
		for (size_t i = 0; i < N - 1; ++i)
		{
			const char c = string.begin()[i];
			const char lower = (c >= 'A' && c <= 'Z') ? char(c + ('a' - 'A')) : c;
			if (lower != literal[i])
				return false;
		}
		return true;
	}

	enum class FunctionKind : uint8_t { Unknown, Calc, Min, Max, Clamp };

	class Parser {
	public:
		Parser(const String& input, CalculationParseTarget target) : input(input), target(target) {}

		NodePtr Parse()
		{
			SkipWhitespace();
			NodePtr root = ParseFunction(0);
			SkipWhitespace();
			if (!root || position != input.size())
				return nullptr;
			if ((FinalTypeMask(root->type) & target.allowed_final_types) == 0)
				return nullptr;
			return root;
		}

		Units GetDependencyMask() const { return dependency_mask; }

	private:
		bool SkipWhitespace()
		{
			bool saw_whitespace = false;
			for (;;)
			{
				while (position < input.size() && IsWhitespace(input[position]))
				{
					saw_whitespace = true;
					++position;
				}
				if (position + 1 < input.size() && input[position] == '/' && input[position + 1] == '*')
				{
					const size_t end = input.find("*/", position + 2);
					if (end == String::npos)
					{
						position = input.size();
						break;
					}
					position = end + 2;
					continue;
				}
				break;
			}
			return saw_whitespace;
		}

		bool Consume(char c)
		{
			SkipWhitespace();
			if (position < input.size() && input[position] == c)
			{
				++position;
				return true;
			}
			return false;
		}

		bool ConsumeClose()
		{
			SkipWhitespace();
			if (position == input.size())
				return true;
			if (input[position] == ')')
			{
				++position;
				return true;
			}
			return false;
		}

		NodePtr ParseFunction(size_t depth)
		{
			if (depth > MaxDepth)
				return nullptr;
			SkipWhitespace();
			const size_t name_begin = position;
			while (position < input.size() &&
				((input[position] >= 'a' && input[position] <= 'z') || (input[position] >= 'A' && input[position] <= 'Z') || input[position] == '-'))
				++position;
			const StringView name(input, name_begin, position - name_begin);
			FunctionKind function = FunctionKind::Unknown;
			if (EqualsAsciiCaseInsensitive(name, "calc"))
				function = FunctionKind::Calc;
			else if (EqualsAsciiCaseInsensitive(name, "min"))
				function = FunctionKind::Min;
			else if (EqualsAsciiCaseInsensitive(name, "max"))
				function = FunctionKind::Max;
			else if (EqualsAsciiCaseInsensitive(name, "clamp"))
				function = FunctionKind::Clamp;
			if (function == FunctionKind::Unknown)
				return nullptr;
			if (position >= input.size() || input[position++] != '(')
				return nullptr;

			if (function == FunctionKind::Calc)
			{
				NodePtr node = ParseSum(depth);
				if (!node || !ConsumeClose())
					return nullptr;
				return node;
			}

			Vector<NodePtr> arguments;
			if (function == FunctionKind::Clamp)
			{
				arguments.reserve(3);
				for (size_t i = 0; i < 3; ++i)
				{
					SkipWhitespace();
					NodePtr argument;
					const bool none = (i == 0 || i == 2) && ConsumeKeyword("none");
					if (none)
						argument = nullptr;
					else
						argument = ParseSum(depth);
					if (!none && !argument)
						return nullptr;
					arguments.push_back(std::move(argument));
					if (i != 2 && !Consume(','))
						return nullptr;
				}
				if (!ConsumeClose())
					return nullptr;
				CalculationNumericType common;
				bool have_common = false;
				for (const NodePtr& argument : arguments)
				{
					if (!argument)
						continue;
					if (!have_common)
					{
						common = argument->type;
						have_common = true;
					}
					else if (common != argument->type)
						return nullptr;
				}
				if (!have_common)
					return nullptr;
				auto node = MakeShared<Calculation::Node>();
				node->kind = Calculation::Kind::Clamp;
				node->type = common;
				node->children = std::move(arguments);
				return node;
			}

			SkipWhitespace();
			if (position < input.size() && input[position] == ')')
				return nullptr;
			arguments.reserve(2);
			while (arguments.size() < MaxArguments)
			{
				NodePtr argument = ParseSum(depth);
				if (!argument)
					return nullptr;
				arguments.push_back(std::move(argument));
				SkipWhitespace();
				if (position == input.size() || input[position] == ')')
				{
					if (position < input.size())
						++position;
					break;
				}
				if (arguments.size() == MaxArguments)
					return nullptr;
				if (!Consume(','))
					return nullptr;
			}
			if (arguments.empty() || arguments.size() > MaxArguments)
				return nullptr;
			if (position > input.size())
				return nullptr;
			for (size_t i = 1; i < arguments.size(); ++i)
				if (arguments[i]->type != arguments[0]->type)
					return nullptr;
			auto node = MakeShared<Calculation::Node>();
			node->kind = function == FunctionKind::Min ? Calculation::Kind::Min : Calculation::Kind::Max;
			node->type = arguments[0]->type;
			node->children = std::move(arguments);
			return node;
		}

		template <size_t N>
		bool ConsumeKeyword(const char (&keyword)[N])
		{
			SkipWhitespace();
			constexpr size_t length = N - 1;
			if (position + length > input.size() || !EqualsAsciiCaseInsensitive(StringView(input, position, length), keyword))
				return false;
			const size_t end = position + length;
			if (end < input.size() && ((input[end] >= 'a' && input[end] <= 'z') || (input[end] >= 'A' && input[end] <= 'Z') || input[end] == '-'))
				return false;
			position = end;
			return true;
		}

		NodePtr ParseSum(size_t depth)
		{
			NodePtr lhs = ParseProduct(depth);
			if (!lhs)
				return nullptr;
			const CalculationNumericType type = lhs->type;
			Vector<NodePtr> children;
			for (;;)
			{
				const bool whitespace_before = SkipWhitespace();
				if (position >= input.size() || (input[position] != '+' && input[position] != '-'))
					break;
				if (children.empty())
				{
					children.reserve(2);
					children.push_back(std::move(lhs));
				}
				const char op = input[position++];
				const bool whitespace_after = SkipWhitespace();
				if (!whitespace_before || !whitespace_after)
					return nullptr;
				NodePtr rhs = ParseProduct(depth);
				if (!rhs || rhs->type != type)
					return nullptr;
				if (op == '-')
					rhs = MakeUnary(Calculation::Kind::Negate, std::move(rhs));
				children.push_back(std::move(rhs));
			}
			if (children.empty())
				return lhs;
			auto node = MakeShared<Calculation::Node>();
			node->kind = Calculation::Kind::Sum;
			node->type = type;
			node->children = std::move(children);
			return node;
		}

		NodePtr ParseProduct(size_t depth)
		{
			NodePtr lhs = ParseValue(depth);
			if (!lhs)
				return nullptr;
			Vector<NodePtr> children;
			CalculationNumericType type = lhs->type;
			for (;;)
			{
				const size_t before_whitespace = position;
				SkipWhitespace();
				if (position >= input.size() || (input[position] != '*' && input[position] != '/'))
				{
					position = before_whitespace;
					break;
				}
				if (children.empty())
				{
					children.reserve(2);
					children.push_back(std::move(lhs));
				}
				const char op = input[position++];
				NodePtr rhs = ParseValue(depth);
				if (!rhs)
					return nullptr;
				type = MultiplyTypes(type, rhs->type, op == '/' ? -1 : 1);
				if (op == '/')
					rhs = MakeUnary(Calculation::Kind::Invert, std::move(rhs));
				children.push_back(std::move(rhs));
			}
			if (children.empty())
				return lhs;
			auto node = MakeShared<Calculation::Node>();
			node->kind = Calculation::Kind::Product;
			node->type = type;
			node->children = std::move(children);
			return node;
		}

		NodePtr ParseValue(size_t depth)
		{
			SkipWhitespace();
			if (position >= input.size())
				return nullptr;
			if (input[position] == '(')
			{
				if (depth >= MaxDepth)
					return nullptr;
				++position;
				NodePtr value = ParseSum(depth + 1);
				if (!value || !ConsumeClose())
					return nullptr;
				return value;
			}
			if ((input[position] >= 'a' && input[position] <= 'z') || (input[position] >= 'A' && input[position] <= 'Z'))
			{
				if (depth >= MaxDepth)
					return nullptr;
				return ParseFunction(depth + 1);
			}
			return ParseNumericValue();
		}

		NodePtr ParseNumericValue()
		{
			if (++terms > MaxTerms)
				return nullptr;

			const size_t number_begin = position;
			if (position < input.size() && (input[position] == '+' || input[position] == '-'))
				++position;

			const size_t integer_begin = position;
			while (position < input.size() && IsDigit(input[position]))
				++position;
			bool has_digits = position > integer_begin;

			if (position + 1 < input.size() && input[position] == '.' && IsDigit(input[position + 1]))
			{
				++position;
				while (position < input.size() && IsDigit(input[position]))
					++position;
				has_digits = true;
			}
			if (!has_digits)
				return nullptr;

			if (position < input.size() && (input[position] == 'e' || input[position] == 'E'))
			{
				size_t exponent_position = position + 1;
				if (exponent_position < input.size() && (input[exponent_position] == '+' || input[exponent_position] == '-'))
					++exponent_position;
				if (exponent_position < input.size() && IsDigit(input[exponent_position]))
				{
					position = exponent_position + 1;
					while (position < input.size() && IsDigit(input[position]))
						++position;
				}
			}

			char* end = nullptr;
			const char* number_begin_ptr = input.c_str() + number_begin;
			const float number = strtof(number_begin_ptr, &end);
			if (end != input.c_str() + position || !std::isfinite(number))
				return nullptr;
			const size_t unit_begin = position;
			if (position < input.size() && input[position] == '%')
				++position;
			else
				while (position < input.size() &&
					((input[position] >= 'a' && input[position] <= 'z') || (input[position] >= 'A' && input[position] <= 'Z')))
					++position;
			const StringView unit_name(input, unit_begin, position - unit_begin);
			Unit unit = Unit::UNKNOWN;
			Calculation::ValueUnit value_unit = Calculation::ValueUnit::Public;
			bool milliseconds = false;
			if (unit_name.empty())
				unit = Unit::NUMBER;
			else if (EqualsAsciiCaseInsensitive(unit_name, "%"))
				unit = Unit::PERCENT;
			else if (EqualsAsciiCaseInsensitive(unit_name, "px"))
				unit = Unit::PX;
			else if (EqualsAsciiCaseInsensitive(unit_name, "dp"))
				unit = Unit::DP;
			else if (EqualsAsciiCaseInsensitive(unit_name, "vw"))
				unit = Unit::VW;
			else if (EqualsAsciiCaseInsensitive(unit_name, "vh"))
				unit = Unit::VH;
			else if (EqualsAsciiCaseInsensitive(unit_name, "em"))
				unit = Unit::EM;
			else if (EqualsAsciiCaseInsensitive(unit_name, "rem"))
				unit = Unit::REM;
			else if (EqualsAsciiCaseInsensitive(unit_name, "in"))
				unit = Unit::INCH;
			else if (EqualsAsciiCaseInsensitive(unit_name, "cm"))
				unit = Unit::CM;
			else if (EqualsAsciiCaseInsensitive(unit_name, "mm"))
				unit = Unit::MM;
			else if (EqualsAsciiCaseInsensitive(unit_name, "pt"))
				unit = Unit::PT;
			else if (EqualsAsciiCaseInsensitive(unit_name, "pc"))
				unit = Unit::PC;
			else if (EqualsAsciiCaseInsensitive(unit_name, "deg"))
				unit = Unit::DEG;
			else if (EqualsAsciiCaseInsensitive(unit_name, "rad"))
				unit = Unit::RAD;
			else if (EqualsAsciiCaseInsensitive(unit_name, "x"))
				unit = Unit::X;
			else if ((target.allowed_final_types & CalculationTypeMask(CalculationFinalType::Time)) != 0 &&
				(EqualsAsciiCaseInsensitive(unit_name, "s") || (milliseconds = EqualsAsciiCaseInsensitive(unit_name, "ms"))))
			{
				unit = Unit::NUMBER;
				value_unit = Calculation::ValueUnit::Seconds;
			}
			if (unit == Unit::UNKNOWN)
				return nullptr;
			if (unit == Unit::EM || unit == Unit::REM || unit == Unit::VW || unit == Unit::VH || unit == Unit::DP || Any(unit & Unit::PPI_UNIT))
				dependency_mask = dependency_mask | unit;
			auto node = MakeShared<Calculation::Node>();
			node->value = milliseconds ? number * 0.001f : number;
			node->unit = unit;
			node->value_unit = value_unit;
			node->type = value_unit == Calculation::ValueUnit::Seconds ? TimeType() : TypeForUnit(unit, target.percentage_hint);
			return node;
		}

		NodePtr MakeUnary(Calculation::Kind kind, NodePtr child)
		{
			auto node = MakeShared<Calculation::Node>();
			node->kind = kind;
			node->type = kind == Calculation::Kind::Invert ? MultiplyTypes({}, child->type, -1) : child->type;
			node->children.push_back(std::move(child));
			return node;
		}

		const String& input;
		CalculationParseTarget target;
		size_t position = 0;
		size_t terms = 0;
		Units dependency_mask = Unit::UNKNOWN;
	};

	struct EvalValue {
		float value = 0;
		CalculationNumericType type;
		Unit unit = Unit::UNKNOWN;
		Calculation::ValueUnit value_unit = Calculation::ValueUnit::Public;
		bool percent_only = false;
		int percentage_basis_power = 0;
		bool contains_dp_scaled_physical = false;
	};

	enum class EvalStatus { Success, Deferred, Failure };

	bool CanonicalValue(const Calculation::Node& node, EvalValue& result)
	{
		result.value = node.value;
		result.type = node.type;
		result.unit = node.unit;
		result.value_unit = node.value_unit;
		result.percent_only = node.unit == Unit::PERCENT;
		result.percentage_basis_power =
			(node.unit == Unit::PERCENT && FinalTypeMask(node.type) != CalculationTypeMask(CalculationFinalType::Percent)) ? 1 : 0;
		result.contains_dp_scaled_physical = Any(node.unit & Unit::PPI_UNIT);
		switch (node.unit)
		{
		case Unit::NUMBER: return true;
		case Unit::PERCENT: return true;
		case Unit::PX: return true;
		case Unit::INCH:
			result.value *= 96.f;
			result.unit = Unit::PX;
			return true;
		case Unit::CM:
			result.value *= 96.f / 2.54f;
			result.unit = Unit::PX;
			return true;
		case Unit::MM:
			result.value *= 96.f / 25.4f;
			result.unit = Unit::PX;
			return true;
		case Unit::PT:
			result.value *= 96.f / 72.f;
			result.unit = Unit::PX;
			return true;
		case Unit::PC:
			result.value *= 16.f;
			result.unit = Unit::PX;
			return true;
		case Unit::DEG: return true;
		case Unit::RAD:
			result.value *= 180.f / float(3.14159265358979323846);
			result.unit = Unit::DEG;
			return true;
		case Unit::X: return true;
		default: return false;
		}
	}

	Unit CanonicalUnit(const CalculationNumericType& type, bool percent_only)
	{
		if (percent_only)
			return Unit::PERCENT;
		const CalculationTypeMask mask = FinalTypeMask(type);
		if (mask == CalculationTypeMask(CalculationFinalType::Number))
			return Unit::NUMBER;
		if (mask == CalculationTypeMask(CalculationFinalType::Length))
			return Unit::PX;
		if (mask == CalculationTypeMask(CalculationFinalType::Angle))
			return Unit::DEG;
		if (mask == CalculationTypeMask(CalculationFinalType::Time))
			return Unit::NUMBER;
		if (mask == CalculationTypeMask(CalculationFinalType::Resolution))
			return Unit::X;
		if (mask == CalculationTypeMask(CalculationFinalType::Percent))
			return Unit::PERCENT;
		return Unit::UNKNOWN;
	}

	EvalStatus EvaluateNode(const Calculation::Node& node, EvalValue& result)
	{
		if (node.kind == Calculation::Kind::Value)
		{
			if (!CanonicalValue(node, result))
				return EvalStatus::Deferred;
			return std::isfinite(result.value) ? EvalStatus::Success : EvalStatus::Failure;
		}
		if (node.kind == Calculation::Kind::Negate || node.kind == Calculation::Kind::Invert)
		{
			if (node.children.size() != 1 || !node.children[0])
				return EvalStatus::Failure;
			const EvalStatus child_status = EvaluateNode(*node.children[0], result);
			if (child_status != EvalStatus::Success)
				return child_status;
			if (node.kind == Calculation::Kind::Negate)
				result.value = -result.value;
			else
			{
				result.value = 1.f / result.value;
				result.type = node.type;
				result.percent_only = false;
				result.percentage_basis_power = -result.percentage_basis_power;
			}
			result.unit = CanonicalUnit(result.type, result.percent_only);
			result.value_unit = FinalTypeMask(result.type) == CalculationTypeMask(CalculationFinalType::Time) ? Calculation::ValueUnit::Seconds
																											  : Calculation::ValueUnit::Public;
			return std::isfinite(result.value) ? EvalStatus::Success : EvalStatus::Failure;
		}

		if (node.kind == Calculation::Kind::Sum || node.kind == Calculation::Kind::Product)
		{
			if (node.children.empty())
				return EvalStatus::Failure;
			EvalValue aggregate;
			const EvalStatus first_status = EvaluateNode(*node.children[0], aggregate);
			if (first_status != EvalStatus::Success)
				return first_status;
			for (size_t i = 1; i < node.children.size(); ++i)
			{
				EvalValue rhs;
				if (!node.children[i])
					return EvalStatus::Failure;
				const EvalStatus rhs_status = EvaluateNode(*node.children[i], rhs);
				if (rhs_status != EvalStatus::Success)
					return rhs_status;
				if (node.kind == Calculation::Kind::Sum)
				{
					if (aggregate.unit != rhs.unit || aggregate.percentage_basis_power != rhs.percentage_basis_power)
						return EvalStatus::Deferred;
					aggregate.value += rhs.value;
					aggregate.percent_only = aggregate.percent_only && rhs.percent_only;
				}
				else
				{
					aggregate.value *= rhs.value;
					aggregate.type = MultiplyTypes(aggregate.type, rhs.type);
					aggregate.percent_only = aggregate.percent_only && (rhs.unit == Unit::NUMBER || rhs.percent_only);
					aggregate.percentage_basis_power += rhs.percentage_basis_power;
					aggregate.unit = CanonicalUnit(aggregate.type, aggregate.percent_only);
					aggregate.value_unit = FinalTypeMask(aggregate.type) == CalculationTypeMask(CalculationFinalType::Time)
						? Calculation::ValueUnit::Seconds
						: Calculation::ValueUnit::Public;
				}
				aggregate.contains_dp_scaled_physical |= rhs.contains_dp_scaled_physical;
				if (!std::isfinite(aggregate.value))
					return EvalStatus::Failure;
			}
			aggregate.type = node.type;
			aggregate.unit = CanonicalUnit(node.type, aggregate.percent_only);
			result = aggregate;
			result.value_unit = FinalTypeMask(result.type) == CalculationTypeMask(CalculationFinalType::Time) ? Calculation::ValueUnit::Seconds
																											  : Calculation::ValueUnit::Public;
			if (result.percentage_basis_power != 0)
			{
				const CalculationTypeMask final_type = FinalTypeMask(result.type);
				const bool representable_percentage = result.percentage_basis_power == 1 && result.percent_only && result.unit == Unit::PERCENT &&
					(final_type == CalculationTypeMask(CalculationFinalType::Length) ||
						final_type == CalculationTypeMask(CalculationFinalType::Angle));
				if (!representable_percentage)
					return EvalStatus::Deferred;
			}
			return result.unit != Unit::UNKNOWN ? EvalStatus::Success : EvalStatus::Deferred;
		}

		if (node.kind == Calculation::Kind::Min || node.kind == Calculation::Kind::Max || node.kind == Calculation::Kind::Clamp)
		{
			Vector<EvalValue> values;
			values.reserve(node.children.size());
			bool contains_dp_scaled_physical = false;
			for (const NodePtr& child : node.children)
			{
				if (!child)
				{
					values.push_back({});
					continue;
				}
				EvalValue value;
				const EvalStatus status = EvaluateNode(*child, value);
				if (status != EvalStatus::Success)
					return status;
				contains_dp_scaled_physical |= value.contains_dp_scaled_physical;
				values.push_back(value);
			}
			Unit common_unit = Unit::UNKNOWN;
			int common_percentage_basis_power = 0;
			bool have_common_percentage_basis_power = false;
			for (size_t i = 0; i < values.size(); ++i)
			{
				if (!node.children[i])
					continue;
				if (common_unit == Unit::UNKNOWN)
					common_unit = values[i].unit;
				else if (values[i].unit != common_unit)
					return EvalStatus::Deferred;
				if (!have_common_percentage_basis_power)
				{
					common_percentage_basis_power = values[i].percentage_basis_power;
					have_common_percentage_basis_power = true;
				}
				else if (values[i].percentage_basis_power != common_percentage_basis_power)
					return EvalStatus::Deferred;
			}
			if (node.kind == Calculation::Kind::Min || node.kind == Calculation::Kind::Max)
			{
				result = values[0];
				for (size_t i = 1; i < values.size(); ++i)
					if ((node.kind == Calculation::Kind::Min && values[i].value < result.value) ||
						(node.kind == Calculation::Kind::Max && values[i].value > result.value))
						result = values[i];
			}
			else
			{
				const bool has_min = bool(node.children[0]);
				const bool has_max = bool(node.children[2]);
				result = values[1];
				if (has_max && result.value > values[2].value)
					result = values[2];
				if (has_min && result.value < values[0].value)
					result = values[0];
			}
			result.type = node.type;
			result.unit = common_unit;
			result.value_unit = FinalTypeMask(node.type) == CalculationTypeMask(CalculationFinalType::Time) ? Calculation::ValueUnit::Seconds
																											: Calculation::ValueUnit::Public;
			result.percentage_basis_power = common_percentage_basis_power;
			result.contains_dp_scaled_physical = contains_dp_scaled_physical;
			return std::isfinite(result.value) ? EvalStatus::Success : EvalStatus::Failure;
		}
		return EvalStatus::Failure;
	}

	NodePtr SimplifyNode(const NodePtr& node, bool& failed)
	{
		EvalValue value;
		const EvalStatus status = EvaluateNode(*node, value);
		if (status == EvalStatus::Failure)
		{
			failed = true;
			return nullptr;
		}
		if (status == EvalStatus::Success && value.unit != Unit::UNKNOWN && !value.contains_dp_scaled_physical &&
			node->kind != Calculation::Kind::Negate && node->kind != Calculation::Kind::Invert)
		{
			if (node->kind == Calculation::Kind::Value && node->value == value.value && node->unit == value.unit &&
				node->value_unit == value.value_unit)
				return node;
			auto constant = MakeShared<Calculation::Node>();
			constant->value = value.value;
			constant->unit = value.unit;
			constant->value_unit = value.value_unit;
			constant->type = node->type;
			return constant;
		}

		SharedPtr<Calculation::Node> simplified;
		for (size_t i = 0; i < node->children.size(); ++i)
		{
			const NodePtr& child = node->children[i];
			if (!child)
				continue;
			NodePtr simplified_child = SimplifyNode(child, failed);
			if (failed)
				return nullptr;
			if (simplified_child != child)
			{
				if (!simplified)
					simplified = MakeShared<Calculation::Node>(*node);
				simplified->children[i] = std::move(simplified_child);
			}
		}
		return simplified ? NodePtr(std::move(simplified)) : node;
	}

} // namespace

CalculationParseTarget MakeCalculationParseTarget(CalculationFinalType type, CalculationPercentageHint hint)
{
	return {CalculationTypeMask(type), hint};
}

bool ParseCalculation(const String& expression, const CalculationParseTarget& target, CalculationPtr& result)
{
	result.reset();
	Parser parser(expression, target);
	NodePtr root = parser.Parse();
	if (!root)
		return false;
	const Units dependency_mask = parser.GetDependencyMask();
	bool failed = false;
	NodePtr simplified_root = SimplifyNode(root, failed);
	if (failed || !simplified_root)
	{
		return false;
	}
	result = MakeShared<Calculation>(std::move(simplified_root), dependency_mask);
	return true;
}

bool EvaluateCalculation(const Calculation& calculation, CalculationConstantValue& result)
{
	if (!calculation.GetRoot())
		return false;
	EvalValue value;
	if (EvaluateNode(*calculation.GetRoot(), value) != EvalStatus::Success || !std::isfinite(value.value))
		return false;
	result.value = value.value;
	result.unit = value.unit;
	return result.unit != Unit::UNKNOWN;
}

bool EvaluateCalculationTime(const Calculation& calculation, float& seconds)
{
	seconds = 0.f;
	if (!calculation.GetRoot() || FinalTypeMask(calculation.GetType()) != CalculationTypeMask(CalculationFinalType::Time))
		return false;
	EvalValue value;
	if (EvaluateNode(*calculation.GetRoot(), value) != EvalStatus::Success || value.value_unit != Calculation::ValueUnit::Seconds ||
		!std::isfinite(value.value))
		return false;
	seconds = value.value;
	return true;
}

CalculationTypeMask GetCalculationFinalType(const Calculation& calculation)
{
	return calculation.GetRoot() ? FinalTypeMask(calculation.GetType()) : 0;
}

bool SimplifyCalculation(const Calculation& calculation, CalculationPtr& result)
{
	result.reset();
	if (!calculation.GetRoot())
		return false;
	bool failed = false;
	NodePtr root = SimplifyNode(calculation.GetRoot(), failed);
	if (failed || !root)
		return false;
	result = MakeShared<Calculation>(std::move(root), calculation.GetDependencyMask(), calculation.GetResidualForm(), calculation.GetLinearPx(),
		calculation.GetLinearPercent());
	return true;
}

} // namespace Rml
