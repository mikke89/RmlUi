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

#ifndef RMLUICORETEXTURE_H
#define RMLUICORETEXTURE_H

#include "Header.h"
#include "Types.h"
#include <functional>

namespace Rml {
namespace Core {

class TextureResource;
class RenderInterface;

/*
	Callback function for generating textures.
	/// @param[in] name The name used to set the texture.
	/// @param[out] data The raw data of the texture, each pixel has four 8-bit channels: red-green-blue-alpha.
	/// @param[out] dimensions The width and height of the generated texture.
	/// @return True on success.
*/
using TextureCallback = std::function<bool(const String& name, UniquePtr<const byte[]>& data, Vector2i& dimensions)>;


/**
	Abstraction of a two-dimensional texture image, with an application-specific texture handle.

	@author Peter Curry
 */

struct RMLUICORE_API Texture
{
public:
	/// Set the texture source and path. The texture is added to the global cache and only loaded on first use.
	/// @param[in] source The source of the texture.
	/// @param[in] source_path The path of the resource that is requesting the texture (ie, the RCSS file in which it was specified, etc).
	void Set(const String& source, const String& source_path = "");

	/// Set a callback function for generating the texture on first use. The texture is never added to the global cache.
	/// @param[in] name The name of the texture.
	/// @param[in] callback The callback function which generates the data of the texture, see TextureCallback.
	void Set(const String& name, const TextureCallback& callback);

	/// Returns the texture's source name. This is usually the name of the file the texture was loaded from.
	/// @return The name of the this texture's source. This will be the empty string if this texture is not loaded.
	const String& GetSource() const;
	/// Returns the texture's handle.
	/// @param[in] The render interface that is requesting the handle.
	/// @return The texture's handle. This will be nullptr if the texture isn't loaded.
	TextureHandle GetHandle(RenderInterface* render_interface) const;
	/// Returns the texture's dimensions.
	/// @param[in] The render interface that is requesting the dimensions.
	/// @return The texture's dimensions. This will be (0, 0) if the texture isn't loaded.
	Vector2i GetDimensions(RenderInterface* render_interface) const;

	/// Returns true if the texture points to the same underlying resource.
	bool operator==(const Texture&) const;

	/// Returns true if the underlying resource is set.
	explicit operator bool() const;

private:
	SharedPtr<TextureResource> resource;
};

}
}

#endif
