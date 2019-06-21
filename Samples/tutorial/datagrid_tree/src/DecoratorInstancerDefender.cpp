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
#include <RmlUi/Core/Math.h>
#include <RmlUi/Core/String.h>
#include "DecoratorDefender.h"

DecoratorInstancerDefender::DecoratorInstancerDefender()
{
	RegisterProperty("image-src", "").AddParser("string");
}

DecoratorInstancerDefender::~DecoratorInstancerDefender()
{
}

// Instances a decorator given the property tag and attributes from the RCSS file.
Rml::Core::Decorator* DecoratorInstancerDefender::InstanceDecorator(const Rml::Core::String& RMLUI_UNUSED_PARAMETER(name), const Rml::Core::PropertyDictionary& properties)
{
	RMLUI_UNUSED(name);

	const Rml::Core::Property* image_source_property = properties.GetProperty("image-src");
	Rml::Core::String image_source = image_source_property->Get< Rml::Core::String >();

	DecoratorDefender* decorator = new DecoratorDefender();
	if (decorator->Initialise(image_source, image_source_property->source))
		return decorator;

	decorator->RemoveReference();
	ReleaseDecorator(decorator);
	return NULL;
}

// Releases the given decorator.
void DecoratorInstancerDefender::ReleaseDecorator(Rml::Core::Decorator* decorator)
{
	delete decorator;
}

// Releases the instancer.
void DecoratorInstancerDefender::Release()
{
	delete this;
}
