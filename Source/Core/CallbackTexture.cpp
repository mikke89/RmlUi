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

#include "../../Include/RmlUi/Core/CallbackTexture.h"
#include "../../Include/RmlUi/Core/Texture.h"
#include "RenderManagerAccess.h"

namespace Rml {

void CallbackTexture::Release()
{
	if (resource_handle != StableVectorIndex::Invalid)
	{
		RenderManagerAccess::ReleaseResource(render_manager, *this);
		Clear();
	}
}

Rml::CallbackTexture::operator Texture() const
{
	return Texture(render_manager, resource_handle);
}

CallbackTextureInterface::CallbackTextureInterface(RenderManager& render_manager, RenderInterface& render_interface, TextureHandle& texture_handle,
	Vector2i& dimensions) : render_manager(render_manager), render_interface(render_interface), texture_handle(texture_handle), dimensions(dimensions)
{}

bool CallbackTextureInterface::GenerateTexture(Span<const byte> source, Vector2i new_dimensions) const
{
	if (texture_handle)
	{
		RMLUI_ERRORMSG("Texture already set");
		return false;
	}
	texture_handle = render_interface.GenerateTexture(source, new_dimensions);
	if (texture_handle)
		dimensions = new_dimensions;
	return texture_handle != TextureHandle{};
}

void CallbackTextureInterface::SaveLayerAsTexture() const
{
	if (texture_handle)
	{
		RMLUI_ERRORMSG("Texture already set");
		return;
	}

	const Rectanglei region = render_manager.GetScissorRegion();
	if (!region.Valid())
	{
		RMLUI_ERRORMSG("Save layer as texture requires a scissor region to be set first");
		return;
	}

	texture_handle = render_interface.SaveLayerAsTexture();
	if (texture_handle)
		dimensions = region.Size();
}

RenderManager& CallbackTextureInterface::GetRenderManager() const
{
	return render_manager;
}

CallbackTextureSource::CallbackTextureSource(CallbackTextureFunction&& callback) : callback(std::move(callback)) {}

CallbackTextureSource::CallbackTextureSource(CallbackTextureSource&& other) noexcept :
	callback(std::move(other.callback)), textures(std::move(other.textures))
{}

CallbackTextureSource& CallbackTextureSource::operator=(CallbackTextureSource&& other) noexcept
{
	callback = std::move(other.callback);
	textures = std::move(other.textures);
	return *this;
}

Texture CallbackTextureSource::GetTexture(RenderManager& render_manager) const
{
	CallbackTexture& texture = textures[&render_manager];
	if (!texture)
		texture = render_manager.MakeCallbackTexture(callback);
	return Texture(texture);
}

} // namespace Rml
