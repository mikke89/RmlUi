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

#include "../../Include/RmlUi/Core/Texture.h"
#include "TextureDatabase.h"
#include "TextureResource.h"

namespace Rml {

// Attempts to load a texture.
void Texture::Set(const String& source, const String& source_path)
{
	resource = TextureDatabase::Fetch(source, source_path);
}

void Texture::Set(const String& name, const TextureCallback& callback)
{
	resource = MakeShared<TextureResource>();
	resource->Set(name, callback);
}

// Returns the texture's source name. This is usually the name of the file the texture was loaded from.
const String& Texture::GetSource() const
{
	static String empty_string;
	if (!resource)
		return empty_string;

	return resource->GetSource();
}

// Returns the texture's handle. 
TextureHandle Texture::GetHandle(RenderInterface* render_interface) const
{
	if (!resource)
		return 0;

	return resource->GetHandle(render_interface);
}

// Returns the texture's dimensions.
Vector2i Texture::GetDimensions(RenderInterface* render_interface) const
{
	if (!resource)
		return Vector2i(0, 0);

	return resource->GetDimensions(render_interface);
}

bool Texture::operator==(const Texture& other) const
{
	return resource == other.resource;
}

Texture::operator bool() const
{
	return static_cast<bool>(resource);
}

} // namespace Rml
