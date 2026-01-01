#pragma once

#include <RmlUi/Core/Texture.h>
#include <RmlUi/Core/Types.h>

class Sprite {
public:
	Sprite(const Rml::Vector2f& dimensions, const Rml::Vector2f& top_left_texcoord, const Rml::Vector2f& bottom_right_texcoord);
	~Sprite();

	void Render(Rml::RenderManager& render_manager, Rml::Vector2f position, float dp_ratio, Rml::ColourbPremultiplied color, Rml::Texture texture);

	Rml::Vector2f dimensions;
	Rml::Vector2f top_left_texcoord;
	Rml::Vector2f bottom_right_texcoord;
};

struct ColoredPoint {
	Rml::ColourbPremultiplied color;
	Rml::Vector2f position;
};
using ColoredPointList = Rml::Vector<ColoredPoint>;

void DrawPoints(Rml::RenderManager& render_manager, float point_size, const ColoredPointList& points);
