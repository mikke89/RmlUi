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

#include "../../Include/RmlUi/Core/RenderInterfaceCompatibility.h"
#include "../../Include/RmlUi/Core/Math.h"

namespace Rml {

static void UnPremultiplyAlpha(const byte* source, byte* destination)
{
	const byte alpha = source[3];
	destination[0] = (alpha > 0 ? (source[0] * 255) / alpha : 255);
	destination[1] = (alpha > 0 ? (source[1] * 255) / alpha : 255);
	destination[2] = (alpha > 0 ? (source[2] * 255) / alpha : 255);
	destination[3] = alpha;
}

RenderInterfaceCompatibility::RenderInterfaceCompatibility() : adapter(new RenderInterfaceAdapter(*this)) {}

RenderInterfaceCompatibility::~RenderInterfaceCompatibility() {}

CompiledGeometryHandle RenderInterfaceCompatibility::CompileGeometry(Vertex* /*vertices*/, int /*num_vertices*/, int* /*indices*/,
	int /*num_indices*/, TextureHandle /*texture*/)
{
	return 0;
}

void RenderInterfaceCompatibility::RenderCompiledGeometry(CompiledGeometryHandle /*geometry*/, const Vector2f& /*translation*/) {}

void RenderInterfaceCompatibility::ReleaseCompiledGeometry(CompiledGeometryHandle /*geometry*/) {}

bool RenderInterfaceCompatibility::LoadTexture(TextureHandle& /*texture_handle*/, Vector2i& /*texture_dimensions*/, const String& /*source*/)
{
	return false;
}

bool RenderInterfaceCompatibility::GenerateTexture(TextureHandle& /*texture_handle*/, const byte* /*source*/, const Vector2i& /*source_dimensions*/)
{
	return false;
}

void RenderInterfaceCompatibility::ReleaseTexture(TextureHandle /*texture*/) {}

void RenderInterfaceCompatibility::SetTransform(const Matrix4f* /*transform*/) {}

RenderInterface* RenderInterfaceCompatibility::GetAdaptedInterface()
{
	return static_cast<RenderInterface*>(adapter.get());
}

RenderInterfaceAdapter::RenderInterfaceAdapter(RenderInterfaceCompatibility& legacy) : legacy(legacy) {}

CompiledGeometryHandle RenderInterfaceAdapter::CompileGeometry(Span<const Vertex> vertices, Span<const int> indices)
{
	// Previously, vertex colors were given in unpremultipled alpha, while now they are given in premultiplied alpha. If
	// not corrected for, transparent colors may look darker than they should with the legacy renderer. Thus, here we
	// make such a conversion.
	//
	// When upgrading your renderer, it is strongly recommended to convert your pipeline to use premultiplied alpha,
	// both to avoid copying vertex data like here and to achieve correct blending results.
	//
	// Note that, the vertices and indices are now guaranteed to be valid and immutable until the call to
	// ReleaseGeometry. Thus, it is possible to avoid copying the data even if you need access to it during the render
	// call. However, (1) due to the need to modify the vertices, we need to make a copy of them here. And (2), due to a
	// limitation in the legacy render interface, vertices and indices were previously submitted as pointers to mutable
	// vertices and indices. They were never intended to be mutable, but to avoid a const_cast we need to copy both of
	// them for that reason too.

	Vector<Vertex> vertices_unpremultiplied(vertices.begin(), vertices.end());
	for (size_t i = 0; i < vertices.size(); i++)
	{
		UnPremultiplyAlpha(vertices[i].colour, vertices_unpremultiplied[i].colour);
	}

	Vector<int> indices_copy(indices.begin(), indices.end());

	AdaptedGeometry* data = new AdaptedGeometry{std::move(vertices_unpremultiplied), std::move(indices_copy), {}};
	return reinterpret_cast<Rml::CompiledGeometryHandle>(data);
}

void RenderInterfaceAdapter::RenderGeometry(CompiledGeometryHandle handle, Vector2f translation, TextureHandle texture)
{
	AdaptedGeometry* geometry = reinterpret_cast<AdaptedGeometry*>(handle);

	// Textures were previously stored with the compiled geometry, but is now instead submitted during rendering.
	LegacyCompiledGeometryHandle& legacy_geometry = geometry->textures[texture];
	if (!legacy_geometry)
	{
		legacy_geometry = legacy.CompileGeometry(geometry->vertices.data(), (int)geometry->vertices.size(), geometry->indices.data(),
			(int)geometry->indices.size(), texture);
	}

	// If the legacy renderer supports compiling, use that, otherwise render the geometry in immediate mode.
	if (legacy_geometry)
	{
		legacy.RenderCompiledGeometry(legacy_geometry, translation);
	}
	else
	{
		legacy.RenderGeometry(geometry->vertices.data(), (int)geometry->vertices.size(), geometry->indices.data(), (int)geometry->indices.size(),
			texture, translation);
	}
}

void RenderInterfaceAdapter::ReleaseGeometry(CompiledGeometryHandle handle)
{
	AdaptedGeometry* geometry = reinterpret_cast<AdaptedGeometry*>(handle);
	for (auto& pair : geometry->textures)
		legacy.ReleaseCompiledGeometry(pair.second);

	delete reinterpret_cast<AdaptedGeometry*>(geometry);
}

void RenderInterfaceAdapter::EnableScissorRegion(bool enable)
{
	legacy.EnableScissorRegion(enable);
}

void RenderInterfaceAdapter::SetScissorRegion(Rectanglei region)
{
	legacy.SetScissorRegion(region.Left(), region.Top(), region.Width(), region.Height());
}

void RenderInterfaceAdapter::EnableClipMask(bool enable)
{
	legacy.EnableScissorRegion(enable);
}

void RenderInterfaceAdapter::RenderToClipMask(ClipMaskOperation operation, CompiledGeometryHandle handle, Vector2f translation)
{
	switch (operation)
	{
	case ClipMaskOperation::Set:
	case ClipMaskOperation::Intersect:
		// Intersect is considered like Set. This typically occurs in nested clipping situations, which never worked
		// correctly in legacy.
		break;
	case ClipMaskOperation::SetInverse:
		// Using features not supported in legacy, bail out.
		return;
	}

	// New features can render more complex clip masks, while legacy only supported rectangle scissoring. Find the
	// geometry's rectangular coverage.
	const AdaptedGeometry* geometry = reinterpret_cast<AdaptedGeometry*>(handle);

	Rectanglef rectangle = Rectanglef::FromPosition(geometry->vertices[0].position);
	for (const Vertex& vertex : geometry->vertices)
		rectangle = rectangle.Join(vertex.position);

	const Rectanglei scissor = Rectanglei(rectangle.Translate(translation));
	legacy.SetScissorRegion(scissor.Left(), scissor.Top(), scissor.Width(), scissor.Height());
}

TextureHandle RenderInterfaceAdapter::LoadTexture(Vector2i& texture_dimensions, const String& source)
{
	TextureHandle texture_handle = {};
	if (!legacy.LoadTexture(texture_handle, texture_dimensions, source))
		texture_handle = {};
	return texture_handle;
}

TextureHandle RenderInterfaceAdapter::GenerateTexture(Span<const byte> source_data, Vector2i source_dimensions)
{
	// Previously, textures were given in unpremultiplied alpha format. Since RmlUi 6, they are given in premultiplied
	// alpha. For compatibility, convert the texture to unpremultiplied alpha which is expected by legacy render
	// interfaces.
	const int num_bytes = source_dimensions.x * source_dimensions.y * 4;
	std::unique_ptr<byte[]> unpremultiplied_copy(new byte[num_bytes]);

	for (int i = 0; i < num_bytes; i += 4)
	{
		UnPremultiplyAlpha(&source_data[i], unpremultiplied_copy.get() + i);
	}

	TextureHandle texture_handle = {};
	if (!legacy.GenerateTexture(texture_handle, unpremultiplied_copy.get(), source_dimensions))
		texture_handle = {};
	return texture_handle;
}

void RenderInterfaceAdapter::ReleaseTexture(TextureHandle texture_handle)
{
	legacy.ReleaseTexture(texture_handle);
}

void RenderInterfaceAdapter::SetTransform(const Matrix4f* transform)
{
	legacy.SetTransform(transform);
}

} // namespace Rml
