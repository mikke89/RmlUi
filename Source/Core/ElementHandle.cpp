#include "ElementHandle.h"
#include "../../Include/RmlUi/Core/ComputedValues.h"
#include "../../Include/RmlUi/Core/Context.h"
#include "../../Include/RmlUi/Core/ElementDocument.h"
#include "../../Include/RmlUi/Core/ElementUtilities.h"
#include "../../Include/RmlUi/Core/Event.h"
#include "../../Include/RmlUi/Core/Property.h"
#include "../../Include/RmlUi/Core/PropertyDefinition.h"
#include "../../Include/RmlUi/Core/PropertyDictionary.h"
#include "../../Include/RmlUi/Core/PropertySpecification.h"

namespace Rml {

class ElementHandleTargetData {
public:
	enum { TOP, RIGHT, BOTTOM, LEFT, NUM_EDGES };
	using MoveData = ElementHandle::MoveData;
	using SizeData = ElementHandle::SizeData;

	ElementHandleTargetData(Element* target, Context* context, const Array<NumericValue, NUM_EDGES>& edge_margin) :
		target(target), computed(target->GetComputedValues()), box(target->GetBox()), parent_box(GetParentBox(target, context)),
		containing_block(target->GetContainingBlock()), position(computed.position()),
		resolved_edge_margin(ResolveEdgeMargin(target, box, edge_margin))
	{
		SetDefiniteMargins();
	}

	// The following table lists the expected behavior for each combination of definite (non-auto) properties:
	//
	//   Definite properties  | Move                    | Size
	//   ---------------------|-------------------------|--------------------------
	//   (none)               | left += dx              | width += dx
	//   left                 | left += dx              | width += dx
	//   right                | right -= dx             | right -= dx; width += dx
	//   width                | left += dx              | width += dx
	//   left & right         | left += dx; right -= dx | right -= dx
	//   left & width         | left += dx;             | width += dx
	//   right & width        | right -= dx             | right -= dx; width += dx
	//   right & width        | right -= dx             | right -= dx; width += dx
	//   left & right & width | left += dx;             | width += dx
	//
	// For simplicity, this table only specifies the horizontal direction. The same behavior applies for the
	// corresponding properties in the vertical direction. For now, we assume that the handle is anchored to the
	// bottom-right corner of the target element.

	MoveData GetMoveData(Vector2f& drag_delta_min, Vector2f& drag_delta_max) const
	{
		using namespace Style;
		MoveData data = {};

		data.original_position_top_left = {GetPositionLeft(), GetPositionTop()};
		data.original_position_bottom_right = {GetPositionRight(), GetPositionBottom()};

		data.bottom_right.x = (computed.right().type != Right::Auto);
		data.bottom_right.y = (computed.bottom().type != Bottom::Auto);
		data.top_left.x = (!data.bottom_right.x || computed.left().type != Left::Auto);
		data.top_left.y = (!data.bottom_right.y || computed.top().type != Top::Auto);

		const Vector2f distance_to_top_left = DistanceToTopLeft();
		const Vector2f distance_to_bottom_right = DistanceToBottomRight(distance_to_top_left);

		drag_delta_min = Math::Max(drag_delta_min, Vector2f{resolved_edge_margin[LEFT], resolved_edge_margin[TOP]} - distance_to_top_left);
		drag_delta_max = Math::Min(drag_delta_max, Vector2f{-resolved_edge_margin[RIGHT], -resolved_edge_margin[BOTTOM]} + distance_to_bottom_right);

		return data;
	}

