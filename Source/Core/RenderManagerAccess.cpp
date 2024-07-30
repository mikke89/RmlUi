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

#include "RenderManagerAccess.h"
#include "../../Include/RmlUi/Core/Texture.h"
#include "TextureDatabase.h"

namespace Rml {

Vector2i RenderManagerAccess::GetDimensions(RenderManager* render_manager, TextureFileIndex texture)
{
	return render_manager->texture_database->file_database.GetDimensions(render_manager->render_interface, texture);
}

Vector2i RenderManagerAccess::GetDimensions(RenderManager* render_manager, StableVectorIndex callback_texture)
{
	return render_manager->texture_database->callback_database.GetDimensions(render_manager, render_manager->render_interface, callback_texture);
}

void RenderManagerAccess::Render(RenderManager* render_manager, const Geometry& geometry, Vector2f translation, Texture texture,
	const CompiledShader& shader)
{
	render_manager->Render(geometry, translation, texture, shader);
}

void RenderManagerAccess::GetTextureSourceList(RenderManager* render_manager, StringList& source_list)
{
	render_manager->GetTextureSourceList(source_list);
}

bool RenderManagerAccess::ReleaseTexture(RenderManager* render_manager, const String& texture_source)
{
	return render_manager->ReleaseTexture(texture_source);
}

void RenderManagerAccess::ReleaseAllTextures(RenderManager* render_manager)
{
	render_manager->ReleaseAllTextures();
}

void RenderManagerAccess::ReleaseAllCompiledGeometry(RenderManager* render_manager)
{
	render_manager->ReleaseAllCompiledGeometry();
}

} // namespace Rml
