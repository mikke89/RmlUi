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
