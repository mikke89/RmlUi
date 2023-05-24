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

#include "DecoratorStarfield.h"
#include "GameDetails.h"
#include "Sprite.h"
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementUtilities.h>
#include <RmlUi/Core/GeometryUtilities.h>
#include <RmlUi/Core/Math.h>
#include <RmlUi/Core/RenderInterface.h>
#include <RmlUi/Core/SystemInterface.h>
#include <Shell.h>

DecoratorStarfield::~DecoratorStarfield() {}

bool DecoratorStarfield::Initialise(int _num_layers, const Rml::Colourb& _top_colour, const Rml::Colourb& _bottom_colour, float _top_speed,
	float _bottom_speed, int _top_density, int _bottom_density)
{
	num_layers = _num_layers;
	top_colour = _top_colour;
	bottom_colour = _bottom_colour;
	top_speed = _top_speed;
	bottom_speed = _bottom_speed;
	top_density = _top_density;
	bottom_density = _bottom_density;

	return true;
}

Rml::DecoratorDataHandle DecoratorStarfield::GenerateElementData(Rml::Element* element) const
{
	const double t = Rml::GetSystemInterface()->GetElapsedTime();

	StarField* star_field = new StarField();
	star_field->star_layers.resize(num_layers);

	for (int i = 0; i < num_layers; i++)
	{
		float layer_depth = i / (float)num_layers;

		int density = int((top_density * layer_depth) + (bottom_density * (1.0f - layer_depth)));
		star_field->star_layers[i].stars.resize(density);

		Rml::Colourb colour = (top_colour * layer_depth) + (bottom_colour * (1.0f - layer_depth));
		star_field->star_layers[i].colour = colour;

		float speed = (top_speed * layer_depth) + (bottom_speed * (1.0f - layer_depth));
		star_field->star_layers[i].speed = speed;

		star_field->dimensions = element->GetBox().GetSize(Rml::BoxArea::Padding);

		if (star_field->dimensions.x > 0)
		{
			for (int j = 0; j < density; j++)
			{
				star_field->star_layers[i].stars[j].x = (float)Rml::Math::RandomReal(star_field->dimensions.x);
				star_field->star_layers[i].stars[j].y = (float)Rml::Math::RandomReal(star_field->dimensions.y);
			}
		}

		star_field->last_update = t;
	}

	return reinterpret_cast<Rml::DecoratorDataHandle>(star_field);
}

void DecoratorStarfield::ReleaseElementData(Rml::DecoratorDataHandle element_data) const
{
	delete reinterpret_cast<StarField*>(element_data);
}

void DecoratorStarfield::RenderElement(Rml::Element* element, Rml::DecoratorDataHandle element_data) const
{
	const double t = Rml::GetSystemInterface()->GetElapsedTime();

	StarField* star_field = reinterpret_cast<StarField*>(element_data);
	star_field->Update(t);

	const float dp_ratio = Rml::ElementUtilities::GetDensityIndependentPixelRatio(element);
	const float point_size = Rml::Math::RoundUp(2.f * dp_ratio);

	int num_stars = 0;

	for (size_t i = 0; i < star_field->star_layers.size(); i++)
		num_stars += (int)star_field->star_layers[i].stars.size();

	ColoredPointList points;
	points.reserve(num_stars);

	for (size_t i = 0; i < star_field->star_layers.size(); i++)
	{
		Rml::Colourb color = star_field->star_layers[i].colour;

		for (size_t j = 0; j < star_field->star_layers[i].stars.size(); j++)
		{
			const Rml::Vector2f position = star_field->star_layers[i].stars[j];
			points.push_back(ColoredPoint{color, position});
		}
	}

	DrawPoints(point_size, points);
}

void DecoratorStarfield::StarField::Update(double t)
{
	float delta_time = float(t - last_update);
	last_update = t;

	if (!GameDetails::GetPaused())
	{
		for (size_t i = 0; i < star_layers.size(); i++)
		{
			float movement = star_layers[i].speed * delta_time;

			for (size_t j = 0; j < star_layers[i].stars.size(); j++)
			{
				star_layers[i].stars[j].y += movement;
				if (star_layers[i].stars[j].y > dimensions.y)
				{
					star_layers[i].stars[j].y = 0;
					star_layers[i].stars[j].x = Rml::Math::RandomReal(dimensions.x);
				}
			}
		}
	}
}
