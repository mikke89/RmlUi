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

#ifndef RMLUI_CORE_RENDERMANAGERACCESS_H
#define RMLUI_CORE_RENDERMANAGERACCESS_H

#include "../../Include/RmlUi/Core/Core.h"
#include "../../Include/RmlUi/Core/RenderManager.h"
#include "../../Include/RmlUi/Core/Types.h"

namespace Rml {

class CompiledFilter;
class CompiledShader;
class CallbackTexture;
class Geometry;
class Texture;

class RenderManagerAccess {
private:
	template <typename T>
	static auto ReleaseResource(RenderManager* render_manager, T& resource)
	{
		return render_manager->ReleaseResource(resource);
	}

	static Vector2i GetDimensions(RenderManager* render_manager, TextureFileIndex texture);
	static Vector2i GetDimensions(RenderManager* render_manager, StableVectorIndex callback_texture);

	static void Render(RenderManager* render_manager, const Geometry& geometry, Vector2f translation, Texture texture, const CompiledShader& shader);

	static void GetTextureSourceList(RenderManager* render_manager, StringList& source_list);

	static bool ReleaseTexture(RenderManager* render_manager, const String& texture_source);
	static void ReleaseAllTextures(RenderManager* render_manager);
	static void ReleaseAllCompiledGeometry(RenderManager* render_manager);

	friend class CompiledFilter;
	friend class CompiledShader;
	friend class CallbackTexture;
	friend class Geometry;
	friend class Texture;

	friend StringList Rml::GetTextureSourceList();
	friend bool Rml::ReleaseTexture(const String&, RenderInterface*);
	friend void Rml::ReleaseTextures(RenderInterface*);
	friend void Rml::ReleaseCompiledGeometry(RenderInterface*);
};

} // namespace Rml

#endif
