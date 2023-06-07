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

#include "DecoratorGradient.h"
#include "../../Include/RmlUi/Core/ComputedValues.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/ElementUtilities.h"
#include "../../Include/RmlUi/Core/Geometry.h"
#include "../../Include/RmlUi/Core/GeometryUtilities.h"
#include "../../Include/RmlUi/Core/Math.h"
#include "../../Include/RmlUi/Core/PropertyDefinition.h"
#include "ComputeProperty.h"

namespace Rml {

// Returns the point along the input line ('line_point', 'line_vector') closest to the input 'point'.
static Vector2f IntersectionPointToLineNormal(const Vector2f point, const Vector2f line_point, const Vector2f line_vector)
{
	const Vector2f delta = line_point - point;
	return line_point - delta.DotProduct(line_vector) * line_vector;
}

/// Convert all color stop positions to normalized numbers.
/// @param[in] element The element to resolve lengths against.
/// @param[in] gradient_line_length The length of the gradient line, along which color stops are placed.
/// @param[in] soft_spacing The desired minimum distance between stops to avoid aliasing, in normalized number units.
/// @param[in] unresolved_stops
/// @return A list of resolved color stops, all in number units.
static ColorStopList ResolveColorStops(Element* element, const float gradient_line_length, const float soft_spacing,
	const ColorStopList& unresolved_stops)
{
	ColorStopList stops = unresolved_stops;
	const int num_stops = (int)stops.size();

	// Resolve all lengths, percentages, and angles to numbers. After this step all stops with a unit other than Number are considered as Auto.
	for (ColorStop& stop : stops)
	{
		if (Any(stop.position.unit & Unit::LENGTH))
		{
			const float resolved_position = element->ResolveLength(stop.position);
			stop.position = NumericValue(resolved_position / gradient_line_length, Unit::NUMBER);
		}
		else if (stop.position.unit == Unit::PERCENT)
		{
			stop.position = NumericValue(stop.position.number * 0.01f, Unit::NUMBER);
		}
		else if (Any(stop.position.unit & Unit::ANGLE))
		{
			stop.position = NumericValue(ComputeAngle(stop.position) * (1.f / (2.f * Math::RMLUI_PI)), Unit::NUMBER);
		}
	}

	// Resolve auto positions of the first and last color stops.
	auto resolve_edge_stop = [](ColorStop& stop, float auto_to_number) {
		if (stop.position.unit != Unit::NUMBER)
			stop.position = NumericValue(auto_to_number, Unit::NUMBER);
	};
	resolve_edge_stop(stops[0], 0.f);
	resolve_edge_stop(stops[num_stops - 1], 1.f);

	// Ensures that color stop positions are strictly increasing, and have at least 1px spacing to avoid aliasing.
	auto nudge_stop = [prev_position = stops[0].position.number](ColorStop& stop, bool update_prev = true) mutable {
		stop.position.number = Math::Max(stop.position.number, prev_position);
		if (update_prev)
			prev_position = stop.position.number;
	};
	int auto_begin_i = -1;

	// Evenly space stops with sequential auto indices, and nudge stop positions to ensure strictly increasing positions.
	for (int i = 1; i < num_stops; i++)
	{
		ColorStop& stop = stops[i];
		if (stop.position.unit != Unit::NUMBER)
		{
			// Mark the first of any consecutive auto stops.
			if (auto_begin_i < 0)
				auto_begin_i = i;
		}
		else if (auto_begin_i < 0)
		{
			// The stop has a definite position and there are no previous autos to handle, just ensure it is properly spaced.
			nudge_stop(stop);
		}
		else
		{
			// Space out all the previous auto stops, indices [auto_begin_i, i).
			nudge_stop(stop, false);
			const int num_auto_stops = i - auto_begin_i;
			const float t0 = stops[auto_begin_i - 1].position.number;
			const float t1 = stop.position.number;

			for (int j = 0; j < num_auto_stops; j++)
			{
				const float fraction_along_t0_t1 = float(j + 1) / float(num_auto_stops + 1);
				stops[j + auto_begin_i].position = NumericValue(t0 + (t1 - t0) * fraction_along_t0_t1, Unit::NUMBER);
				nudge_stop(stops[j + auto_begin_i]);
			}

			nudge_stop(stop);
			auto_begin_i = -1;
		}
	}

	// Ensures that stops are placed some minimum distance from each other to avoid aliasing, if possible.
	for (int i = 1; i < num_stops - 1; i++)
	{
		const float p0 = stops[i - 1].position.number;
		const float p1 = stops[i].position.number;
		const float p2 = stops[i + 1].position.number;
		float& new_position = stops[i].position.number;

		if (p1 - p0 < soft_spacing)
		{
			if (p2 - p0 < 2.f * soft_spacing)
				new_position = 0.5f * (p2 + p0);
			else
				new_position = p0 + soft_spacing;
		}
	}

	RMLUI_ASSERT(std::all_of(stops.begin(), stops.end(), [](auto&& stop) { return stop.position.unit == Unit::NUMBER; }));

	return stops;
}

DecoratorStraightGradient::DecoratorStraightGradient() {}

DecoratorStraightGradient::~DecoratorStraightGradient() {}

bool DecoratorStraightGradient::Initialise(const Direction in_direction, const Colourb in_start, const Colourb in_stop)
{
	direction = in_direction;
	start = in_start;
	stop = in_stop;
	return true;
}

DecoratorDataHandle DecoratorStraightGradient::GenerateElementData(Element* element, BoxArea paint_area) const
{
	Geometry* geometry = new Geometry();
	const Box& box = element->GetBox();

	const ComputedValues& computed = element->GetComputedValues();
	const float opacity = computed.opacity();

	GeometryUtilities::GenerateBackground(geometry, element->GetBox(), Vector2f(0), computed.border_radius(), Colourb(), paint_area);

	// Apply opacity
	Colourb colour_start = start;
	colour_start.alpha = (byte)(opacity * (float)colour_start.alpha);
	Colourb colour_stop = stop;
	colour_stop.alpha = (byte)(opacity * (float)colour_stop.alpha);

	const Vector2f offset = box.GetPosition(paint_area);
	const Vector2f size = box.GetSize(paint_area);

	Vector<Vertex>& vertices = geometry->GetVertices();

	if (direction == Direction::Horizontal)
	{
		for (int i = 0; i < (int)vertices.size(); i++)
		{
			const float t = Math::Clamp((vertices[i].position.x - offset.x) / size.x, 0.0f, 1.0f);
			vertices[i].colour = Math::RoundedLerp(t, colour_start, colour_stop);
		}
	}
	else if (direction == Direction::Vertical)
	{
		for (int i = 0; i < (int)vertices.size(); i++)
		{
			const float t = Math::Clamp((vertices[i].position.y - offset.y) / size.y, 0.0f, 1.0f);
			vertices[i].colour = Math::RoundedLerp(t, colour_start, colour_stop);
		}
	}

	return reinterpret_cast<DecoratorDataHandle>(geometry);
}

void DecoratorStraightGradient::ReleaseElementData(DecoratorDataHandle element_data) const
{
	delete reinterpret_cast<Geometry*>(element_data);
}

void DecoratorStraightGradient::RenderElement(Element* element, DecoratorDataHandle element_data) const
{
	auto* data = reinterpret_cast<Geometry*>(element_data);
	data->Render(element->GetAbsoluteOffset(BoxArea::Border));
}

DecoratorStraightGradientInstancer::DecoratorStraightGradientInstancer()
{
	ids.direction = RegisterProperty("direction", "horizontal").AddParser("keyword", "horizontal, vertical").GetId();
	ids.start = RegisterProperty("start-color", "#ffffff").AddParser("color").GetId();
	ids.stop = RegisterProperty("stop-color", "#ffffff").AddParser("color").GetId();
	RegisterShorthand("decorator", "direction, start-color, stop-color", ShorthandType::FallThrough);
}

DecoratorStraightGradientInstancer::~DecoratorStraightGradientInstancer() {}

SharedPtr<Decorator> DecoratorStraightGradientInstancer::InstanceDecorator(const String& name, const PropertyDictionary& properties_,
	const DecoratorInstancerInterface& /*interface_*/)
{
	using Direction = DecoratorStraightGradient::Direction;
	Direction direction;
	if (name == "horizontal-gradient")
		direction = Direction::Horizontal;
	else if (name == "vertical-gradient")
		direction = Direction::Vertical;
	else
	{
		direction = (Direction)properties_.GetProperty(ids.direction)->Get<int>();
		Log::Message(Log::LT_WARNING,
			"Decorator syntax 'gradient(horizontal|vertical ...)' is deprecated, please replace with 'horizontal-gradient(...)' or "
			"'vertical-gradient(...)'");
	}

	Colourb start = properties_.GetProperty(ids.start)->Get<Colourb>();
	Colourb stop = properties_.GetProperty(ids.stop)->Get<Colourb>();

	auto decorator = MakeShared<DecoratorStraightGradient>();
	if (decorator->Initialise(direction, start, stop))
		return decorator;

	return nullptr;
}

DecoratorLinearGradient::DecoratorLinearGradient() {}

DecoratorLinearGradient::~DecoratorLinearGradient() {}

bool DecoratorLinearGradient::Initialise(bool in_repeating, Corner in_corner, float in_angle, const ColorStopList& in_color_stops)
{
	repeating = in_repeating;
	corner = in_corner;
	angle = in_angle;
	color_stops = in_color_stops;
	return !color_stops.empty();
}

DecoratorDataHandle DecoratorLinearGradient::GenerateElementData(Element* element, BoxArea paint_area) const
{
	RenderInterface* render_interface = GetRenderInterface();
	if (!render_interface)
		return INVALID_DECORATORDATAHANDLE;

	RMLUI_ASSERT(!color_stops.empty());

	const Box& box = element->GetBox();
	const Vector2f dimensions = box.GetSize(paint_area);

	LinearGradientShape gradient_shape = CalculateShape(dimensions);

	// One-pixel minimum color stop spacing to avoid aliasing.
	const float soft_spacing = 1.f / gradient_shape.length;

	ColorStopList resolved_stops = ResolveColorStops(element, gradient_shape.length, soft_spacing, color_stops);

	auto element_data = MakeUnique<ElementData>();
	element_data->shader = render_interface->CompileShader("linear-gradient",
		Dictionary{
			{"angle", Variant(angle)},
			{"p0", Variant(gradient_shape.p0)},
			{"p1", Variant(gradient_shape.p1)},
			{"length", Variant(gradient_shape.length)},
			{"repeating", Variant(repeating)},
			{"color_stop_list", Variant(std::move(resolved_stops))},
		});

	if (!element_data->shader)
		return INVALID_DECORATORDATAHANDLE;

	Geometry& geometry = element_data->geometry;

	const ComputedValues& computed = element->GetComputedValues();
	const byte alpha = byte(computed.opacity() * 255.f);
	GeometryUtilities::GenerateBackground(&geometry, box, Vector2f(), computed.border_radius(), Colourb(255, alpha), paint_area);

	const Vector2f render_offset = box.GetPosition(paint_area);
	for (Vertex& vertex : geometry.GetVertices())
		vertex.tex_coord = vertex.position - render_offset;

	return reinterpret_cast<DecoratorDataHandle>(element_data.release());
}

void DecoratorLinearGradient::ReleaseElementData(DecoratorDataHandle handle) const
{
	ElementData* element_data = reinterpret_cast<ElementData*>(handle);
	GetRenderInterface()->ReleaseCompiledShader(element_data->shader);
	delete element_data;
}

void DecoratorLinearGradient::RenderElement(Element* element, DecoratorDataHandle handle) const
{
	ElementData* element_data = reinterpret_cast<ElementData*>(handle);
	element_data->geometry.RenderWithShader(element_data->shader, element->GetAbsoluteOffset(BoxArea::Border));
}

DecoratorLinearGradient::LinearGradientShape DecoratorLinearGradient::CalculateShape(Vector2f dim) const
{
	using uint = unsigned int;
	const Vector2f corners[(int)Corner::Count] = {Vector2f(dim.x, 0), dim, Vector2f(0, dim.y), Vector2f(0, 0)};
	const Vector2f center = 0.5f * dim;

	uint quadrant = 0;
	Vector2f line_vector;

	if (corner == Corner::None)
	{
		// Find the target quadrant and unit vector for the given angle.
		quadrant = uint(Math::NormaliseAngle(angle) * (4.f / (2.f * Math::RMLUI_PI))) % 4u;
		line_vector = Vector2f(Math::Sin(angle), -Math::Cos(angle));
	}
	else
	{
		// Quadrant given by the corner, need to find the vector perpendicular to the line connecting the neighboring corners.
		quadrant = uint(corner);
		const Vector2f v_neighbors = (corners[(quadrant + 1u) % 4u] - corners[(quadrant + 3u) % 4u]).Normalise();
		line_vector = {v_neighbors.y, -v_neighbors.x};
	}

	const uint quadrant_opposite = (quadrant + 2u) % 4u;

	const Vector2f starting_point = IntersectionPointToLineNormal(corners[quadrant_opposite], center, line_vector);
	const Vector2f ending_point = IntersectionPointToLineNormal(corners[quadrant], center, line_vector);

	const float length = Math::Absolute(dim.x * line_vector.x) + Math::Absolute(-dim.y * line_vector.y);

	return LinearGradientShape{starting_point, ending_point, length};
}

DecoratorLinearGradientInstancer::DecoratorLinearGradientInstancer()
{
	ids.angle = RegisterProperty("angle", "180deg").AddParser("angle").GetId();
	ids.direction_to = RegisterProperty("to", "unspecified").AddParser("keyword", "unspecified, to").GetId();
	// See Direction enum for keyword values.
	ids.direction_x = RegisterProperty("direction-x", "unspecified").AddParser("keyword", "unspecified=0, left=8, right=2").GetId();
	ids.direction_y = RegisterProperty("direction-y", "unspecified").AddParser("keyword", "unspecified=0, top=1, bottom=4").GetId();
	ids.color_stop_list = RegisterProperty("color-stops", "").AddParser("color_stop_list").GetId();

	RegisterShorthand("direction", "angle, to, direction-x, direction-y, direction-x", ShorthandType::FallThrough);
	RegisterShorthand("decorator", "direction?, color-stops#", ShorthandType::RecursiveCommaSeparated);
}

DecoratorLinearGradientInstancer::~DecoratorLinearGradientInstancer() {}

SharedPtr<Decorator> DecoratorLinearGradientInstancer::InstanceDecorator(const String& name, const PropertyDictionary& properties_,
	const DecoratorInstancerInterface& /*interface_*/)
{
	const Property* p_angle = properties_.GetProperty(ids.angle);
	const Property* p_direction_to = properties_.GetProperty(ids.direction_to);
	const Property* p_direction_x = properties_.GetProperty(ids.direction_x);
	const Property* p_direction_y = properties_.GetProperty(ids.direction_y);
	const Property* p_color_stop_list = properties_.GetProperty(ids.color_stop_list);

	if (!p_angle || !p_direction_to || !p_direction_x || !p_direction_y || !p_color_stop_list)
		return nullptr;

	using Corner = DecoratorLinearGradient::Corner;
	Corner corner = Corner::None;
	float angle = 0.f;

	if (p_direction_to->Get<bool>())
	{
		const Direction direction = (Direction)(p_direction_x->Get<int>() | p_direction_y->Get<int>());
		switch (direction)
		{
		case Direction::Top: angle = 0.f; break;
		case Direction::Right: angle = 0.5f * Math::RMLUI_PI; break;
		case Direction::Bottom: angle = Math::RMLUI_PI; break;
		case Direction::Left: angle = 1.5f * Math::RMLUI_PI; break;
		case Direction::TopLeft: corner = Corner::TopLeft; break;
		case Direction::TopRight: corner = Corner::TopRight; break;
		case Direction::BottomRight: corner = Corner::BottomRight; break;
		case Direction::BottomLeft: corner = Corner::BottomLeft; break;
		case Direction::None:
		default: return nullptr; break;
		}
	}
	else
	{
		angle = ComputeAngle(p_angle->GetNumericValue());
	}

	if (p_color_stop_list->unit != Unit::COLORSTOPLIST)
		return nullptr;
	const ColorStopList& color_stop_list = p_color_stop_list->value.GetReference<ColorStopList>();
	const bool repeating = (name == "repeating-linear-gradient");

	auto decorator = MakeShared<DecoratorLinearGradient>();
	if (decorator->Initialise(repeating, corner, angle, color_stop_list))
		return decorator;

	return nullptr;
}

} // namespace Rml
