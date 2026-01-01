#include "../../Include/RmlUi/Core/FontEffect.h"
#include "../../Include/RmlUi/Core/FontEffectInstancer.h"

namespace Rml {

FontEffect::FontEffect() : layer(Layer::Back), colour(255, 255, 255), fingerprint(0) {}

FontEffect::~FontEffect() {}

bool FontEffect::HasUniqueTexture() const
{
	return false;
}

bool FontEffect::GetGlyphMetrics(Vector2i& /*origin*/, Vector2i& /*dimensions*/, const FontGlyph& /*glyph*/) const
{
	return false;
}

void FontEffect::GenerateGlyphTexture(byte* /*destination_data*/, Vector2i /*destination_dimensions*/, int /*destination_stride*/,
	const FontGlyph& /*glyph*/) const
{}

void FontEffect::SetColour(const Colourb _colour)
{
	colour = _colour;
}

Colourb FontEffect::GetColour() const
{
	return colour;
}

FontEffect::Layer FontEffect::GetLayer() const
{
	return layer;
}

void FontEffect::SetLayer(Layer _layer)
{
	layer = _layer;
}

size_t FontEffect::GetFingerprint() const
{
	return fingerprint;
}

void FontEffect::SetFingerprint(size_t _fingerprint)
{
	fingerprint = _fingerprint;
}

void FontEffect::FillColorValuesFromAlpha(byte* destination, Vector2i dimensions, int stride)
{
	for (int y = 0; y < dimensions.y; ++y)
	{
		for (int x = 0; x < dimensions.x; ++x)
		{
			const int i = y * stride + x * 4;
			const byte alpha = destination[i + 3];
			destination[i + 0] = destination[i + 1] = destination[i + 2] = alpha;
		}
	}
}

} // namespace Rml
