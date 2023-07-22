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

#include "FontEffectGlow.h"
#include "../../Include/RmlUi/Core/PropertyDefinition.h"
#include "Memory.h"

namespace Rml {

FontEffectGlow::FontEffectGlow()
{
	width_blur = 0;
	width_outline = 0;
	combined_width = 0;
	SetLayer(Layer::Back);
}

FontEffectGlow::~FontEffectGlow() {}

bool FontEffectGlow::HasUniqueTexture() const
{
	return true;
}

bool FontEffectGlow::Initialise(int _width_outline, int _width_blur, Vector2i _offset)
{
	if (_width_outline < 0 || _width_blur < 0)
		return false;

	width_outline = _width_outline;
	width_blur = _width_blur;
	combined_width = width_blur + width_outline;
	offset = _offset;

	// Outline filter.
	filter_outline.Initialise(width_outline, FilterOperation::Dilation);
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
	const float std_dev = (width_blur == 0 ? 1.f : .4f * float(width_blur));
	const float two_variance = 2.f * std_dev * std_dev;
	const float gain = 1.f / Math::SquareRoot(Math::RMLUI_PI * two_variance);

	float sum_weight = 0.f;

	// We separate the blur filter into two passes, horizontal and vertical, for performance reasons.
	filter_blur_x.Initialise(Vector2i(width_blur, 0), FilterOperation::Sum);
	filter_blur_y.Initialise(Vector2i(0, width_blur), FilterOperation::Sum);

	for (int x = -width_blur; x <= width_blur; ++x)
	{
		float weight = gain * Math::Exp(-Math::SquareRoot(float(x * x) / two_variance));

		filter_blur_x[0][x + width_blur] = weight;
		filter_blur_y[x + width_blur][0] = weight;
		sum_weight += weight;
	}

	// Normalize the kernels
	for (int x = -width_blur; x <= width_blur; ++x)
	{
		filter_blur_x[0][x + width_blur] /= sum_weight;
		filter_blur_y[x + width_blur][0] /= sum_weight;
	}

	return true;
}

bool FontEffectGlow::GetGlyphMetrics(Vector2i& origin, Vector2i& dimensions, const FontGlyph& /*glyph*/) const
{
	if (dimensions.x * dimensions.y > 0)
	{
		origin.x += offset.x - combined_width;
		origin.y += offset.y - combined_width;

		dimensions.x += 2 * combined_width;
		dimensions.y += 2 * combined_width;

		return true;
	}

	return false;
}

void FontEffectGlow::GenerateGlyphTexture(byte* destination_data, const Vector2i destination_dimensions, int destination_stride,
	const FontGlyph& glyph) const
{
	const Vector2i buf_dimensions = destination_dimensions;
	const int buf_stride = buf_dimensions.x;
	const int buf_size = buf_dimensions.x * buf_dimensions.y;

	DynamicArray<byte, GlobalStackAllocator<byte>> outline_output(buf_size);
	DynamicArray<byte, GlobalStackAllocator<byte>> blur_x_output(buf_size);

	filter_outline.Run(outline_output.data(), buf_dimensions, buf_stride, ColorFormat::A8, glyph.bitmap_data, glyph.bitmap_dimensions,
		Vector2i(combined_width), glyph.color_format);

	filter_blur_x.Run(blur_x_output.data(), buf_dimensions, buf_stride, ColorFormat::A8, outline_output.data(), buf_dimensions, Vector2i(0),
		ColorFormat::A8);

	filter_blur_y.Run(destination_data, destination_dimensions, destination_stride, ColorFormat::RGBA8, blur_x_output.data(), buf_dimensions,
		Vector2i(0), ColorFormat::A8);

	FillColorValuesFromAlpha(destination_data, destination_dimensions, destination_stride);
}

FontEffectGlowInstancer::FontEffectGlowInstancer() :
	id_width_outline(PropertyId::Invalid), id_width_blur(PropertyId::Invalid), id_color(PropertyId::Invalid)
{
	id_width_outline = RegisterProperty("width-outline", "1px", true).AddParser("length").GetId();
	id_width_blur = RegisterProperty("width-blur", "-1px", true).AddParser("length").GetId();
	id_offset_x = RegisterProperty("offset-x", "0px", true).AddParser("length").GetId();
	id_offset_y = RegisterProperty("offset-y", "0px", true).AddParser("length").GetId();
	id_color = RegisterProperty("color", "white", false).AddParser("color").GetId();
	RegisterShorthand("font-effect", "width-outline, width-blur, offset-x, offset-y, color", ShorthandType::FallThrough);
}

FontEffectGlowInstancer::~FontEffectGlowInstancer() {}

SharedPtr<FontEffect> FontEffectGlowInstancer::InstanceFontEffect(const String& /*name*/, const PropertyDictionary& properties)
{
	Vector2i offset;
	int width_outline = properties.GetProperty(id_width_outline)->Get<int>();
	int width_blur = properties.GetProperty(id_width_blur)->Get<int>();
	offset.x = properties.GetProperty(id_offset_x)->Get<int>();
	offset.y = properties.GetProperty(id_offset_y)->Get<int>();
	Colourb color = properties.GetProperty(id_color)->Get<Colourb>();

	if (width_blur < 0)
		width_blur = width_outline;

	auto font_effect = MakeShared<FontEffectGlow>();
	if (font_effect->Initialise(width_outline, width_blur, offset))
	{
		font_effect->SetColour(color);
		return font_effect;
	}

	return nullptr;
}

} // namespace Rml
