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

#ifndef RMLUI_CORE_TEXTUREDATABASE_H
#define RMLUI_CORE_TEXTUREDATABASE_H

#include "../../Include/RmlUi/Core/Types.h"

namespace Rml {

class TextureResource;

/**
    @author Peter Curry
 */

class TextureDatabase {
public:
	static void Initialise();
	static void Shutdown();

	/// Fetch a texture resource from file.
	/// The texture will be returned from the database if it already exists, otherwise a new
	/// entry will be added and returned.
	static SharedPtr<TextureResource> Fetch(const String& source, const String& source_directory);

	/// Release all textures in the database.
	static void ReleaseTextures();

	/// Release a given texture from the database.
	static bool ReleaseTexture(const String& source);

	/// Adds a texture resource with a callback function and stores it as a weak (raw) pointer in the database.
	static void AddCallbackTexture(TextureResource* texture);

	/// Removes a callback texture from the database.
	static void RemoveCallbackTexture(TextureResource* texture);

	/// Return a list of all texture sources currently in the database.
	static StringList GetSourceList();

	/// Returns true if there are no textures in the database yet to be released through the render interface.
	static bool AllTexturesReleased();

private:
	TextureDatabase();
	~TextureDatabase();

	using TextureMap = UnorderedMap<String, SharedPtr<TextureResource>>;
	TextureMap textures;

	using CallbackTextureMap = UnorderedSet<TextureResource*>;
	CallbackTextureMap callback_textures;
};

} // namespace Rml
#endif
