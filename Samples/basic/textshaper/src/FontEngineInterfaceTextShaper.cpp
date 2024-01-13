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

#include "FontEngineInterfaceTextShaper.h"
#include "FontFaceHandleTextShaper.h"
#include "FontProvider.h"
#include <RmlUi/Core.h>

FontEngineInterfaceTextShaper::FontEngineInterfaceTextShaper()
{
	FontProvider::Initialise();
}

FontEngineInterfaceTextShaper::~FontEngineInterfaceTextShaper()
{
	FontProvider::Shutdown();
}

bool FontEngineInterfaceTextShaper::LoadFontFace(const String& file_name, bool /*fallback_face*/, Style::FontWeight weight)
{
	return FontProvider::LoadFontFace(file_name, weight);
}

bool FontEngineInterfaceTextShaper::LoadFontFace(const byte* data, int data_size, const String& font_family, Style::FontStyle style,
	Style::FontWeight weight, bool /*fallback_face*/)
{
	return FontProvider::LoadFontFace(data, data_size, font_family, style, weight);
}

FontFaceHandle FontEngineInterfaceTextShaper::GetFontFaceHandle(const String& family, Style::FontStyle style, Style::FontWeight weight, int size)
{
	auto handle = FontProvider::GetFontFaceHandle(family, style, weight, size);
	return reinterpret_cast<FontFaceHandle>(handle);
}

FontEffectsHandle FontEngineInterfaceTextShaper::PrepareFontEffects(FontFaceHandle handle, const FontEffectList& font_effects)
{
	auto handle_text_shaper = reinterpret_cast<FontFaceHandleTextShaper*>(handle);
	return (FontEffectsHandle)handle_text_shaper->GenerateLayerConfiguration(font_effects);
}

const FontMetrics& FontEngineInterfaceTextShaper::GetFontMetrics(FontFaceHandle handle)
{
	auto handle_text_shaper = reinterpret_cast<FontFaceHandleTextShaper*>(handle);
	return handle_text_shaper->GetFontMetrics();
}

int FontEngineInterfaceTextShaper::GetStringWidth(FontFaceHandle handle, const String& string, const TextShapingContext& text_shaping_context,
	Character prior_character)
{
	auto handle_text_shaper = reinterpret_cast<FontFaceHandleTextShaper*>(handle);
	return handle_text_shaper->GetStringWidth(string, text_shaping_context, registered_languages, prior_character);
}

int FontEngineInterfaceTextShaper::GenerateString(FontFaceHandle handle, FontEffectsHandle font_effects_handle, const String& string,
	const Vector2f& position, const Colourb& colour, float opacity, const TextShapingContext& text_shaping_context, GeometryList& geometry)
{
	auto handle_text_shaper = reinterpret_cast<FontFaceHandleTextShaper*>(handle);
	return handle_text_shaper->GenerateString(geometry, string, position, colour, opacity, text_shaping_context, registered_languages,
		(int)font_effects_handle);
}

int FontEngineInterfaceTextShaper::GetVersion(FontFaceHandle handle)
{
	auto handle_text_shaper = reinterpret_cast<FontFaceHandleTextShaper*>(handle);
	return handle_text_shaper->GetVersion();
}

void FontEngineInterfaceTextShaper::ReleaseFontResources()
{
	FontProvider::ReleaseFontResources();
}

void FontEngineInterfaceTextShaper::RegisterLanguage(const String& language_bcp47_code, const String& script_iso15924_code,
	const TextFlowDirection text_flow_direction)
{
	registered_languages[language_bcp47_code] = LanguageData{script_iso15924_code, text_flow_direction};
}
