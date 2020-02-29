/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019 The RmlUi Team, and contributors
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

#ifndef RMLUICORETEXTURERESOURCE_H
#define RMLUICORETEXTURERESOURCE_H

#include "../../Include/RmlUi/Core/Traits.h"
#include "../../Include/RmlUi/Core/Texture.h"

namespace Rml {
namespace Core {

/**
	A texture resource stores application-generated texture data (handle and dimensions) for each
	unique render interface that needs to render the data. It is used through a Texture object.

	@author Peter Curry
 */

class TextureResource : public NonCopyMoveable
{
public:
	TextureResource();
	~TextureResource();

	/// Clear any existing data and set the source path. Texture loading is delayed until the texture is
	/// accessed by a specific render interface.
	void Set(const String& source);

	/// Clear any existing data and set a callback function for loading the data. Texture loading is
	/// delayed until the texture is accessed by a specific render interface.
	void Set(const String& name, const TextureCallback& callback);

	/// Returns the resource's underlying texture handle.
	TextureHandle GetHandle(RenderInterface* render_interface);
	/// Returns the dimensions of the resource's texture.
	const Vector2i& GetDimensions(RenderInterface* render_interface);

	/// Returns the resource's source.
	const String& GetSource() const;

	/// Releases the texture's handle.
	void Release(RenderInterface* render_interface = nullptr);

protected:
	/// Attempts to load the texture from the source, or the callback function if set.
	bool Load(RenderInterface* render_interface);

private:
	String source;

	using TextureData = std::pair< TextureHandle, Vector2i >;
	using TextureDataMap = SmallUnorderedMap< RenderInterface*, TextureData >;
	TextureDataMap texture_data;

	UniquePtr<TextureCallback> texture_callback;
};

}
}

#endif
