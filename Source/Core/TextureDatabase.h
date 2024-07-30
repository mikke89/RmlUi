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

#include "../../Include/RmlUi/Core/CallbackTexture.h"
#include "../../Include/RmlUi/Core/StableVector.h"
#include "../../Include/RmlUi/Core/Types.h"

namespace Rml {

class RenderInterface;

class CallbackTextureDatabase : NonCopyMoveable {
public:
	CallbackTextureDatabase();
	~CallbackTextureDatabase();

	StableVectorIndex CreateTexture(CallbackTextureFunction&& callback);
	void ReleaseTexture(RenderInterface* render_interface, StableVectorIndex callback_index);

	Vector2i GetDimensions(RenderManager* render_manager, RenderInterface* render_interface, StableVectorIndex callback_index);
	TextureHandle GetHandle(RenderManager* render_manager, RenderInterface* render_interface, StableVectorIndex callback_index);

	size_t size() const;

	void ReleaseAllTextures(RenderInterface* render_interface);

private:
	struct CallbackTextureEntry {
		CallbackTextureFunction callback;
		TextureHandle texture_handle = {};
		Vector2i dimensions;
	};

	CallbackTextureEntry& EnsureLoaded(RenderManager* render_manager, RenderInterface* render_interface, StableVectorIndex callback_index);

	StableVector<CallbackTextureEntry> texture_list;
};

class FileTextureDatabase : NonCopyMoveable {
public:
	FileTextureDatabase();
	~FileTextureDatabase();

	TextureFileIndex LoadTexture(RenderInterface* render_interface, const String& source);

	TextureHandle GetHandle(RenderInterface* render_interface, TextureFileIndex index);
	Vector2i GetDimensions(RenderInterface* render_interface, TextureFileIndex index);

	void GetSourceList(StringList& source_list) const;

	bool ReleaseTexture(RenderInterface* render_interface, const String& source);
	void ReleaseAllTextures(RenderInterface* render_interface);

private:
	struct FileTextureEntry {
		TextureHandle texture_handle = {};
		Vector2i dimensions;
	};

	FileTextureEntry LoadTextureEntry(RenderInterface* render_interface, const String& source);
	FileTextureEntry& EnsureLoaded(RenderInterface* render_interface, TextureFileIndex index);

	Vector<FileTextureEntry> texture_list;
	UnorderedMap<String, TextureFileIndex> texture_map; // key: source, value: index into 'texture_list'
};

class TextureDatabase {
public:
	FileTextureDatabase file_database;
	CallbackTextureDatabase callback_database;
};

} // namespace Rml
#endif
