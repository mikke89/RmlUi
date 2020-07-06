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

#include "../../Include/RmlUi/Core/RenderInterface.h"
#include "TextureDatabase.h"

namespace Rml {

RenderInterface::RenderInterface()
{
	context = nullptr;
}

RenderInterface::~RenderInterface()
{
	TextureDatabase::ReleaseTextures(this);
}

// Called by RmlUi when it wants to compile geometry it believes will be static for the forseeable future.
CompiledGeometryHandle RenderInterface::CompileGeometry(Vertex* /*vertices*/, int /*num_vertices*/, int* /*indices*/, int /*num_indices*/, TextureHandle /*texture*/)
{
	return 0;
}

// Called by RmlUi when it wants to render application-compiled geometry.
void RenderInterface::RenderCompiledGeometry(CompiledGeometryHandle /*geometry*/, const Vector2f& /*translation*/)
{
}

// Called by RmlUi when it wants to release application-compiled geometry.
void RenderInterface::ReleaseCompiledGeometry(CompiledGeometryHandle /*geometry*/)
{
}

// Called by RmlUi when a texture is required by the library.
bool RenderInterface::LoadTexture(TextureHandle& /*texture_handle*/, Vector2i& /*texture_dimensions*/, const String& /*source*/)
{
	return false;
}

// Called by RmlUi when a texture is required to be built from an internally-generated sequence of pixels.
bool RenderInterface::GenerateTexture(TextureHandle& /*texture_handle*/, const byte* /*source*/, const Vector2i& /*source_dimensions*/)
{
	return false;
}

// Called by RmlUi when a loaded texture is no longer required.
void RenderInterface::ReleaseTexture(TextureHandle /*texture*/)
{
}

// Called by RmlUi when it wants to change the current transform matrix to a new matrix.
void RenderInterface::SetTransform(const Matrix4f* /*transform*/)
{
}

// Get the context currently being rendered.
Context* RenderInterface::GetContext() const
{
	return context;
}

} // namespace Rml
