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

#include "../../Include/RmlUi/Core/Geometry.h"
#include "../../Include/RmlUi/Core/Context.h"
#include "../../Include/RmlUi/Core/Core.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/Profiling.h"
#include "../../Include/RmlUi/Core/RenderInterface.h"
#include "GeometryDatabase.h"
#include <utility>


namespace Rml {
namespace Core {

Geometry::Geometry(Element* _host_element)
{
	database_handle = GeometryDatabase::Insert(this);
}

Geometry::Geometry(Context* _host_context)
{
	database_handle = GeometryDatabase::Insert(this);
}

Geometry::Geometry(Geometry&& other)
{
	MoveFrom(other);
	database_handle = GeometryDatabase::Insert(this);
}

Geometry& Geometry::operator=(Geometry&& other)
{
	MoveFrom(other);
	// Keep the database handles from construction unchanged, they are tied to the *this* pointer and should not change.
	return *this;
}

void Geometry::MoveFrom(Geometry& other)
{
	host_context = std::exchange(other.host_context, nullptr);
	host_element = std::exchange(other.host_element, nullptr);

	vertices = std::move(other.vertices);
	indices = std::move(other.indices);

	texture = std::exchange(other.texture, nullptr);

	compiled_geometry = std::exchange(other.compiled_geometry, 0);
	compile_attempted = std::exchange(other.compile_attempted, false);
}

Geometry::~Geometry()
{
	GeometryDatabase::Erase(database_handle);

	Release();
}

// Set the host element for this geometry; this should be passed in the constructor if possible.
void Geometry::SetHostElement(Element* _host_element)
{
	if (host_element == _host_element)
		return;

	if (host_element != nullptr)
	{
		Release();
		host_context = nullptr;
	}

	host_element = _host_element;
}

void Geometry::Render(const Vector2f& translation)
{
	RenderInterface* render_interface = GetRenderInterface();
	if (render_interface == nullptr)
		return;

	// Render our compiled geometry if possible.
	if (compiled_geometry)
	{
		RMLUI_ZoneScopedN("RenderCompiled");
		render_interface->RenderCompiledGeometry(compiled_geometry, translation);
	}
	// Otherwise, if we actually have geometry, try to compile it if we haven't already done so, otherwise render it in
	// immediate mode.
	else
	{
		if (vertices.empty() ||
			indices.empty())
			return;

		RMLUI_ZoneScopedN("RenderGeometry");

		if (!compile_attempted)
		{
			compile_attempted = true;
			compiled_geometry = render_interface->CompileGeometry(&vertices[0], (int) vertices.size(), &indices[0], (int) indices.size(), texture != nullptr ? texture->GetHandle(GetRenderInterface()) : 0);

			// If we managed to compile the geometry, we can clear the local copy of vertices and indices and
			// immediately render the compiled version.
			if (compiled_geometry)
			{	
				render_interface->RenderCompiledGeometry(compiled_geometry, translation);
				return;
			}
		}

		// Either we've attempted to compile before (and failed), or the compile we just attempted failed; either way,
		// render the uncompiled version.
		render_interface->RenderGeometry(&vertices[0], (int) vertices.size(), &indices[0], (int) indices.size(), texture != nullptr ? texture->GetHandle(GetRenderInterface()) : 0, translation);
	}
}

// Returns the geometry's vertices. If these are written to, Release() should be called to force a recompile.
std::vector< Vertex >& Geometry::GetVertices()
{
	return vertices;
}

// Returns the geometry's indices. If these are written to, Release() should be called to force a recompile.
std::vector< int >& Geometry::GetIndices()
{
	return indices;
}

// Gets the geometry's texture.
const Texture* Geometry::GetTexture() const
{
	return texture;
}

// Sets the geometry's texture.
void Geometry::SetTexture(const Texture* _texture)
{
	texture = _texture;
	Release();
}

void Geometry::Release(bool clear_buffers)
{
	if (compiled_geometry)
	{
		GetRenderInterface()->ReleaseCompiledGeometry(compiled_geometry);
		compiled_geometry = 0;
	}

	compile_attempted = false;

	if (clear_buffers)
	{
		vertices.clear();
		indices.clear();
	}
}

// Returns the host context's render interface.
RenderInterface* Geometry::GetRenderInterface()
{
	if (host_context == nullptr)
	{
		if (host_element != nullptr)
			host_context = host_element->GetContext();
	}

	if (host_context == nullptr)
		return Rml::Core::GetRenderInterface();
	else
		return host_context->GetRenderInterface();
}

}
}
