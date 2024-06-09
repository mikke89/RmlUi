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

#include "TextureDatabase.h"
#include "../../Include/RmlUi/Core/Log.h"
#include "../../Include/RmlUi/Core/RenderInterface.h"

namespace Rml {

CallbackTextureDatabase::CallbackTextureDatabase()
{
	constexpr size_t reserve_callback_textures = 30;
	texture_list.reserve(reserve_callback_textures);
}

CallbackTextureDatabase::~CallbackTextureDatabase()
{
	if (!texture_list.empty())
	{
		Log::Message(Log::LT_ERROR, "TextureDatabase destroyed with outstanding callback textures. Will likely result in memory corruption.");
		RMLUI_ERROR;
	}
}

StableVectorIndex CallbackTextureDatabase::CreateTexture(CallbackTextureFunction&& callback)
{
	RMLUI_ASSERT(callback);
	return texture_list.insert(CallbackTextureEntry{std::move(callback), TextureHandle(), Vector2i()});
}

void CallbackTextureDatabase::ReleaseTexture(RenderInterface* render_interface, StableVectorIndex callback_index)
{
	CallbackTextureEntry& data = texture_list[callback_index];
	if (data.texture_handle)
		render_interface->ReleaseTexture(data.texture_handle);
	texture_list.erase(callback_index);
}

Vector2i CallbackTextureDatabase::GetDimensions(RenderManager* render_manager, RenderInterface* render_interface, StableVectorIndex callback_index)
{
	return EnsureLoaded(render_manager, render_interface, callback_index).dimensions;
}

TextureHandle CallbackTextureDatabase::GetHandle(RenderManager* render_manager, RenderInterface* render_interface, StableVectorIndex callback_index)
{
	return EnsureLoaded(render_manager, render_interface, callback_index).texture_handle;
}

auto CallbackTextureDatabase::EnsureLoaded(RenderManager* render_manager, RenderInterface* render_interface, StableVectorIndex callback_index)
	-> CallbackTextureEntry&
{
	CallbackTextureEntry& data = texture_list[callback_index];
	if (!data.texture_handle)
	{
		if (!data.callback(CallbackTextureInterface(*render_manager, *render_interface, data.texture_handle, data.dimensions)))
		{
			data.texture_handle = {};
			data.dimensions = {};
		}
	}
	return data;
}

size_t CallbackTextureDatabase::size() const
{
	return texture_list.size();
}

void CallbackTextureDatabase::ReleaseAllTextures(RenderInterface* render_interface)
{
	texture_list.for_each([render_interface](CallbackTextureEntry& texture) {
		if (texture.texture_handle)
		{
			render_interface->ReleaseTexture(texture.texture_handle);
			texture.texture_handle = {};
			texture.dimensions = {};
		}
	});
}

FileTextureDatabase::FileTextureDatabase() {}

FileTextureDatabase::~FileTextureDatabase()
{
#ifdef RMLUI_DEBUG
	for (const FileTextureEntry& texture : texture_list)
	{
		RMLUI_ASSERTMSG(!texture.texture_handle,
			"TextureDatabase destroyed without releasing all file textures first. Ensure that 'ReleaseAllTextures' is called before destruction.");
	}
#endif
}

TextureFileIndex FileTextureDatabase::LoadTexture(RenderInterface* render_interface, const String& source)
{
	auto it = texture_map.find(source);
	if (it != texture_map.end())
		return it->second;

	FileTextureEntry entry = LoadTextureEntry(render_interface, source);
	if (!entry.texture_handle)
	{
		Rml::Log::Message(Rml::Log::LT_WARNING, "Could not load texture: %s", source.c_str());
		return TextureFileIndex::Invalid;
	}

	const auto index = TextureFileIndex(texture_list.size());
	texture_map[source] = index;
	texture_list.push_back(std::move(entry));

	return index;
}

FileTextureDatabase::FileTextureEntry FileTextureDatabase::LoadTextureEntry(RenderInterface* render_interface, const String& source)
{
	FileTextureEntry result = {};
	result.texture_handle = render_interface->LoadTexture(result.dimensions, source);
	return result;
}

FileTextureDatabase::FileTextureEntry& FileTextureDatabase::EnsureLoaded(RenderInterface* render_interface, TextureFileIndex index)
{
	FileTextureEntry& entry = texture_list[size_t(index)];
	if (!entry.texture_handle)
	{
		auto it = std::find_if(texture_map.begin(), texture_map.end(), [index](const auto& pair) { return pair.second == index; });
		RMLUI_ASSERT(it != texture_map.end());
		const String& source = it->first;
		entry = LoadTextureEntry(render_interface, source);
	}
	return entry;
}

TextureHandle FileTextureDatabase::GetHandle(RenderInterface* render_interface, TextureFileIndex index)
{
	RMLUI_ASSERT(size_t(index) < texture_list.size());
	return EnsureLoaded(render_interface, index).texture_handle;
}

Vector2i FileTextureDatabase::GetDimensions(RenderInterface* render_interface, TextureFileIndex index)
{
	RMLUI_ASSERT(size_t(index) < texture_list.size());
	return EnsureLoaded(render_interface, index).dimensions;
}

void FileTextureDatabase::GetSourceList(StringList& source_list) const
{
	source_list.reserve(source_list.size() + texture_list.size());
	for (const auto& texture : texture_map)
		source_list.push_back(texture.first);
}

bool FileTextureDatabase::ReleaseTexture(RenderInterface* render_interface, const String& source)
{
	auto it = texture_map.find(source);
	if (it == texture_map.end())
		return false;

	FileTextureEntry& texture = texture_list[size_t(it->second)];
	if (texture.texture_handle)
	{
		render_interface->ReleaseTexture(texture.texture_handle);
		texture.texture_handle = {};
		return true;
	}

	return false;
}

void FileTextureDatabase::ReleaseAllTextures(RenderInterface* render_interface)
{
	for (FileTextureEntry& texture : texture_list)
	{
		if (texture.texture_handle)
		{
			render_interface->ReleaseTexture(texture.texture_handle);
			texture.texture_handle = {};
		}
	}
}

} // namespace Rml
