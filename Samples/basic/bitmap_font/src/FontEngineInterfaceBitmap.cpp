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

#include "FontEngineInterfaceBitmap.h"
#include "FontEngineBitmap.h"
#include <RmlUi/Core.h>

void FontEngineInterfaceBitmap::Initialize()
{
	FontProviderBitmap::Initialise();
}

void FontEngineInterfaceBitmap::Shutdown()
{
	FontProviderBitmap::Shutdown();
}

bool FontEngineInterfaceBitmap::LoadFontFace(const String& file_name, bool /*fallback_face*/, FontWeight /*weight*/)
{
	return FontProviderBitmap::LoadFontFace(file_name);
}

bool FontEngineInterfaceBitmap::LoadFontFace(Span<const byte> /*data*/, const String& font_family, FontStyle /*style*/, FontWeight /*weight*/,
	bool /*fallback_face*/)
{
	// We return 'true' here to allow the debugger to continue loading, but we will use our own fonts when it asks for a handle.
	// The debugger might look a bit off with our own fonts, but hey it works.
	if (font_family == "rmlui-debugger-font")
		return true;

	return false;
}

FontFaceHandle FontEngineInterfaceBitmap::GetFontFaceHandle(const String& family, FontStyle style, FontWeight weight, int size)
{
	auto handle = FontProviderBitmap::GetFontFaceHandle(family, style, weight, size);
	return reinterpret_cast<FontFaceHandle>(handle);
}

FontEffectsHandle FontEngineInterfaceBitmap::PrepareFontEffects(FontFaceHandle /*handle*/, const FontEffectList& /*font_effects*/)
{
	// Font effects are not rendered in this implementation.
	return 0;
}

const FontMetrics& FontEngineInterfaceBitmap::GetFontMetrics(FontFaceHandle handle)
{
	auto handle_bitmap = reinterpret_cast<FontFaceBitmap*>(handle);
	return handle_bitmap->GetMetrics();
}

int FontEngineInterfaceBitmap::GetStringWidth(FontFaceHandle handle, StringView string, const TextShapingContext& /*text_shaping_context*/,
	Character prior_character)
{
	auto handle_bitmap = reinterpret_cast<FontFaceBitmap*>(handle);
	return handle_bitmap->GetStringWidth(string, prior_character);
}

int FontEngineInterfaceBitmap::GenerateString(RenderManager& render_manager, FontFaceHandle handle, FontEffectsHandle /*font_effects_handle*/,
	StringView string, Vector2f position, ColourbPremultiplied colour, float /*opacity*/, const TextShapingContext& /*text_shaping_context*/,
	TexturedMeshList& mesh_list)
{
	auto handle_bitmap = reinterpret_cast<FontFaceBitmap*>(handle);
	return handle_bitmap->GenerateString(render_manager, string, position, colour, mesh_list);
}

int FontEngineInterfaceBitmap::GetVersion(FontFaceHandle /*handle*/)
{
	return 0;
}
