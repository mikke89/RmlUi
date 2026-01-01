#pragma once

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
