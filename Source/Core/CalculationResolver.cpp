#include "CalculationResolver.h"
#include "../../Include/RmlUi/Core/ComputedValues.h"
#include "../../Include/RmlUi/Core/Context.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/ElementDocument.h"
#include "../../Include/RmlUi/Core/Math.h"
#include "CalculationParser.h"
#include "ComputeProperty.h"
#include <cmath>

namespace Rml {
namespace {

	using NodePtr = SharedPtr<const Calculation::Node>;

	float PhysicalToPx(float value, Unit unit, float dp_ratio)
	{
		float inch = value * 96.f * dp_ratio;
		switch (unit)
		{
		case Unit::INCH: return inch;
		case Unit::CM: return inch / 2.54f;
		case Unit::MM: return inch / 25.4f;
		case Unit::PT: return inch / 72.f;
		case Unit::PC: return inch / 6.f;
		default: return value;
		}
	}

	bool PercentageBasis(const CalculationResolverContext& context, float& basis)
	{
		switch (context.relative_target)
		{
		case RelativeTarget::FontSize: basis = context.font_size; return true;
		case RelativeTarget::ParentFontSize: basis = context.parent_font_size; return true;
		case RelativeTarget::LineHeight: basis = context.line_height; return true;
		case RelativeTarget::None:
		case RelativeTarget::ContainingBlockWidth:
		case RelativeTarget::ContainingBlockHeight: return false;
		}
		return false;
	}

	NodePtr CanonicalizeNode(const NodePtr& node, const CalculationResolverContext& context, bool& failed)
	{
		if (node->kind == Calculation::Kind::Value)
		{
			float value = node->value;
			Unit unit = node->unit;
			switch (node->unit)
			{
			case Unit::PX:
			case Unit::NUMBER:
			case Unit::X: break;
			case Unit::DP:
				value *= context.dp_ratio;
				unit = Unit::PX;
				break;
			case Unit::VW:
				value *= context.viewport_dimensions.x * .01f;
				unit = Unit::PX;
				break;
			case Unit::VH:
				value *= context.viewport_dimensions.y * .01f;
				unit = Unit::PX;
				break;
			case Unit::EM:
				value *= (context.relative_target == RelativeTarget::ParentFontSize ? context.parent_font_size : context.font_size);
				unit = Unit::PX;
				break;
			case Unit::REM:
				value *= context.document_font_size;
				unit = Unit::PX;
				break;
			case Unit::INCH:
			case Unit::CM:
			case Unit::MM:
			case Unit::PT:
			case Unit::PC:
				value = PhysicalToPx(value, node->unit, context.dp_ratio);
				unit = Unit::PX;
				break;
			case Unit::DEG:
				value = Math::DegreesToRadians(value);
				unit = Unit::RAD;
				break;
			case Unit::RAD: break;
			case Unit::PERCENT:
			{
				float basis = 0.f;
				if (PercentageBasis(context, basis))
				{
					value = value * .01f * basis;
					unit = Unit::PX;
				}
				break;
			}
			default: break;
			}
			if (!std::isfinite(value))
			{
				failed = true;
				return nullptr;
			}
			if (value == node->value && unit == node->unit)
				return node;
			auto result = MakeShared<Calculation::Node>(*node);
			result->value = value;
			result->unit = unit;
			return result;
		}

		SharedPtr<Calculation::Node> result;
		for (size_t i = 0; i < node->children.size(); ++i)
		{
			const NodePtr& child = node->children[i];
			if (!child)
				continue;
			NodePtr canonical_child = CanonicalizeNode(child, context, failed);
			if (failed || !canonical_child)
				return nullptr;
			if (canonical_child != child)
			{
				if (!result)
					result = MakeShared<Calculation::Node>(*node);
				result->children[i] = std::move(canonical_child);
			}
		}
		return result ? NodePtr(std::move(result)) : node;
	}

	struct Affine {
		bool valid = false;
		bool scalar = false;
		float number = 0.f;
		float px = 0.f;
		float percent = 0.f;
	};

