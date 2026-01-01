#pragma once

#include "../../Include/RmlUi/Core/ConvolutionFilter.h"
#include "../../Include/RmlUi/Core/FontEffect.h"
#include "../../Include/RmlUi/Core/FontEffectInstancer.h"

namespace Rml {

/**
    A concrete font effect for rendering Gaussian blurred text.
 */

class FontEffectBlur : public FontEffect {
public:
	FontEffectBlur();
	virtual ~FontEffectBlur();

	bool Initialise(int width);

	bool HasUniqueTexture() const override;

	bool GetGlyphMetrics(Vector2i& origin, Vector2i& dimensions, const FontGlyph& glyph) const override;

	void GenerateGlyphTexture(byte* destination_data, Vector2i destination_dimensions, int destination_stride, const FontGlyph& glyph) const override;

private:
	int width;
	ConvolutionFilter filter_x, filter_y;
};

/**
    A concrete font effect instancer for the blur effect.
 */

class FontEffectBlurInstancer : public FontEffectInstancer {
public:
	FontEffectBlurInstancer();
	virtual ~FontEffectBlurInstancer();

	SharedPtr<FontEffect> InstanceFontEffect(const String& name, const PropertyDictionary& properties) override;

private:
	PropertyId id_width, id_color;
};

} // namespace Rml
