#include "FontEffectBlur.h"
#include "../../Include/RmlUi/Core/PropertyDefinition.h"
#include "Memory.h"

namespace Rml {

FontEffectBlur::FontEffectBlur()
{
	width = 0;
	SetLayer(Layer::Back);
}

FontEffectBlur::~FontEffectBlur() {}

bool FontEffectBlur::HasUniqueTexture() const
{
	return true;
}

bool FontEffectBlur::Initialise(int _width)
{
	if (_width <= 0)
		return false;

	width = _width;

	const float std_dev = .4f * float(width);
	const float two_variance = 2.f * std_dev * std_dev;
	const float gain = 1.f / Math::SquareRoot(Math::RMLUI_PI * two_variance);

	float sum_weight = 0.f;

	// We separate the blur filter into two passes, horizontal and vertical, for performance reasons.
	filter_x.Initialise(Vector2i(width, 0), FilterOperation::Sum);
	filter_y.Initialise(Vector2i(0, width), FilterOperation::Sum);

	for (int x = -width; x <= width; ++x)
	{
		float weight = gain * Math::Exp(-Math::SquareRoot(float(x * x) / two_variance));

		filter_x[0][x + width] = weight;
		filter_y[x + width][0] = weight;
		sum_weight += weight;
	}

	// Normalize the kernels
	for (int x = -width; x <= width; ++x)
	{
		filter_x[0][x + width] /= sum_weight;
		filter_y[x + width][0] /= sum_weight;
	}

	return true;
}

bool FontEffectBlur::GetGlyphMetrics(Vector2i& origin, Vector2i& dimensions, const FontGlyph& /*glyph*/) const
{
	if (dimensions.x * dimensions.y > 0)
	{
		origin.x -= width;
		origin.y -= width;

		dimensions.y += 2 * width;
		dimensions.x += 2 * width;

		return true;
	}

	return false;
}

void FontEffectBlur::GenerateGlyphTexture(byte* destination_data, const Vector2i destination_dimensions, int destination_stride,
	const FontGlyph& glyph) const
{
	const Vector2i buf_dimensions = destination_dimensions;
	const int buf_stride = buf_dimensions.x;
	const int buf_size = buf_dimensions.x * buf_dimensions.y;
	DynamicArray<byte, GlobalStackAllocator<byte>> x_output(buf_size);

	filter_x.Run(x_output.data(), buf_dimensions, buf_stride, ColorFormat::A8, glyph.bitmap_data, glyph.bitmap_dimensions, Vector2i(width),
		glyph.color_format);

	filter_y.Run(destination_data, destination_dimensions, destination_stride, ColorFormat::RGBA8, x_output.data(), buf_dimensions, Vector2i(0),
		ColorFormat::A8);

	FillColorValuesFromAlpha(destination_data, destination_dimensions, destination_stride);
}

FontEffectBlurInstancer::FontEffectBlurInstancer() : id_width(PropertyId::Invalid), id_color(PropertyId::Invalid)
{
	id_width = RegisterProperty("width", "1px", true).AddParser("length").GetId();
	id_color = RegisterProperty("color", "white", false).AddParser("color").GetId();
	RegisterShorthand("font-effect", "width, color", ShorthandType::FallThrough);
}

FontEffectBlurInstancer::~FontEffectBlurInstancer() {}

SharedPtr<FontEffect> FontEffectBlurInstancer::InstanceFontEffect(const String& /*name*/, const PropertyDictionary& properties)
{
	float width = properties.GetProperty(id_width)->Get<float>();
	Colourb color = properties.GetProperty(id_color)->Get<Colourb>();

	auto font_effect = MakeShared<FontEffectBlur>();
	if (font_effect->Initialise(int(width)))
	{
		font_effect->SetColour(color);
		return font_effect;
	}

	return nullptr;
}

} // namespace Rml
