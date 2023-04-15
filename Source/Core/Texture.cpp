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

#include "../../Include/RmlUi/Core/Texture.h"
#include "TextureDatabase.h"
#include "TextureResource.h"

namespace Rml {

void Texture::Set(const String& source, const String& source_path)
{
	resource = TextureDatabase::Fetch(source, source_path);
}

void Texture::Set(const String& name, const TextureCallback& callback)
{
	resource = MakeShared<TextureResource>();
	resource->Set(name, callback);
}

const String& Texture::GetSource() const
{
	static String empty_string;
	if (!resource)
		return empty_string;

	return resource->GetSource();
}

TextureHandle Texture::GetHandle() const
{
	if (!resource)
		return 0;

	return resource->GetHandle();
}

Vector2i Texture::GetDimensions() const
{
	if (!resource)
		return {};

	return resource->GetDimensions();
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
