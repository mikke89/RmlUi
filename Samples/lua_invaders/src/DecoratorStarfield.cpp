#include "DecoratorStarfield.h"
#include "GameDetails.h"
#include "Sprite.h"
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementUtilities.h>
#include <RmlUi/Core/Math.h>
#include <RmlUi/Core/PropertyDefinition.h>
#include <RmlUi/Core/SystemInterface.h>

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

Rml::DecoratorDataHandle DecoratorStarfield::GenerateElementData(Rml::Element* element, Rml::BoxArea /*paint_area*/) const
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
		Rml::ColourbPremultiplied color = star_field->star_layers[i].colour.ToPremultiplied();

		for (size_t j = 0; j < star_field->star_layers[i].stars.size(); j++)
		{
			const Rml::Vector2f position = star_field->star_layers[i].stars[j];
			points.push_back(ColoredPoint{color, position});
		}
	}

	if (Rml::RenderManager* render_manager = element->GetRenderManager())
		DrawPoints(*render_manager, point_size, points);
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

DecoratorInstancerStarfield::DecoratorInstancerStarfield()
{
	id_num_layers = RegisterProperty("num-layers", "3").AddParser("number").GetId();
	id_top_colour = RegisterProperty("top-colour", "#dddc").AddParser("color").GetId();
	id_bottom_colour = RegisterProperty("bottom-colour", "#333c").AddParser("color").GetId();
	id_top_speed = RegisterProperty("top-speed", "10.0").AddParser("number").GetId();
	id_bottom_speed = RegisterProperty("bottom-speed", "2.0").AddParser("number").GetId();
	id_top_density = RegisterProperty("top-density", "15").AddParser("number").GetId();
	id_bottom_density = RegisterProperty("bottom-density", "10").AddParser("number").GetId();
}

DecoratorInstancerStarfield::~DecoratorInstancerStarfield() {}

Rml::SharedPtr<Rml::Decorator> DecoratorInstancerStarfield::InstanceDecorator(const Rml::String& /*name*/, const Rml::PropertyDictionary& properties,
	const Rml::DecoratorInstancerInterface& /*instancer_interface*/)
{
	int num_layers = properties.GetProperty(id_num_layers)->Get<int>();
	Rml::Colourb top_colour = properties.GetProperty(id_top_colour)->Get<Rml::Colourb>();
	Rml::Colourb bottom_colour = properties.GetProperty(id_bottom_colour)->Get<Rml::Colourb>();
	float top_speed = properties.GetProperty(id_top_speed)->Get<float>();
	float bottom_speed = properties.GetProperty(id_bottom_speed)->Get<float>();
	int top_density = properties.GetProperty(id_top_density)->Get<int>();
	int bottom_density = properties.GetProperty(id_bottom_density)->Get<int>();

	auto decorator = Rml::MakeShared<DecoratorStarfield>();
	if (decorator->Initialise(num_layers, top_colour, bottom_colour, top_speed, bottom_speed, top_density, bottom_density))
		return decorator;

	return nullptr;
}
