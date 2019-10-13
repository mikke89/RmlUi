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

#include "precompiled.h"
#include "FontEffectOutlineInstancer.h"
#include "FontEffectOutline.h"

namespace Rml {
namespace Core {

FontEffectOutlineInstancer::FontEffectOutlineInstancer() : id_width(PropertyId::Invalid), id_color(PropertyId::Invalid)
{
	id_width = RegisterProperty("width", "1px", true).AddParser("length").GetId();
	id_color = RegisterProperty("color", "white", false).AddParser("color").GetId();
	RegisterShorthand("font-effect", "width, color", ShorthandType::FallThrough);
}

FontEffectOutlineInstancer::~FontEffectOutlineInstancer()
{
}

// Instances an outline font effect.
SharedPtr<FontEffect> FontEffectOutlineInstancer::InstanceFontEffect(const String& RMLUI_UNUSED_PARAMETER(name), const PropertyDictionary& properties)
{
	RMLUI_UNUSED(name);

	float width = properties.GetProperty(id_width)->Get< float >();
	Colourb color = properties.GetProperty(id_color)->Get< Colourb >();

	auto font_effect = std::make_shared<FontEffectOutline>();
	if (font_effect->Initialise(Math::RealToInteger(width)))
	{
		font_effect->SetColour(color);
		return font_effect;
	}

	return nullptr;
}

}
}
