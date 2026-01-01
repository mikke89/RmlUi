#pragma once

#include "../../Include/RmlUi/Core/ConvolutionFilter.h"
#include "../../Include/RmlUi/Core/FontEffect.h"
#include "../../Include/RmlUi/Core/FontEffectInstancer.h"

namespace Rml {

/**
    A font effect for rendering glow around text.

    Glow consists of an outline pass followed by a Gaussian blur pass.

 */

class FontEffectGlow : public FontEffect {
public:
	FontEffectGlow();
	virtual ~FontEffectGlow();

	bool Initialise(int width_outline, int width_blur, Vector2i offset);

	bool HasUniqueTexture() const override;

	bool GetGlyphMetrics(Vector2i& origin, Vector2i& dimensions, const FontGlyph& glyph) const override;

	void GenerateGlyphTexture(byte* destination_data, Vector2i destination_dimensions, int destination_stride, const FontGlyph& glyph) const override;

private:
	int width_outline, width_blur, combined_width;
	Vector2i offset;
	ConvolutionFilter filter_outline, filter_blur_x, filter_blur_y;
};

/**
    A concrete font effect instancer for the glow effect.
 */

class FontEffectGlowInstancer : public FontEffectInstancer {
public:
	FontEffectGlowInstancer();
	virtual ~FontEffectGlowInstancer();

	SharedPtr<FontEffect> InstanceFontEffect(const String& name, const PropertyDictionary& properties) override;

private:
	PropertyId id_width_outline, id_width_blur, id_offset_x, id_offset_y, id_color;
};

} // namespace Rml
