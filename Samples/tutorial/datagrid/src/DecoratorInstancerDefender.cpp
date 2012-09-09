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
#include <EMP/Core/Math.h>
#include <EMP/Core/String.h>
#include "DecoratorDefender.h"

DecoratorInstancerDefender::DecoratorInstancerDefender()
{
	RegisterProperty("image-src", "").AddParser("string");
}

DecoratorInstancerDefender::~DecoratorInstancerDefender()
{
}

// Instances a decorator given the property tag and attributes from the RCSS file.
Rocket::Core::Decorator* DecoratorInstancerDefender::InstanceDecorator(const EMP::Core::String& EMP_UNUSED(name), const Rocket::Core::PropertyDictionary& properties)
{
	const Rocket::Core::Property* image_source_property = properties.GetProperty("image-src");
	EMP::Core::String image_source = image_source_property->Get< EMP::Core::String >();

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
