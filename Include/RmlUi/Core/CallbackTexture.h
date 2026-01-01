#pragma once

#include "Header.h"
#include "Types.h"
#include "UniqueRenderResource.h"

namespace Rml {

class RenderInterface;
class RenderManager;
class CallbackTextureInterface;
class Texture;

/**
    Callback function for generating textures on demand.
    /// @param[in] texture_interface The interface used to specify the texture.
    /// @return True on success.
 */
using CallbackTextureFunction = Function<bool(const CallbackTextureInterface& texture_interface)>;

/**
    Callback texture is a unique render resource for generating textures on demand.

    Can be constructed through the render manager.
 */
class RMLUICORE_API CallbackTexture final : public UniqueRenderResource<CallbackTexture, StableVectorIndex, StableVectorIndex::Invalid> {
public:
	CallbackTexture() = default;

	operator Texture() const;

	void Release();

private:
	CallbackTexture(RenderManager* render_manager, StableVectorIndex resource_handle) : UniqueRenderResource(render_manager, resource_handle) {}
	friend class RenderManager;
};

/**
    Interface for generating a texture through the callback texture function.

    The client should submit a texture using one of the Generate/Save/Set functions exactly once during the callback.
 */
class RMLUICORE_API CallbackTextureInterface {
public:
	CallbackTextureInterface(RenderManager& render_manager, RenderInterface& render_interface, TextureHandle& texture_handle, Vector2i& dimensions);

	/// Generate texture from byte source.
	/// @param[in] source Texture data in 8-bit RGBA (premultiplied) format.
	/// @param[in] dimensions The width and height of the texture.
	/// @return True on success.
	bool GenerateTexture(Span<const byte> source, Vector2i dimensions) const;

	/// Store the current layer as a texture, so that it can be rendered with geometry later.
	/// @note The texture will be extracted using the bounds defined by the active scissor region, thereby matching its size.
	void SaveLayerAsTexture() const;

	/// Manually set the texture directly from a custom texture handle.
	/// @param[in] handle The handle that represents the texture.
	/// @param[in] dimensions The width and height of the texture.
	void SetTextureHandle(TextureHandle handle, Vector2i dimensions) const;

	RenderManager& GetRenderManager() const;

private:
	RenderManager& render_manager;
	RenderInterface& render_interface;
	TextureHandle& texture_handle;
	Vector2i& dimensions;
};

/**
    Stores a texture callback function.

    Used to generate and cache callback textures for one or more render managers.
 */
class RMLUICORE_API CallbackTextureSource {
public:
	CallbackTextureSource() = default;
	CallbackTextureSource(CallbackTextureFunction&& callback);
	~CallbackTextureSource() = default;

	CallbackTextureSource(const CallbackTextureSource&) = delete;
	CallbackTextureSource& operator=(const CallbackTextureSource&) = delete;

	CallbackTextureSource(CallbackTextureSource&& other) noexcept;
	CallbackTextureSource& operator=(CallbackTextureSource&& other) noexcept;

	Texture GetTexture(RenderManager& render_manager) const;

private:
	CallbackTextureFunction callback;
	mutable SmallUnorderedMap<RenderManager*, CallbackTexture> textures;
};

} // namespace Rml
