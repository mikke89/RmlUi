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

#include "../../Include/RmlUi/Core/FontEngineInterface.h"

namespace Rml {

FontEngineInterface::FontEngineInterface()
{
}

FontEngineInterface::~FontEngineInterface()
{
}

bool FontEngineInterface::LoadFontFace(const String& /*file_name*/, bool /*fallback_face*/)
{
	return false;
}

bool FontEngineInterface::LoadFontFace(const byte* /*data*/, int /*data_size*/, const String& /*font_family*/,
	Style::FontStyle /*style*/, Style::FontWeight /*weight*/, bool /*fallback_face*/)
{
	return false;
}

FontFaceHandle FontEngineInterface::GetFontFaceHandle(const String& /*family*/, Style::FontStyle /*style*/,
	Style::FontWeight /*weight*/, int /*size*/)
{
	return 0;
}
	
FontEffectsHandle FontEngineInterface::PrepareFontEffects(FontFaceHandle /*handle*/, const FontEffectList& /*font_effects*/)
{
	return 0;
}

int FontEngineInterface::GetSize(FontFaceHandle /*handle*/)
{
	return 0;
}

int FontEngineInterface::GetXHeight(FontFaceHandle /*handle*/)
{
	return 0;
}

int FontEngineInterface::GetLineHeight(FontFaceHandle /*handle*/)
{
	return 0;
}

int FontEngineInterface::GetBaseline(FontFaceHandle /*handle*/)
{
	return 0;
}

float FontEngineInterface::GetUnderline(FontFaceHandle /*handle*/, float& /*thickness*/)
{
	return 0;
}

int FontEngineInterface::GetStringWidth(FontFaceHandle /*handle*/, const String& /*string*/, Character /*prior_character*/)
{
	return 0;
}

int FontEngineInterface::GenerateString(FontFaceHandle /*face_handle*/, FontEffectsHandle /*font_effects_handle*/, const String& /*string*/,
	const Vector2f& /*position*/, const Colourb& /*colour*/, GeometryList& /*geometry*/)
{
	return 0;
}

int FontEngineInterface::GetVersion(FontFaceHandle /*handle*/)
{
	return 0;
}

} // namespace Rml
