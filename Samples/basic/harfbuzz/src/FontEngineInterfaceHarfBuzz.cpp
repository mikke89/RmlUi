#include "FontEngineInterfaceHarfBuzz.h"
#include "FontFaceHandleHarfBuzz.h"
#include "FontProvider.h"
#include <RmlUi/Core.h>

void FontEngineInterfaceHarfBuzz::Initialize()
{
	FontProvider::Initialise();
}
void FontEngineInterfaceHarfBuzz::Shutdown()
{
	FontProvider::Shutdown();
}

bool FontEngineInterfaceHarfBuzz::LoadFontFace(const String& file_name, int face_index, bool fallback_face, Style::FontWeight weight)
{
	return FontProvider::LoadFontFace(file_name, face_index, fallback_face, weight);
}

bool FontEngineInterfaceHarfBuzz::LoadFontFace(Span<const byte> data, int face_index, const String& font_family, Style::FontStyle style,
	Style::FontWeight weight, bool fallback_face)
{
	return FontProvider::LoadFontFace(data, face_index, font_family, style, weight, fallback_face);
}

FontFaceHandle FontEngineInterfaceHarfBuzz::GetFontFaceHandle(const String& family, Style::FontStyle style, Style::FontWeight weight, int size)
{
	auto handle = FontProvider::GetFontFaceHandle(family, style, weight, size);
	return reinterpret_cast<FontFaceHandle>(handle);
}

FontEffectsHandle FontEngineInterfaceHarfBuzz::PrepareFontEffects(FontFaceHandle handle, const FontEffectList& font_effects)
{
	auto handle_harfbuzz = reinterpret_cast<FontFaceHandleHarfBuzz*>(handle);
	return (FontEffectsHandle)handle_harfbuzz->GenerateLayerConfiguration(font_effects);
}

const FontMetrics& FontEngineInterfaceHarfBuzz::GetFontMetrics(FontFaceHandle handle)
{
	auto handle_harfbuzz = reinterpret_cast<FontFaceHandleHarfBuzz*>(handle);
	return handle_harfbuzz->GetFontMetrics();
}

int FontEngineInterfaceHarfBuzz::GetStringWidth(FontFaceHandle handle, StringView string, const TextShapingContext& text_shaping_context,
	Character prior_character)
{
	auto handle_harfbuzz = reinterpret_cast<FontFaceHandleHarfBuzz*>(handle);
	return handle_harfbuzz->GetStringWidth(string, text_shaping_context, registered_languages, prior_character);
}

int FontEngineInterfaceHarfBuzz::GenerateString(RenderManager& render_manager, FontFaceHandle handle, FontEffectsHandle font_effects_handle,
	StringView string, Vector2f position, ColourbPremultiplied colour, float opacity, const TextShapingContext& text_shaping_context,
	TexturedMeshList& mesh_list)
{
	auto handle_harfbuzz = reinterpret_cast<FontFaceHandleHarfBuzz*>(handle);
	return handle_harfbuzz->GenerateString(render_manager, mesh_list, string, position, colour, opacity, text_shaping_context, registered_languages,
		(int)font_effects_handle);
}

int FontEngineInterfaceHarfBuzz::GetVersion(FontFaceHandle handle)
{
	auto handle_harfbuzz = reinterpret_cast<FontFaceHandleHarfBuzz*>(handle);
	return handle_harfbuzz->GetVersion();
}

void FontEngineInterfaceHarfBuzz::ReleaseFontResources()
{
	FontProvider::ReleaseFontResources();
}

void FontEngineInterfaceHarfBuzz::RegisterLanguage(const String& language_bcp47_code, const String& script_iso15924_code,
	const TextFlowDirection text_flow_direction)
{
	registered_languages[language_bcp47_code] = LanguageData{script_iso15924_code, text_flow_direction};
}
