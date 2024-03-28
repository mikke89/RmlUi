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

#include "../../Include/RmlUi/Core/Decorator.h"
#include "../../Include/RmlUi/Core/PropertyDefinition.h"
#include "../../Include/RmlUi/Core/RenderManager.h"
#include "../../Include/RmlUi/Core/StyleSheet.h"
#include "../../Include/RmlUi/Core/Texture.h"
#include <algorithm>

namespace Rml {

Decorator::Decorator() {}

Decorator::~Decorator() {}

int Decorator::AddTexture(Texture texture)
{
	if (!texture)
		return -1;

	if (!first_texture)
		first_texture = texture;

	if (first_texture == texture)
		return 0;

	auto it = std::find(additional_textures.begin(), additional_textures.end(), texture);
	if (it != additional_textures.end())
		return (int)(it - additional_textures.begin()) + 1;

	additional_textures.push_back(texture);
	return (int)additional_textures.size();
}

int Decorator::GetNumTextures() const
{
	int result = (first_texture ? 1 : 0);
	result += (int)additional_textures.size();
	return result;
}

Texture Decorator::GetTexture(int index) const
{
	if (index == 0)
		return first_texture;

	index -= 1;
	if (index < 0 || index >= (int)additional_textures.size())
		return {};

	return additional_textures[index];
}

DecoratorInstancer::DecoratorInstancer() {}

DecoratorInstancer::~DecoratorInstancer() {}

const Sprite* DecoratorInstancerInterface::GetSprite(const String& name) const
{
	return style_sheet.GetSprite(name);
}

Texture DecoratorInstancerInterface::GetTexture(const String& filename) const
{
	if (!property_source)
	{
		Log::Message(Log::LT_WARNING, "Texture name '%s' in decorator could not be loaded, no property source available.", filename.c_str());
		return {};
	}

	return render_manager.LoadTexture(filename, property_source->path);
}

RenderManager& DecoratorInstancerInterface::GetRenderManager() const
{
	return render_manager;
}

} // namespace Rml