	Affine AnalyzeAffine(const Calculation::Node& node)
	{
		if (node.kind == Calculation::Kind::Value)
		{
			if (node.unit == Unit::NUMBER)
				return {true, true, node.value, 0.f, 0.f};
			if (node.unit == Unit::PX)
				return {true, false, 0.f, node.value, 0.f};
			if (node.unit == Unit::PERCENT)
				return {true, false, 0.f, 0.f, node.value};
			return {};
		}
		if (node.kind == Calculation::Kind::Negate && node.children.size() == 1 && node.children[0])
		{
			Affine a = AnalyzeAffine(*node.children[0]);
			if (!a.valid)
				return {};
			a.number = -a.number;
			a.px = -a.px;
			a.percent = -a.percent;
			return a;
		}
		if (node.kind == Calculation::Kind::Invert && node.children.size() == 1 && node.children[0])
		{
			Affine a = AnalyzeAffine(*node.children[0]);
			if (!a.valid || !a.scalar || a.number == 0.f)
				return {};
			a.number = 1.f / a.number;
			return std::isfinite(a.number) ? a : Affine{};
		}
		if (node.kind == Calculation::Kind::Sum)
		{
			Affine out{true, false, 0.f, 0.f, 0.f};
			for (const NodePtr& child : node.children)
			{
				if (!child)
					return {};
				Affine a = AnalyzeAffine(*child);
				if (!a.valid || a.scalar)
					return {};
				out.px += a.px;
				out.percent += a.percent;
			}
			return out;
		}
		if (node.kind == Calculation::Kind::Product)
		{
			Affine out{true, true, 1.f, 0.f, 0.f};
			for (const NodePtr& child : node.children)
			{
				if (!child)
					return {};
				Affine a = AnalyzeAffine(*child);
				if (!a.valid)
					return {};
				if (out.scalar && a.scalar)
					out.number *= a.number;
				else if (out.scalar && !a.scalar)
				{
					out.px = a.px * out.number;
					out.percent = a.percent * out.number;
					out.number = 0.f;
					out.scalar = false;
				}
				else if (!out.scalar && a.scalar)
				{
					out.px *= a.number;
					out.percent *= a.number;
				}
				else
					return {};
			}
			return out;
		}
		return {};
	}

	bool EvaluateUsedNode(const Calculation::Node& node, float percentage_basis, float& result)
	{
		if (node.kind == Calculation::Kind::Value)
		{
			if (node.unit == Unit::PERCENT)
				result = node.value * .01f * percentage_basis;
			else
				result = node.value;
			return std::isfinite(result);
		}

		if (node.kind == Calculation::Kind::Negate || node.kind == Calculation::Kind::Invert)
		{
			if (node.children.size() != 1 || !node.children[0] || !EvaluateUsedNode(*node.children[0], percentage_basis, result))
				return false;
			if (node.kind == Calculation::Kind::Negate)
				result = -result;
			else
			{
				if (result == 0.f)
					return false;
				result = 1.f / result;
			}
			return std::isfinite(result);
		}

		if (node.kind == Calculation::Kind::Sum || node.kind == Calculation::Kind::Product || node.kind == Calculation::Kind::Min ||
			node.kind == Calculation::Kind::Max)
		{
			if (node.children.empty())
				return false;
			float accumulated = (node.kind == Calculation::Kind::Product ? 1.f : 0.f);
			bool first = true;
			for (const NodePtr& child : node.children)
			{
				float value = 0.f;
				if (!child || !EvaluateUsedNode(*child, percentage_basis, value))
					return false;
				if (node.kind == Calculation::Kind::Sum)
					accumulated += value;
				else if (node.kind == Calculation::Kind::Product)
					accumulated *= value;
				else if (first || (node.kind == Calculation::Kind::Min ? value < accumulated : value > accumulated))
					accumulated = value;
				first = false;
				if (!std::isfinite(accumulated))
					return false;
			}
			result = accumulated;
			return true;
		}

		if (node.kind == Calculation::Kind::Clamp)
		{
			if (node.children.size() != 3 || !node.children[1])
				return false;
			float value = 0.f;
			if (!EvaluateUsedNode(*node.children[1], percentage_basis, value))
				return false;
			if (node.children[2])
			{
				float maximum = 0.f;
				if (!EvaluateUsedNode(*node.children[2], percentage_basis, maximum))
					return false;
				value = Math::Min(value, maximum);
			}
			if (node.children[0])
			{
				float minimum = 0.f;
				if (!EvaluateUsedNode(*node.children[0], percentage_basis, minimum))
					return false;
				value = Math::Max(value, minimum);
			}
			result = value;
			return std::isfinite(result);
		}

		return false;
	}

