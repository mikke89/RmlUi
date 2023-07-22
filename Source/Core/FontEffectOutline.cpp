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

#include "FontEffectOutline.h"
#include "../../Include/RmlUi/Core/PropertyDefinition.h"

namespace Rml {

FontEffectOutline::FontEffectOutline()
{
	width = 0;
	SetLayer(Layer::Back);
}

FontEffectOutline::~FontEffectOutline() {}

bool FontEffectOutline::HasUniqueTexture() const
{
	return true;
}

bool FontEffectOutline::Initialise(int _width)
{
	if (_width <= 0)
		return false;

	width = _width;

	filter.Initialise(width, FilterOperation::Dilation);
	for (int x = -width; x <= width; ++x)
	{
		for (int y = -width; y <= width; ++y)
		{
			float weight = 1;

			float distance = Math::SquareRoot(float(x * x + y * y));
			if (distance > width)
			{
				weight = (width + 1) - distance;
				weight = Math::Max(weight, 0.0f);
			}

			filter[x + width][y + width] = weight;
		}
	}

	return true;
}

bool FontEffectOutline::GetGlyphMetrics(Vector2i& origin, Vector2i& dimensions, const FontGlyph& /*glyph*/) const
{
	if (dimensions.x * dimensions.y > 0)
	{
		origin.x -= width;
		origin.y -= width;

		dimensions.x += 2 * width;
		dimensions.y += 2 * width;

		return true;
	}

	return false;
}

void FontEffectOutline::GenerateGlyphTexture(byte* destination_data, const Vector2i destination_dimensions, int destination_stride,
	const FontGlyph& glyph) const
{
	filter.Run(destination_data, destination_dimensions, destination_stride, ColorFormat::RGBA8, glyph.bitmap_data, glyph.bitmap_dimensions,
		Vector2i(width), glyph.color_format);

	FillColorValuesFromAlpha(destination_data, destination_dimensions, destination_stride);
}

FontEffectOutlineInstancer::FontEffectOutlineInstancer() : id_width(PropertyId::Invalid), id_color(PropertyId::Invalid)
{
	id_width = RegisterProperty("width", "1px", true).AddParser("length").GetId();
	id_color = RegisterProperty("color", "white", false).AddParser("color").GetId();
	RegisterShorthand("font-effect", "width, color", ShorthandType::FallThrough);
}

FontEffectOutlineInstancer::~FontEffectOutlineInstancer() {}

SharedPtr<FontEffect> FontEffectOutlineInstancer::InstanceFontEffect(const String& /*name*/, const PropertyDictionary& properties)
{
	float width = properties.GetProperty(id_width)->Get<float>();
	Colourb color = properties.GetProperty(id_color)->Get<Colourb>();

	auto font_effect = MakeShared<FontEffectOutline>();
	if (font_effect->Initialise(int(width)))
	{
		font_effect->SetColour(color);
		return font_effect;
	}

	return nullptr;
}

} // namespace Rml
