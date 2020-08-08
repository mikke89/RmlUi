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

#ifndef DECORATORINSTANCERDEFENDER_H
#define DECORATORINSTANCERDEFENDER_H

#include <RmlUi/Core/DecoratorInstancer.h>

/**
	Decorator instancer for the Defender decorator.
	@author Robert Curry
 */

class DecoratorInstancerDefender : public Rml::DecoratorInstancer
{
public:
	DecoratorInstancerDefender();
	virtual ~DecoratorInstancerDefender();

	/// Instances a decorator given the property tag and attributes from the RCSS file.
	Rml::SharedPtr<Rml::Decorator> InstanceDecorator(const Rml::String& name, const Rml::PropertyDictionary& properties, const Rml::DecoratorInstancerInterface& instancer_interface) override;

private:
	Rml::PropertyId id_image_src;
};

#endif