	SizeData GetSizeData(Vector2f& drag_delta_min, Vector2f& drag_delta_max) const
	{
		using namespace Style;
		SizeData data = {};

		data.original_size = box.GetSize(computed.box_sizing() == BoxSizing::BorderBox ? BoxArea::Border : BoxArea::Content);
		data.original_position_bottom_right = {GetPositionRight(), GetPositionBottom()};

		data.bottom_right.x = (computed.right().type != Right::Auto);
		data.bottom_right.y = (computed.bottom().type != Bottom::Auto);
		data.width_height.x = (computed.left().type == Left::Auto || computed.right().type == Right::Auto || computed.width().type != Width::Auto);
		data.width_height.y = (computed.top().type == Top::Auto || computed.bottom().type == Bottom::Auto || computed.height().type != Height::Auto);

		const Vector2f min_size = {
			ResolveValue(computed.min_width(), containing_block.x),
			ResolveValue(computed.min_height(), containing_block.y),
		};
		const Vector2f max_size = {
			ResolveValueOr(computed.max_width(), containing_block.x, FLT_MAX),
			ResolveValueOr(computed.max_height(), containing_block.y, FLT_MAX),
		};
		const Vector2f distance_to_bottom_right = DistanceToBottomRight(DistanceToTopLeft());

		drag_delta_min = Math::Max(drag_delta_min, min_size - data.original_size);
		drag_delta_max = Math::Min(drag_delta_max, max_size - data.original_size);
		drag_delta_max = Math::Min(drag_delta_max, Vector2f{-resolved_edge_margin[RIGHT], -resolved_edge_margin[BOTTOM]} + distance_to_bottom_right);

		return data;
	}

private:
	void SetDefiniteMargins()
	{
		// Set any auto margins to their current value, since auto-margins may affect the size and position of an element.
		auto SetDefiniteMargin = [](Element* element, PropertyId margin_id, BoxEdge edge) {
			element->SetProperty(margin_id, Property(Math::Round(element->GetBox().GetEdge(BoxArea::Margin, edge)), Unit::PX));
		};
		using Style::Margin;
		if (computed.margin_top().type == Margin::Auto)
			SetDefiniteMargin(target, PropertyId::MarginTop, BoxEdge::Top);
		if (computed.margin_right().type == Margin::Auto)
			SetDefiniteMargin(target, PropertyId::MarginRight, BoxEdge::Right);
		if (computed.margin_bottom().type == Margin::Auto)
			SetDefiniteMargin(target, PropertyId::MarginBottom, BoxEdge::Bottom);
		if (computed.margin_left().type == Margin::Auto)
			SetDefiniteMargin(target, PropertyId::MarginLeft, BoxEdge::Left);
	}

	static Array<float, NUM_EDGES> ResolveEdgeMargin(Element* target, const Box& box, const Array<NumericValue, NUM_EDGES>& edge_margin)
	{
		const Vector2f target_size = box.GetSize(BoxArea::Border);
		Array<float, NUM_EDGES> resolved_edge_margin = {};
		for (int i = 0; i < NUM_EDGES; i++)
		{
			resolved_edge_margin[i] = (edge_margin[i].unit == Unit::UNKNOWN
					? -FLT_MAX
					: Math::Round(target->ResolveNumericValue(edge_margin[i], target_size[(i == LEFT || i == RIGHT) ? 0 : 1])));
		}
		return resolved_edge_margin;
	}

	static const Box& GetParentBox(Element* target, Context* context)
	{
		return target->GetOffsetParent() ? target->GetOffsetParent()->GetBox() : context->GetRootElement()->GetBox();
	}

	template <typename Func>
	static float ResolveValueOrInvoke(const Style::LengthPercentageAuto& value, float containing_block, Style::Position position,
		Func&& fallback_func)
	{
		if (value.type != Style::LengthPercentageAuto::Auto)
			return ResolveValue(value, containing_block);
		if (position == Style::Position::Relative)
			return 0.0f;
		return fallback_func();
	}

