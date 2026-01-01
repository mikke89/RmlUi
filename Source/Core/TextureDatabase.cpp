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
	if (!data.texture_handle && !data.load_failed)
	{
		if (!data.callback(CallbackTextureInterface(*render_manager, *render_interface, data.texture_handle, data.dimensions)))
		{
			data.load_failed = true;
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

TextureFileIndex FileTextureDatabase::InsertTexture(const String& source)
{
	auto it = texture_map.find(source);
	if (it != texture_map.end())
		return it->second;

	// The texture is not yet loaded from the render interface. That is deferred until the texture is needed, such as when it becomes visible.
	const auto index = TextureFileIndex(texture_list.size());
	texture_map[source] = index;
	texture_list.push_back({});

	return index;
}

FileTextureDatabase::FileTextureEntry FileTextureDatabase::LoadTextureEntry(RenderInterface* render_interface, const String& source)
{
	FileTextureEntry result = {};
	result.texture_handle = render_interface->LoadTexture(result.dimensions, source);
	if (!result.texture_handle)
	{
		result.load_texture_failed = true;
		Rml::Log::Message(Rml::Log::LT_WARNING, "Could not load texture: %s", source.c_str());
	}
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
		if (!entry.load_texture_failed)
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
		texture = {};
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
			texture = {};
		}
	}
}

} // namespace Rml
