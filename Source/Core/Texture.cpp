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
