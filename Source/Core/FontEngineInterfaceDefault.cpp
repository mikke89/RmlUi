/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019 The RmlUi Team, and contributors
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

#include "precompiled.h"
#include "FontDatabaseDefault.h"
#include "FontFaceHandleDefault.h"
#include "FontEngineInterfaceDefault.h"

namespace Rml {
namespace Core {

#ifndef RMLUI_NO_FONT_INTERFACE_DEFAULT

FontEngineInterfaceDefault::FontEngineInterfaceDefault()
{
	FontDatabaseDefault::Initialise();
}

FontEngineInterfaceDefault::~FontEngineInterfaceDefault()
{
	FontDatabaseDefault::Shutdown();
}

bool FontEngineInterfaceDefault::LoadFontFace(const String& file_name)
{
	return FontDatabaseDefault::LoadFontFace(file_name);
}

FontFaceHandle FontEngineInterfaceDefault::GetFontFaceHandle(const String& family,
	const String& charset, Style::FontStyle style, Style::FontWeight weight, int size)
{
	auto handle = FontDatabaseDefault::GetFontFaceHandle(family, charset, style, weight, size);
	return reinterpret_cast<FontFaceHandle>(handle.get());
}
	
int FontEngineInterfaceDefault::GenerateLayerConfiguration(FontFaceHandle handle, const FontEffectList& font_effects) const
{
	auto handle_default = reinterpret_cast<FontFaceHandleDefault *>(handle);
	return handle_default->GenerateLayerConfiguration(font_effects);
}

int FontEngineInterfaceDefault::GetCharacterWidth(FontFaceHandle handle) const
{
	auto handle_default = reinterpret_cast<FontFaceHandleDefault *>(handle);
	return handle_default->GetCharacterWidth();
}

int FontEngineInterfaceDefault::GetSize(FontFaceHandle handle) const
{
	auto handle_default = reinterpret_cast<FontFaceHandleDefault *>(handle);
	return handle_default->GetSize();
}

int FontEngineInterfaceDefault::GetXHeight(FontFaceHandle handle) const
{
	auto handle_default = reinterpret_cast<FontFaceHandleDefault *>(handle);
	return handle_default->GetXHeight();
}

int FontEngineInterfaceDefault::GetLineHeight(FontFaceHandle handle) const
{
	auto handle_default = reinterpret_cast<FontFaceHandleDefault *>(handle);
	return handle_default->GetLineHeight();
}

int FontEngineInterfaceDefault::GetBaseline(FontFaceHandle handle) const
{
	auto handle_default = reinterpret_cast<FontFaceHandleDefault *>(handle);
	return handle_default->GetBaseline();
}

float FontEngineInterfaceDefault::GetUnderline(FontFaceHandle handle, float *thickness) const
{
	auto handle_default = reinterpret_cast<FontFaceHandleDefault *>(handle);
	return handle_default->GetUnderline(thickness);
}

int FontEngineInterfaceDefault::GetStringWidth(FontFaceHandle handle, const WString& string, word prior_character)
{
	auto handle_default = reinterpret_cast<FontFaceHandleDefault *>(handle);
	return handle_default->GetStringWidth(string, prior_character);
}

int FontEngineInterfaceDefault::GenerateString(FontFaceHandle handle, GeometryList& geometry, const WString& string,
	const Vector2f& position, const Colourb& colour, int layer_configuration) const
{
	auto handle_default = reinterpret_cast<FontFaceHandleDefault *>(handle);
	return handle_default->GenerateString(geometry, string, position, colour, layer_configuration);
}

#endif

}
}
