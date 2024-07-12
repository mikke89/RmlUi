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

#ifndef RMLUI_CORE_CALLBACKTEXTURE_H
#define RMLUI_CORE_CALLBACKTEXTURE_H

#include "Header.h"
#include "Types.h"
#include "UniqueRenderResource.h"

namespace Rml {

class RenderInterface;
class RenderManager;
class CallbackTextureInterface;
class Texture;

/*
    Callback function for generating textures on demand.
    /// @param[in] texture_interface The interface used to specify the texture.
    /// @return True on success.
 */
using CallbackTextureFunction = Function<bool(const CallbackTextureInterface& texture_interface)>;

/**
    Callback texture is a unique render resource for generating textures on demand.

    It is constructed through the render manager.
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
    Interface handed to the texture callback function, which the client can use to submit a single texture.
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

	RenderManager& GetRenderManager() const;

private:
	RenderManager& render_manager;
	RenderInterface& render_interface;
	TextureHandle& texture_handle;
	Vector2i& dimensions;
};

/**
    Stores a texture callback function, which is used to generate and cache callback textures possibly for multiple render managers.
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
#endif
