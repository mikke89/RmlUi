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

#ifndef RMLUI_CORE_TEXTURE_H
#define RMLUI_CORE_TEXTURE_H

#include "Header.h"
#include "Types.h"

namespace Rml {

class CallbackTexture;
class RenderManager;

/**
    Texture is a simple view of either a file texture or a callback texture.

    It is constructed through the render manager. It can be freely copied, and does not own or release the underlying
    resource. The user is responsible for ensuring that the lifetime of the texture is valid.
 */
class RMLUICORE_API Texture {
public:
	Texture() = default;

	Vector2i GetDimensions() const;

	explicit operator bool() const;
	bool operator==(const Texture& other) const;

private:
	Texture(RenderManager* render_manager, TextureFileIndex file_index);
	Texture(RenderManager* render_manager, StableVectorIndex callback_index);

	RenderManager* render_manager = nullptr;
	TextureFileIndex file_index = TextureFileIndex::Invalid;
	StableVectorIndex callback_index = StableVectorIndex::Invalid;

	friend class RenderManager;
	friend class CallbackTexture;
};

/**
    Stores the file source for a texture, which is used to generate textures possibly for multiple render managers.
 */
class RMLUICORE_API TextureSource : NonCopyMoveable {
public:
	TextureSource() = default;
	TextureSource(String source, String document_path);

	Texture GetTexture(RenderManager& render_manager) const;

	const String& GetSource() const;
	const String& GetDefinitionSource() const;

private:
	String source;
	String document_path;
	mutable SmallUnorderedMap<RenderManager*, Texture> textures;
};

} // namespace Rml
#endif
