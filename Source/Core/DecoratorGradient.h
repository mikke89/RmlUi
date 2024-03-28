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

#ifndef RMLUI_CORE_DECORATORGRADIENT_H
#define RMLUI_CORE_DECORATORGRADIENT_H

#include "../../Include/RmlUi/Core/DecorationTypes.h"
#include "../../Include/RmlUi/Core/Decorator.h"
#include "../../Include/RmlUi/Core/Geometry.h"
#include "../../Include/RmlUi/Core/ID.h"

namespace Rml {

using Vector2Numeric = Vector2<NumericValue>;

/**
    Straight gradient.

    CSS usage:
    decorator: horizontal-gradient( <start-color> <stop-color> );
    decorator: vertical-gradient( <start-color> <stop-color> );
 */
class DecoratorStraightGradient : public Decorator {
public:
	enum class Direction { Horizontal, Vertical };

	DecoratorStraightGradient();
	virtual ~DecoratorStraightGradient();

	bool Initialise(Direction direction, Colourb start, Colourb stop);

	DecoratorDataHandle GenerateElementData(Element* element, BoxArea paint_area) const override;
	void ReleaseElementData(DecoratorDataHandle element_data) const override;

	void RenderElement(Element* element, DecoratorDataHandle element_data) const override;

private:
	Direction direction = {};
	Colourb start, stop;
};

class DecoratorStraightGradientInstancer : public DecoratorInstancer {
public:
	DecoratorStraightGradientInstancer();
	virtual ~DecoratorStraightGradientInstancer();

	SharedPtr<Decorator> InstanceDecorator(const String& name, const PropertyDictionary& properties,
		const DecoratorInstancerInterface& instancer_interface) override;

private:
	struct PropertyIds {
		PropertyId direction, start, stop;
	};
	PropertyIds ids;
};

/**
    Linear gradient.
 */
class DecoratorLinearGradient : public Decorator {
public:
	enum class Corner { TopRight, BottomRight, BottomLeft, TopLeft, None, Count = None };

	DecoratorLinearGradient();
	virtual ~DecoratorLinearGradient();

	bool Initialise(bool repeating, Corner corner, float angle, const ColorStopList& color_stops);

	DecoratorDataHandle GenerateElementData(Element* element, BoxArea paint_area) const override;
	void ReleaseElementData(DecoratorDataHandle element_data) const override;

	void RenderElement(Element* element, DecoratorDataHandle element_data) const override;

private:
	struct LinearGradientShape {
		// Gradient line starting and ending points.
		Vector2f p0, p1;
		float length;
	};

	LinearGradientShape CalculateShape(Vector2f box_dimensions) const;

	bool repeating = false;
	Corner corner = Corner::None;
	float angle = 0.f;
	ColorStopList color_stops;
};

class DecoratorLinearGradientInstancer : public DecoratorInstancer {
public:
	DecoratorLinearGradientInstancer();
	virtual ~DecoratorLinearGradientInstancer();

	SharedPtr<Decorator> InstanceDecorator(const String& name, const PropertyDictionary& properties,
		const DecoratorInstancerInterface& instancer_interface) override;

private:
	enum class Direction {
		None = 0,
		Top = 1,
		Right = 2,
		Bottom = 4,
		Left = 8,
		TopLeft = Top | Left,
		TopRight = Top | Right,
		BottomRight = Bottom | Right,
		BottomLeft = Bottom | Left,
	};
	struct PropertyIds {
		PropertyId angle;
		PropertyId direction_to, direction_x, direction_y;
		PropertyId color_stop_list;
	};
	PropertyIds ids;
};

/**
    Radial gradient.
 */
class DecoratorRadialGradient : public Decorator {
public:
	enum class Shape { Circle, Ellipse, Unspecified };
	enum class SizeType { ClosestSide, FarthestSide, ClosestCorner, FarthestCorner, LengthPercentage };

	DecoratorRadialGradient();
	virtual ~DecoratorRadialGradient();

	bool Initialise(bool repeating, Shape shape, SizeType size_type, Vector2Numeric size, Vector2Numeric position, const ColorStopList& color_stops);

	DecoratorDataHandle GenerateElementData(Element* element, BoxArea paint_area) const override;
	void ReleaseElementData(DecoratorDataHandle element_data) const override;

	void RenderElement(Element* element, DecoratorDataHandle element_data) const override;

private:
	struct RadialGradientShape {
		Vector2f center, radius;
	};
	RadialGradientShape CalculateRadialGradientShape(Element* element, Vector2f dimensions) const;

	bool repeating = false;
	Shape shape = {};
	SizeType size_type = {};
	Vector2Numeric size;
	Vector2Numeric position;

	ColorStopList color_stops;
};

class DecoratorRadialGradientInstancer : public DecoratorInstancer {
public:
	DecoratorRadialGradientInstancer();
	virtual ~DecoratorRadialGradientInstancer();

	SharedPtr<Decorator> InstanceDecorator(const String& name, const PropertyDictionary& properties,
		const DecoratorInstancerInterface& instancer_interface) override;

private:
	struct GradientPropertyIds {
		PropertyId ending_shape;
		PropertyId size_x, size_y;
		PropertyId position_x, position_y;
		PropertyId color_stop_list;
	};
	GradientPropertyIds ids;
};

/**
    Conic gradient.
 */
class DecoratorConicGradient : public Decorator {
public:
	DecoratorConicGradient();
	virtual ~DecoratorConicGradient();

	bool Initialise(bool repeating, float angle, Vector2Numeric position, const ColorStopList& color_stops);

	DecoratorDataHandle GenerateElementData(Element* element, BoxArea paint_area) const override;
	void ReleaseElementData(DecoratorDataHandle element_data) const override;

	void RenderElement(Element* element, DecoratorDataHandle element_data) const override;

private:
	bool repeating = false;
	float angle = {};
	Vector2Numeric position;
	ColorStopList color_stops;
};

class DecoratorConicGradientInstancer : public DecoratorInstancer {
public:
	DecoratorConicGradientInstancer();
	virtual ~DecoratorConicGradientInstancer();

	SharedPtr<Decorator> InstanceDecorator(const String& name, const PropertyDictionary& properties,
		const DecoratorInstancerInterface& instancer_interface) override;

private:
	struct GradientPropertyIds {
		PropertyId angle;
		PropertyId position_x, position_y;
		PropertyId color_stop_list;
	};
	GradientPropertyIds ids;
};

} // namespace Rml
#endif
