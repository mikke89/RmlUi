/*
 * Copyright (c) 2006 - 2008
 * Wandering Monster Studios Limited
 *
 * Any use of this program is governed by the terms of Wandering Monster
 * Studios Limited's Licence Agreement included with this program, a copy
 * of which can be obtained by contacting Wandering Monster Studios
 * Limited at info@wanderingmonster.co.nz.
 *
 */

#include "DecoratorInstancerDefender.h"
#include <Rocket/Core/Math.h>
#include <Rocket/Core/String.h>
#include "DecoratorDefender.h"

DecoratorInstancerDefender::DecoratorInstancerDefender()
{
	id_image_src = RegisterProperty("image-src", "").AddParser("string").GetId();
	RegisterShorthand("decorator", "image-src", Rocket::Core::ShorthandType::FallThrough);
}

DecoratorInstancerDefender::~DecoratorInstancerDefender()
{
}

// Instances a decorator given the property tag and attributes from the RCSS file.
Rocket::Core::Decorator* DecoratorInstancerDefender::InstanceDecorator(const Rocket::Core::String& ROCKET_UNUSED_PARAMETER(name), const Rocket::Core::PropertyDictionary& properties)
{
	ROCKET_UNUSED(name);

	const Rocket::Core::Property* image_source_property = properties.GetProperty(id_image_src);
	Rocket::Core::String image_source = image_source_property->Get< Rocket::Core::String >();

	DecoratorDefender* decorator = new DecoratorDefender();
	if (decorator->Initialise(image_source, image_source_property->source))
		return decorator;

	decorator->RemoveReference();
	ReleaseDecorator(decorator);
	return NULL;
}

// Releases the given decorator.
void DecoratorInstancerDefender::ReleaseDecorator(Rocket::Core::Decorator* decorator)
{
	delete decorator;
}

// Releases the instancer.
void DecoratorInstancerDefender::Release()
{
	delete this;
}