	// The following is derived at by solving the expressions in 'Element::UpdateOffset' for the computed top/left/bottom/right values.
	float GetPositionTop() const
	{
		return ResolveValueOrInvoke(computed.top(), containing_block.y, position, [&] {
			return target->GetOffsetTop() - (box.GetEdge(BoxArea::Margin, BoxEdge::Top) + parent_box.GetEdge(BoxArea::Border, BoxEdge::Top));
		});
	}
	float GetPositionLeft() const
	{
		return ResolveValueOrInvoke(computed.left(), containing_block.x, position, [&] {
			return target->GetOffsetLeft() - (box.GetEdge(BoxArea::Margin, BoxEdge::Left) + parent_box.GetEdge(BoxArea::Border, BoxEdge::Left));
		});
	}
	float GetPositionBottom() const
	{
		return ResolveValueOrInvoke(computed.bottom(), containing_block.y, position, [&] {
			return containing_block.y + parent_box.GetEdge(BoxArea::Border, BoxEdge::Top) -
				(target->GetOffsetTop() + box.GetSize(BoxArea::Border).y + box.GetEdge(BoxArea::Margin, BoxEdge::Bottom));
		});
	}
	float GetPositionRight() const
	{
		return ResolveValueOrInvoke(computed.right(), containing_block.x, position, [&] {
			return containing_block.x + parent_box.GetEdge(BoxArea::Border, BoxEdge::Left) -
				(target->GetOffsetLeft() + box.GetSize(BoxArea::Border).x + box.GetEdge(BoxArea::Margin, BoxEdge::Right));
		});
	}

	Vector2f DistanceToTopLeft() const
	{
		return {target->GetOffsetLeft() - parent_box.GetEdge(BoxArea::Border, BoxEdge::Left),
			target->GetOffsetTop() - parent_box.GetEdge(BoxArea::Border, BoxEdge::Top)};
	}
	Vector2f DistanceToBottomRight(Vector2f distance_to_top_left) const
	{
		const Vector2f scroll_size = {target->GetParentNode()->GetScrollWidth(), target->GetParentNode()->GetScrollHeight()};
		return scroll_size - box.GetSize(BoxArea::Border) - distance_to_top_left;
	}

	Element* target;
	const ComputedValues& computed;
	const Box& box;
	const Box& parent_box;
	const Vector2f containing_block;
	const Style::Position position;
	const Array<float, NUM_EDGES> resolved_edge_margin;
};

class HandleEdgeMarginParser {
private:
	PropertySpecification specification;
	Array<PropertyId, 4> ids;
	ShorthandId id_constraint;

public:
	HandleEdgeMarginParser() : specification(4, 1)
	{
		ids = {
			specification.RegisterProperty("edge-t", "", false, false).AddParser("length_percent").GetId(),
			specification.RegisterProperty("edge-r", "", false, false).AddParser("length_percent").GetId(),
			specification.RegisterProperty("edge-b", "", false, false).AddParser("length_percent").GetId(),
			specification.RegisterProperty("edge-l", "", false, false).AddParser("length_percent").GetId(),
		};
		id_constraint = specification.RegisterShorthand("edge-margin", "edge-t, edge-r, edge-b, edge-l", ShorthandType::Box);
	}

