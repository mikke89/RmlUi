/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019 The RmlUi Team, and contributors
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
