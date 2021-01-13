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

#include "TextureResource.h"
#include "TextureDatabase.h"
#include "../../Include/RmlUi/Core/Log.h"
#include "../../Include/RmlUi/Core/RenderInterface.h"
#include "../../Include/RmlUi/Core/Profiling.h"

namespace Rml {

TextureResource::TextureResource()
{
}

TextureResource::~TextureResource()
{
	Reset();
}

void TextureResource::Set(const String& _source)
{
	Reset();
	source = _source;
}

void TextureResource::Set(const String& name, const TextureCallback& callback)
{
	Reset();
	source = name;
	texture_callback = MakeUnique<TextureCallback>(callback);
	TextureDatabase::AddCallbackTexture(this);
}

void TextureResource::Reset()
{
	Release();

	if (texture_callback)
	{
		TextureDatabase::RemoveCallbackTexture(this);
		texture_callback.reset();
	}

	source.clear();
}

// Returns the resource's underlying texture.
TextureHandle TextureResource::GetHandle(RenderInterface* render_interface)
{
	auto texture_iterator = texture_data.find(render_interface);
	if (texture_iterator == texture_data.end())
	{
		Load(render_interface);
		texture_iterator = texture_data.find(render_interface);
	}

	return texture_iterator->second.first;
}

// Returns the dimensions of the resource's texture.
Vector2i TextureResource::GetDimensions(RenderInterface* render_interface)
{
	auto texture_iterator = texture_data.find(render_interface);
	if (texture_iterator == texture_data.end())
	{
		Load(render_interface);
		texture_iterator = texture_data.find(render_interface);
	}

	return texture_iterator->second.second;
}

// Returns the resource's source.
const String& TextureResource::GetSource() const
{
	return source;
}

// Releases the texture's handle.
void TextureResource::Release(RenderInterface* render_interface)
{
	if (!render_interface)
	{
		for (auto& interface_data_pair : texture_data)
		{
			TextureHandle handle = interface_data_pair.second.first;
			if (handle)
				interface_data_pair.first->ReleaseTexture(handle);
		}

		texture_data.clear();
	}
	else
	{
		TextureDataMap::iterator texture_iterator = texture_data.find(render_interface);
		if (texture_iterator == texture_data.end())
			return;

		TextureHandle handle = texture_iterator->second.first;
		if (handle)
			texture_iterator->first->ReleaseTexture(handle);

		texture_data.erase(render_interface);
	}
}

bool TextureResource::Load(RenderInterface* render_interface)
{
	RMLUI_ZoneScoped;

	// Generate the texture from the callback function if we have one.
	if (texture_callback)
	{
		Vector2i dimensions;
		UniquePtr<const byte[]> data = nullptr;

		TextureCallback& callback_fnc = *texture_callback;

		if (!callback_fnc(source, data, dimensions) || !data)
		{
			Log::Message(Log::LT_WARNING, "Failed to generate texture from callback function %s.", source.c_str());
			texture_data[render_interface] = TextureData(0, Vector2i(0, 0));

			return false;
		}

		TextureHandle handle;
		bool success = render_interface->GenerateTexture(handle, data.get(), dimensions);

		if (success)
		{
			texture_data[render_interface] = TextureData(handle, dimensions);
		}
		else
		{
			Log::Message(Log::LT_WARNING, "Failed to generate internal texture %s.", source.c_str());
			texture_data[render_interface] = TextureData(0, Vector2i(0, 0));
		}

		return success;
	}

	// No callback function, load the texture through the render interface.
	TextureHandle handle;
	Vector2i dimensions;
	if (!render_interface->LoadTexture(handle, dimensions, source))
	{
		Log::Message(Log::LT_WARNING, "Failed to load texture from %s.", source.c_str());
		texture_data[render_interface] = TextureData(0, Vector2i(0, 0));

		return false;
	}

	texture_data[render_interface] = TextureData(handle, dimensions);
	return true;
}

} // namespace Rml
