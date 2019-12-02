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
#include "FontEffectGlow.h"

namespace Rml {
namespace Core {

FontEffectGlow::FontEffectGlow()
{
	width_blur = 0;
	width_outline = 0;
	combined_width = 0;
	SetLayer(Layer::Back);
}

FontEffectGlow::~FontEffectGlow()
{
}

bool FontEffectGlow::HasUniqueTexture() const
{
	return true;
}

bool FontEffectGlow::Initialise(int _width_outline, int _width_blur)
{
	if (_width_outline <= 0 || _width_blur <= 0)
		return false;

	width_outline = _width_outline;
	width_blur = _width_blur;
	combined_width = width_blur + width_outline;

	// Outline filter
	// @performance: I think we can separate into horizontal and vertical pass?
	filter_outline.Initialise(width_outline, ConvolutionFilter::DILATION);
	for (int x = -width_outline; x <= width_outline; ++x)
	{
		for (int y = -width_outline; y <= width_outline; ++y)
		{
			float weight = 1;

			float distance = Math::SquareRoot(float(x * x + y * y));
			if (distance > width_outline)
			{
				weight = (width_outline + 1) - distance;
				weight = Math::Max(weight, 0.0f);
			}

			filter_outline[x + width_outline][y + width_outline] = weight;
		}
	}

	// Gaussian blur filter
	const float std_dev = .5f * float(width_blur);
	const float two_variance = 2.f * std_dev * std_dev;
	const float gain = 1.f / (Math::RMLUI_PI * two_variance);

	float sum_weight = 0.f;

	// @performance: Can separate into horizontal and vertical pass
	filter_blur.Initialise(width_blur, ConvolutionFilter::SUM);
	for (int x = -width_blur; x <= width_blur; ++x)
	{
		for (int y = -width_blur; y <= width_blur; ++y)
		{
			float weight = gain * Math::Exp(-Math::SquareRoot(float(x * x + y * y) / two_variance));

			filter_blur[x + width_blur][y + width_blur] = weight;
			sum_weight += weight;
		}
	}

	// Normalize the blur kernel
	for (int x = -width_blur; x <= width_blur; ++x)
		for (int y = -width_blur; y <= width_blur; ++y)
			filter_blur[x + width_blur][y + width_blur] /= sum_weight;


	return true;
}

bool FontEffectGlow::GetGlyphMetrics(Vector2i& origin, Vector2i& dimensions, const FontGlyph& RMLUI_UNUSED_PARAMETER(glyph)) const
{
	RMLUI_UNUSED(glyph);

	if (dimensions.x * dimensions.y > 0)
	{
		origin.x -= combined_width;
		origin.y -= combined_width;

		dimensions.x += combined_width;
		dimensions.y += combined_width;

		return true;
	}

	return false;
}

void FontEffectGlow::GenerateGlyphTexture(byte* destination_data, const Vector2i& destination_dimensions, int destination_stride, const FontGlyph& glyph) const
{
	const int buf_size = destination_dimensions.x * destination_dimensions.y * 4;
	byte* outline_output = new byte[buf_size];
	memset(outline_output, 0, buf_size);

	filter_outline.Run(outline_output, destination_dimensions, destination_dimensions.x * 4, glyph.bitmap_data, glyph.bitmap_dimensions, Vector2i(combined_width));

	for (int i = 0; i < buf_size / 4; i++)
		outline_output[i] = outline_output[i * 4 + 3];

	filter_blur.Run(destination_data, destination_dimensions, destination_stride, outline_output, destination_dimensions, Vector2i(0));

	delete[] outline_output;
}



FontEffectGlowInstancer::FontEffectGlowInstancer() : id_width_outline(PropertyId::Invalid), id_width_blur(PropertyId::Invalid),id_color(PropertyId::Invalid)
{
	id_width_outline = RegisterProperty("width-outline", "-1px", true).AddParser("length").GetId();
	id_width_blur = RegisterProperty("width-blur", "1px", true).AddParser("length").GetId();
	id_color = RegisterProperty("color", "white", false).AddParser("color").GetId();
	RegisterShorthand("font-effect", "width-outline, width-blur, color", ShorthandType::FallThrough);
}

FontEffectGlowInstancer::~FontEffectGlowInstancer()
{
}

SharedPtr<FontEffect> FontEffectGlowInstancer::InstanceFontEffect(const String& RMLUI_UNUSED_PARAMETER(name), const PropertyDictionary& properties)
{
	RMLUI_UNUSED(name);

	int width_outline = properties.GetProperty(id_width_outline)->Get< int >();
	int width_blur = properties.GetProperty(id_width_blur)->Get< int >();
	Colourb color = properties.GetProperty(id_color)->Get< Colourb >();

	if (width_blur < 0)
		width_blur = width_outline;

	auto font_effect = std::make_shared<FontEffectGlow>();
	if (font_effect->Initialise(width_blur, width_outline))
	{
		font_effect->SetColour(color);
		return font_effect;
	}

	return nullptr;
}

}
}