	// Resolve a parsed calculation directly to its canonical scalar for element/compound use. Unlike
	// ResolveCalculation(), this path deliberately does not canonicalize/simplify into replacement
	// Calculation trees: animation endpoints and lazy compound values can be resolved every frame/use
	// without allocating a transient calculation tree.
	bool EvaluateElementNode(const Calculation::Node& node, const CalculationResolverContext& context, float percentage_basis, float& result)
	{
		if (node.kind == Calculation::Kind::Value)
		{
			result = node.value;
			switch (node.unit)
			{
			case Unit::NUMBER:
			case Unit::PX:
			case Unit::RAD:
			case Unit::X: break;
			case Unit::DP: result *= context.dp_ratio; break;
			case Unit::VW: result *= context.viewport_dimensions.x * .01f; break;
			case Unit::VH: result *= context.viewport_dimensions.y * .01f; break;
			case Unit::EM:
				result *= (context.relative_target == RelativeTarget::ParentFontSize ? context.parent_font_size : context.font_size);
				break;
			case Unit::REM: result *= context.document_font_size; break;
			case Unit::INCH:
			case Unit::CM:
			case Unit::MM:
			case Unit::PT:
			case Unit::PC: result = PhysicalToPx(result, node.unit, context.dp_ratio); break;
			case Unit::DEG: result = Math::DegreesToRadians(result); break;
			case Unit::PERCENT:
			{
				float basis = percentage_basis;
				PercentageBasis(context, basis);
				result *= .01f * basis;
				break;
			}
			default: return false;
			}
			return std::isfinite(result);
		}

		if (node.kind == Calculation::Kind::Negate || node.kind == Calculation::Kind::Invert)
		{
			if (node.children.size() != 1 || !node.children[0] || !EvaluateElementNode(*node.children[0], context, percentage_basis, result))
				return false;
			if (node.kind == Calculation::Kind::Negate)
				result = -result;
			else
			{
				if (result == 0.f)
					return false;
				result = 1.f / result;
			}
			return std::isfinite(result);
		}

		if (node.kind == Calculation::Kind::Sum || node.kind == Calculation::Kind::Product || node.kind == Calculation::Kind::Min ||
			node.kind == Calculation::Kind::Max)
		{
			if (node.children.empty())
				return false;
			float accumulated = (node.kind == Calculation::Kind::Product ? 1.f : 0.f);
			bool first = true;
			for (const NodePtr& child : node.children)
			{
				float value = 0.f;
				if (!child || !EvaluateElementNode(*child, context, percentage_basis, value))
					return false;
				if (node.kind == Calculation::Kind::Sum)
					accumulated += value;
				else if (node.kind == Calculation::Kind::Product)
					accumulated *= value;
				else if (first || (node.kind == Calculation::Kind::Min ? value < accumulated : value > accumulated))
					accumulated = value;
				first = false;
				if (!std::isfinite(accumulated))
					return false;
			}
			result = accumulated;
			return true;
		}

		if (node.kind == Calculation::Kind::Clamp)
		{
			if (node.children.size() != 3 || !node.children[1])
				return false;
			float value = 0.f;
			if (!EvaluateElementNode(*node.children[1], context, percentage_basis, value))
				return false;
			if (node.children[2])
			{
				float maximum = 0.f;
				if (!EvaluateElementNode(*node.children[2], context, percentage_basis, maximum))
					return false;
				value = Math::Min(value, maximum);
			}
			if (node.children[0])
			{
				float minimum = 0.f;
				if (!EvaluateElementNode(*node.children[0], context, percentage_basis, minimum))
					return false;
				value = Math::Max(value, minimum);
			}
			result = value;
			return std::isfinite(result);
		}

		return false;
	}

} // namespace

bool ResolveCalculation(const Calculation& calculation, const CalculationResolverContext& context, ResolvedCalculation& result)
{
	result = {};
	if (!calculation.GetRoot())
		return false;
	bool failed = false;
	NodePtr root = CanonicalizeNode(calculation.GetRoot(), context, failed);
	if (failed || !root)
		return false;

	Calculation contextual(root, calculation.GetDependencyMask());
	CalculationPtr simplified;
	if (!SimplifyCalculation(contextual, simplified))
		return false;

	CalculationConstantValue constant;
	if (EvaluateCalculation(*simplified, constant))
	{
		result.form = Calculation::ResidualForm::Constant;
		result.is_constant = true;
		result.value = constant.value;
		result.unit = constant.unit;
		if (result.unit == Unit::DEG)
		{
			result.value = Math::DegreesToRadians(result.value);
			result.unit = Unit::RAD;
		}
		return std::isfinite(result.value);
	}

	const Affine affine = AnalyzeAffine(*simplified->GetRoot());
	if (affine.valid && !affine.scalar && affine.px == 0.f && std::isfinite(affine.percent))
	{
		result.form = Calculation::ResidualForm::Constant;
		result.is_constant = true;
		result.value = affine.percent;
		result.unit = Unit::PERCENT;
		return true;
	}
	if (affine.valid && !affine.scalar && std::isfinite(affine.px) && std::isfinite(affine.percent))
	{
		result.form = Calculation::ResidualForm::LinearLengthPercentage;
		result.residual = MakeShared<Calculation>(simplified->GetRoot(), calculation.GetDependencyMask(),
			Calculation::ResidualForm::LinearLengthPercentage, affine.px, affine.percent);
	}
	else
	{
		result.form = Calculation::ResidualForm::Tree;
		result.residual = MakeShared<Calculation>(simplified->GetRoot(), calculation.GetDependencyMask(), Calculation::ResidualForm::Tree);
	}
	return bool(result.residual);
}

bool ResolveUsedCalculation(const Calculation& calculation, float percentage_basis, float& result)
{
	if (!std::isfinite(percentage_basis) || percentage_basis < 0.f)
		return false;

	if (calculation.GetResidualForm() == Calculation::ResidualForm::LinearLengthPercentage)
	{
		result = calculation.GetLinearPx() + calculation.GetLinearPercent() * .01f * percentage_basis;
		return std::isfinite(result);
	}

	if (!calculation.GetRoot())
		return false;
	return EvaluateUsedNode(*calculation.GetRoot(), percentage_basis, result);
}

bool ResolveElementCalculation(const Calculation& calculation, Element& element, float percentage_basis, float& result,
	RelativeTarget relative_target)
{
	if (!calculation.GetRoot() || !std::isfinite(percentage_basis))
		return false;

	CalculationResolverContext context;
	context.font_size = element.GetComputedValues().font_size();
	if (Element* parent = element.GetParentNode())
		context.parent_font_size = parent->GetComputedValues().font_size();
	else
		context.parent_font_size = DefaultComputedValues().font_size();
	if (ElementDocument* document = element.GetOwnerDocument())
		context.document_font_size = document->GetComputedValues().font_size();
	else
		context.document_font_size = DefaultComputedValues().font_size();
	context.line_height = element.GetLineHeight();
	context.relative_target = relative_target;
	if (Context* element_context = element.GetContext())
	{
		context.viewport_dimensions = Vector2f(element_context->GetDimensions());
		context.dp_ratio = element_context->GetDensityIndependentPixelRatio();
	}

	return EvaluateElementNode(*calculation.GetRoot(), context, percentage_basis, result);
}

} // namespace Rml
