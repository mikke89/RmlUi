/*
 * This source file is part of libRocket, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://www.librocket.com
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
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
#include <Rocket/Core/Math.h>
#include "DecoratorInstancerStarfield.h"

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

DecoratorInstancerStarfield::~DecoratorInstancerStarfield()
{
}

// Instances a decorator given the property tag and attributes from the RCSS file.
std::shared_ptr<Rocket::Core::Decorator> DecoratorInstancerStarfield::InstanceDecorator(const Rocket::Core::String& ROCKET_UNUSED_PARAMETER(name), const Rocket::Core::PropertyDictionary& properties, const Rocket::Core::StyleSheet& style_sheet)
{
	ROCKET_UNUSED(name);

	int num_layers = Rocket::Core::Math::RealToInteger(properties.GetProperty(id_num_layers)->Get< float >());
	Rocket::Core::Colourb top_colour = properties.GetProperty(id_top_colour)->Get< Rocket::Core::Colourb >();
	Rocket::Core::Colourb bottom_colour = properties.GetProperty(id_bottom_colour)->Get< Rocket::Core::Colourb >();
	float top_speed = properties.GetProperty(id_top_speed)->Get< float >();
	float bottom_speed = properties.GetProperty(id_bottom_speed)->Get< float >();
	int top_density = Rocket::Core::Math::RealToInteger(properties.GetProperty(id_top_density)->Get< float >());
	int bottom_density = Rocket::Core::Math::RealToInteger(properties.GetProperty(id_bottom_density)->Get< float >());

	auto decorator = std::make_shared<DecoratorStarfield>();
	if (decorator->Initialise(num_layers, top_colour, bottom_colour, top_speed, bottom_speed, top_density, bottom_density))
		return decorator;

	return nullptr;
}
