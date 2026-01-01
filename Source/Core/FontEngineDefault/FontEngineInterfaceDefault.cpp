#include "FontEngineInterfaceDefault.h"
#include "../../../Include/RmlUi/Core/StringUtilities.h"
#include "FontFaceHandleDefault.h"
#include "FontProvider.h"

namespace Rml {

void FontEngineInterfaceDefault::Initialize()
{
	FontProvider::Initialise();
}

void FontEngineInterfaceDefault::Shutdown()
{
	FontProvider::Shutdown();
}

bool FontEngineInterfaceDefault::LoadFontFace(const String& file_name, int face_index, bool fallback_face, Style::FontWeight weight)
{
	return FontProvider::LoadFontFace(file_name, face_index, fallback_face, weight);
}

bool FontEngineInterfaceDefault::LoadFontFace(Span<const byte> data, int face_index, const String& font_family, Style::FontStyle style, Style::FontWeight weight,
	bool fallback_face)
{
	return FontProvider::LoadFontFace(data, face_index, font_family, style, weight, fallback_face);
}

FontFaceHandle FontEngineInterfaceDefault::GetFontFaceHandle(const String& family, Style::FontStyle style, Style::FontWeight weight, int size)
{
	auto handle = FontProvider::GetFontFaceHandle(family, style, weight, size);
	return reinterpret_cast<FontFaceHandle>(handle);
}

FontEffectsHandle FontEngineInterfaceDefault::PrepareFontEffects(FontFaceHandle handle, const FontEffectList& font_effects)
{
	auto handle_default = reinterpret_cast<FontFaceHandleDefault*>(handle);
	return (FontEffectsHandle)handle_default->GenerateLayerConfiguration(font_effects);
}

const FontMetrics& FontEngineInterfaceDefault::GetFontMetrics(FontFaceHandle handle)
{
	auto handle_default = reinterpret_cast<FontFaceHandleDefault*>(handle);
	return handle_default->GetFontMetrics();
}

int FontEngineInterfaceDefault::GetStringWidth(FontFaceHandle handle, StringView string, const TextShapingContext& text_shaping_context,
	Character prior_character)
{
	auto handle_default = reinterpret_cast<FontFaceHandleDefault*>(handle);
	return handle_default->GetStringWidth(string, text_shaping_context, prior_character);
}

int FontEngineInterfaceDefault::GenerateString(RenderManager& render_manager, FontFaceHandle handle, FontEffectsHandle font_effects_handle,
	StringView string, Vector2f position, ColourbPremultiplied colour, float opacity, const TextShapingContext& text_shaping_context,
	TexturedMeshList& mesh_list)
{
	auto handle_default = reinterpret_cast<FontFaceHandleDefault*>(handle);
	return handle_default->GenerateString(render_manager, mesh_list, string, position, colour, opacity, text_shaping_context,
		(int)font_effects_handle);
}

int FontEngineInterfaceDefault::GetVersion(FontFaceHandle handle)
{
	auto handle_default = reinterpret_cast<FontFaceHandleDefault*>(handle);
	return handle_default->GetVersion();
}

void FontEngineInterfaceDefault::ReleaseFontResources()
{
	FontProvider::ReleaseFontResources();
}

} // namespace Rml
