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

bool FontEngineInterfaceDefault::LoadFontFace(const String& file_name, bool fallback_face, Style::FontWeight weight)
{
	return FontProvider::LoadFontFace(file_name, fallback_face, weight);
}

bool FontEngineInterfaceDefault::LoadFontFace(Span<const byte> data, const String& font_family, Style::FontStyle style, Style::FontWeight weight,
	bool fallback_face)
{
	return FontProvider::LoadFontFace(data, font_family, style, weight, fallback_face);
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
	return handle_default->GetStringWidth(string, text_shaping_context.letter_spacing, prior_character);
}

int FontEngineInterfaceDefault::GenerateString(RenderManager& render_manager, FontFaceHandle handle, FontEffectsHandle font_effects_handle,
	StringView string, Vector2f position, ColourbPremultiplied colour, float opacity, const TextShapingContext& text_shaping_context,
	TexturedMeshList& mesh_list)
{
	auto handle_default = reinterpret_cast<FontFaceHandleDefault*>(handle);
	return handle_default->GenerateString(render_manager, mesh_list, string, position, colour, opacity, text_shaping_context.letter_spacing,
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
