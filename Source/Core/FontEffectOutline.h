#pragma once

#include "../../Include/RmlUi/Core/ConvolutionFilter.h"
#include "../../Include/RmlUi/Core/FontEffect.h"
#include "../../Include/RmlUi/Core/FontEffectInstancer.h"

namespace Rml {

/**
    A concrete font effect for rendering outlines around text.
 */

class FontEffectOutline : public FontEffect {
public:
	FontEffectOutline();
	virtual ~FontEffectOutline();

	bool Initialise(int width);

	bool HasUniqueTexture() const override;

	bool GetGlyphMetrics(Vector2i& origin, Vector2i& dimensions, const FontGlyph& glyph) const override;

	void GenerateGlyphTexture(byte* destination_data, Vector2i destination_dimensions, int destination_stride, const FontGlyph& glyph) const override;

private:
	int width;
	ConvolutionFilter filter;
};

/**
    A concrete font effect instancer for the outline effect.
 */

class FontEffectOutlineInstancer : public FontEffectInstancer {
public:
	FontEffectOutlineInstancer();
	virtual ~FontEffectOutlineInstancer();

	SharedPtr<FontEffect> InstanceFontEffect(const String& name, const PropertyDictionary& properties) override;

private:
	PropertyId id_width, id_color;
};

} // namespace Rml