	bool Parse(const String& value, Array<NumericValue, 4>& out_constraints)
	{
		PropertyDictionary properties;
		if (!specification.ParseShorthandDeclaration(properties, id_constraint, value))
			return false;

		out_constraints = {};
		for (int i = 0; i < 4; i++)
		{
			if (const Property* p = properties.GetProperty(ids[i]))
				out_constraints[i] = p->GetNumericValue();
		}
		return true;
	}
};

ElementHandle::ElementHandle(const String& tag) : Element(tag), drag_start(0, 0)
{
	// Make sure we can be dragged!
	SetProperty(PropertyId::Drag, Property(Style::Drag::Drag));

	move_target = nullptr;
	size_target = nullptr;
	initialised = false;
}

ElementHandle::~ElementHandle() {}

void ElementHandle::OnAttributeChange(const ElementAttributes& changed_attributes)
{
	Element::OnAttributeChange(changed_attributes);

	// Reset initialised state if the move or size targets have changed.
	if (changed_attributes.find("move_target") != changed_attributes.end() || changed_attributes.find("size_target") != changed_attributes.end() ||
		changed_attributes.find("edge_margin") != changed_attributes.end())
	{
		initialised = false;
		move_target = nullptr;
		size_target = nullptr;
	}
}

void ElementHandle::ProcessDefaultAction(Event& event)
{
	Element::ProcessDefaultAction(event);

	if (event.GetTargetElement() == this)
	{
		if (!initialised && GetOwnerDocument())
		{
			const String move_target_name = GetAttribute<String>("move_target", "");
			if (!move_target_name.empty())
				move_target = GetElementById(move_target_name);

			const String size_target_name = GetAttribute<String>("size_target", "");
			if (!size_target_name.empty())
				size_target = GetElementById(size_target_name);

			const String edge_margin_str = GetAttribute<String>("edge_margin", "0px");
			edge_margin = {};
			if (edge_margin_str != "none")
			{
				HandleEdgeMarginParser parser;
				if (!parser.Parse(edge_margin_str, edge_margin))
					Log::Message(Log::LT_WARNING, "Failed to parse 'edge_margin' attribute for element '%s'.", GetAddress().c_str());
			}

			initialised = true;
		}

		if (event == EventId::Dragstart)
		{
			using namespace Style;
			Context* context = GetContext();
			drag_start = event.GetUnprojectedMouseScreenPos();
			drag_delta_min = {-FLT_MAX, -FLT_MAX};
			drag_delta_max = {FLT_MAX, FLT_MAX};

			if (move_target && context)
			{
				ElementHandleTargetData move_target_data(move_target, context, edge_margin);
				move_data = move_target_data.GetMoveData(drag_delta_min, drag_delta_max);
			}

			if (size_target && context)
			{
				ElementHandleTargetData size_target_data(size_target, context, edge_margin);
				size_data = size_target_data.GetSizeData(drag_delta_min, drag_delta_max);
			}

			drag_delta_min = Math::Min(drag_delta_min, Vector2f{0, 0});
			drag_delta_max = Math::Max(drag_delta_max, Vector2f{0, 0});
		}
		else if (event == EventId::Drag)
		{
			const Vector2f delta = Math::Clamp(event.GetUnprojectedMouseScreenPos() - drag_start, drag_delta_min, drag_delta_max);

			if (move_target)
			{
				const Vector2f new_position_top_left = (move_data.original_position_top_left + delta).Round();
				const Vector2f new_position_bottom_right = (move_data.original_position_bottom_right - delta).Round();

				if (move_data.top_left.x)
					move_target->SetProperty(PropertyId::Left, Property(new_position_top_left.x, Unit::PX));
				if (move_data.top_left.y)
					move_target->SetProperty(PropertyId::Top, Property(new_position_top_left.y, Unit::PX));
				if (move_data.bottom_right.x)
					move_target->SetProperty(PropertyId::Right, Property(new_position_bottom_right.x, Unit::PX));
				if (move_data.bottom_right.y)
					move_target->SetProperty(PropertyId::Bottom, Property(new_position_bottom_right.y, Unit::PX));
			}

			if (size_target)
			{
				const Vector2f new_size = Math::Max((size_data.original_size + delta).Round(), Vector2f(0.f));
				const Vector2f new_position_bottom_right = (size_data.original_position_bottom_right - delta).Round();

				if (size_data.width_height.x)
					size_target->SetProperty(PropertyId::Width, Property(new_size.x, Unit::PX));
				if (size_data.width_height.y)
					size_target->SetProperty(PropertyId::Height, Property(new_size.y, Unit::PX));
				if (size_data.bottom_right.x)
					size_target->SetProperty(PropertyId::Right, Property(new_position_bottom_right.x, Unit::PX));
				if (size_data.bottom_right.y)
					size_target->SetProperty(PropertyId::Bottom, Property(new_position_bottom_right.y, Unit::PX));
			}

			Dictionary parameters;
			parameters["handle_x"] = delta.x;
			parameters["handle_y"] = delta.y;
			DispatchEvent(EventId::Handledrag, parameters);
		}
	}
}

} // namespace Rml
