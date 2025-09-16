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

#include "../../Include/RmlUi/Core/Geometry.h"
#include "RenderManagerAccess.h"

namespace Rml {

Geometry::Geometry(RenderManager* render_manager, StableVectorIndex resource_handle) : UniqueRenderResource(render_manager, resource_handle) {}

void Geometry::Render(Vector2f translation, Texture texture, const CompiledShader& shader) const
{
	if (resource_handle == StableVectorIndex::Invalid)
		return;

	translation = translation.Round();

	RenderManagerAccess::Render(render_manager, *this, translation, texture, shader);
}

Mesh Geometry::Release(ReleaseMode mode)
{
	if (resource_handle == StableVectorIndex::Invalid)
		return Mesh();

	Mesh mesh = RenderManagerAccess::ReleaseResource(render_manager, *this);
	Clear();
	if (mode == ReleaseMode::ClearMesh)
	{
		mesh.vertices.clear();
		mesh.indices.clear();
	}
	return mesh;
}

const Mesh& Geometry::GetMesh() const
{
	RMLUI_ASSERT(resource_handle != StableVectorIndex::Invalid);
	return RenderManagerAccess::GetMesh(render_manager, *this);
}

} // namespace Rml
