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

#include "../../Include/RmlUi/Core/Texture.h"
#include "RenderManagerAccess.h"

namespace Rml {

Texture::Texture(RenderManager* render_manager, TextureFileIndex file_index) : render_manager(render_manager), file_index(file_index) {}

Texture::Texture(RenderManager* render_manager, StableVectorIndex callback_index) : render_manager(render_manager), callback_index(callback_index) {}

Vector2i Texture::GetDimensions() const
{
	if (file_index != TextureFileIndex::Invalid)
		return RenderManagerAccess::GetDimensions(render_manager, file_index);
	if (callback_index != StableVectorIndex::Invalid)
		return RenderManagerAccess::GetDimensions(render_manager, callback_index);
	return {};
}

Texture::operator bool() const
{
	return callback_index != StableVectorIndex::Invalid || file_index != TextureFileIndex::Invalid;
}

bool Texture::operator==(const Texture& other) const
{
	return render_manager == other.render_manager && file_index == other.file_index && callback_index == other.callback_index;
}

TextureSource::TextureSource(String source, String document_path) : source(std::move(source)), document_path(std::move(document_path)) {}

Texture TextureSource::GetTexture(RenderManager& render_manager) const
{
	Texture& texture = textures[&render_manager];
	if (!texture)
		texture = render_manager.LoadTexture(source, document_path);
	return texture;
}

const String& TextureSource::GetSource() const
{
	return source;
}

const String& TextureSource::GetDefinitionSource() const
{
	return document_path;
}

} // namespace Rml
