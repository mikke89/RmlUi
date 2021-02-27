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
#include <RmlUi/Core/Types.h>
#include <RmlUi/Core/PropertyDefinition.h>
#include "DecoratorDefender.h"

DecoratorInstancerDefender::DecoratorInstancerDefender()
{
	id_image_src = RegisterProperty("image-src", "").AddParser("string").GetId();
	RegisterShorthand("decorator", "image-src", Rml::ShorthandType::FallThrough);
}

DecoratorInstancerDefender::~DecoratorInstancerDefender()
{
}

// Instances a decorator given the property tag and attributes from the RCSS file.
Rml::SharedPtr<Rml::Decorator> DecoratorInstancerDefender::InstanceDecorator(const Rml::String& /*name*/,
	const Rml::PropertyDictionary& properties, const Rml::DecoratorInstancerInterface& instancer_interface)
{
	const Rml::Property* image_source_property = properties.GetProperty(id_image_src);
	Rml::String image_source = image_source_property->Get< Rml::String >();
	Rml::Texture texture = instancer_interface.GetTexture(image_source);

	auto decorator = Rml::MakeShared<DecoratorDefender>();
	if (decorator->Initialise(texture))
		return decorator;

	return nullptr;
}
