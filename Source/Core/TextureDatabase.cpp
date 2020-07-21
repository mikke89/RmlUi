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

#include "TextureDatabase.h"
#include "TextureResource.h"
#include "../../Include/RmlUi/Core/Core.h"
#include "../../Include/RmlUi/Core/StringUtilities.h"
#include "../../Include/RmlUi/Core/SystemInterface.h"

namespace Rml {

static TextureDatabase* texture_database = nullptr;

TextureDatabase::TextureDatabase()
{
	RMLUI_ASSERT(texture_database == nullptr);
	texture_database = this;
}

TextureDatabase::~TextureDatabase()
{
	RMLUI_ASSERT(texture_database == this);
	texture_database = nullptr;
}

void TextureDatabase::Initialise()
{
	new TextureDatabase();
}

void TextureDatabase::Shutdown()
{
	delete texture_database;
}

// If the requested texture is already in the database, it will be returned with an extra reference count. If not, it
// will be loaded through the application's render interface.
SharedPtr<TextureResource> TextureDatabase::Fetch(const String& source, const String& source_directory)
{
	String path;
	if (source.size() > 0 && source[0] == '?')
		path = source;
	else
		GetSystemInterface()->JoinPath(path, StringUtilities::Replace(source_directory, '|', ':'), source);

	TextureMap::iterator iterator = texture_database->textures.find(path);
	if (iterator != texture_database->textures.end())
	{
		return iterator->second;
	}

	auto resource = MakeShared<TextureResource>();
	resource->Set(path);

	texture_database->textures[resource->GetSource()] = resource;
	return resource;
}

void TextureDatabase::AddCallbackTexture(TextureResource* texture)
{
	if (texture_database)
		texture_database->callback_textures.insert(texture);
}

void TextureDatabase::RemoveCallbackTexture(TextureResource* texture)
{
	if (texture_database)
		texture_database->callback_textures.erase(texture);
}

void TextureDatabase::ReleaseTextures(RenderInterface* render_interface)
{
	if (texture_database)
	{
		for (const auto& texture : texture_database->textures)
			texture.second->Release(render_interface);

		for (const auto& texture : texture_database->callback_textures)
			texture->Release(render_interface);
	}
}

} // namespace Rml
