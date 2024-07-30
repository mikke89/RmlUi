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

bool FontEngineInterfaceHarfBuzz::LoadFontFace(const String& file_name, bool fallback_face, Style::FontWeight weight)
{
	return FontProvider::LoadFontFace(file_name, fallback_face, weight);
}

bool FontEngineInterfaceHarfBuzz::LoadFontFace(Span<const byte> data, const String& font_family, Style::FontStyle style, Style::FontWeight weight,
	bool fallback_face)
{
	return FontProvider::LoadFontFace(data, font_family, style, weight, fallback_face);
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
