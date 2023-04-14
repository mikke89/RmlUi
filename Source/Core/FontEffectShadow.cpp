/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019-2023 The RmlUi Team, and contributors
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

#include "FontEffectShadow.h"
#include "../../Include/RmlUi/Core/PropertyDefinition.h"

namespace Rml {

FontEffectShadow::FontEffectShadow() : offset(0, 0)
{
	SetLayer(Layer::Back);
}

FontEffectShadow::~FontEffectShadow() {}

bool FontEffectShadow::Initialise(const Vector2i _offset)
{
	offset = _offset;
	return true;
}

bool FontEffectShadow::HasUniqueTexture() const
{
	return false;
}

bool FontEffectShadow::GetGlyphMetrics(Vector2i& origin, Vector2i& /*dimensions*/, const FontGlyph& glyph) const
{
	if (glyph.color_format == ColorFormat::RGBA8)
		return false;

	origin += offset;
	return true;
}

FontEffectShadowInstancer::FontEffectShadowInstancer() :
	id_offset_x(PropertyId::Invalid), id_offset_y(PropertyId::Invalid), id_color(PropertyId::Invalid)
{
	id_offset_x = RegisterProperty("offset-x", "0px", true).AddParser("length").GetId();
	id_offset_y = RegisterProperty("offset-y", "0px", true).AddParser("length").GetId();
	id_color = RegisterProperty("color", "white", false).AddParser("color").GetId();
	RegisterShorthand("offset", "offset-x, offset-y", ShorthandType::FallThrough);
	RegisterShorthand("font-effect", "offset-x, offset-y, color", ShorthandType::FallThrough);
}

FontEffectShadowInstancer::~FontEffectShadowInstancer() {}

SharedPtr<FontEffect> FontEffectShadowInstancer::InstanceFontEffect(const String& /*name*/, const PropertyDictionary& properties)
{
	Vector2i offset;
	offset.x = properties.GetProperty(id_offset_x)->Get<int>();
	offset.y = properties.GetProperty(id_offset_y)->Get<int>();
	Colourb color = properties.GetProperty(id_color)->Get<Colourb>();

	auto font_effect = MakeShared<FontEffectShadow>();
	if (font_effect->Initialise(offset))
	{
		font_effect->SetColour(color);
		return font_effect;
	}

	return nullptr;
}

} // namespace Rml
