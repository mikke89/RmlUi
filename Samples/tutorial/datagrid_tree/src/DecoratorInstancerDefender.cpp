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
#include "DecoratorDefender.h"

DecoratorInstancerDefender::DecoratorInstancerDefender()
{
	id_image_src = RegisterProperty("image-src", "").AddParser("string").GetId();
	RegisterShorthand("decorator", "image-src", Rml::Core::ShorthandType::FallThrough);
}

DecoratorInstancerDefender::~DecoratorInstancerDefender()
{
}

// Instances a decorator given the property tag and attributes from the RCSS file.
std::shared_ptr<Rml::Core::Decorator> DecoratorInstancerDefender::InstanceDecorator(const Rml::Core::String& RMLUI_UNUSED_PARAMETER(name), const Rml::Core::PropertyDictionary& properties, const Rml::Core::DecoratorInstancerInterface& interface)
{
	RMLUI_UNUSED(name);

	const Rml::Core::Property* image_source_property = properties.GetProperty(id_image_src);
	Rml::Core::String image_source = image_source_property->Get< Rml::Core::String >();
	Rml::Core::String source_path;
	if (auto & source = image_source_property->source)
		source_path = source->path;

	auto decorator = std::make_shared<DecoratorDefender>();
	if (decorator->Initialise(image_source, source_path))
		return decorator;

	return nullptr;
}