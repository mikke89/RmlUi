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

#ifndef RMLUI_CORE_RENDERINTERFACECOMPATIBILITY_H
#define RMLUI_CORE_RENDERINTERFACECOMPATIBILITY_H

#include "RenderInterface.h"

namespace Rml {

class RenderInterfaceAdapter;

/**
    Provides a backward-compatible adapter for render interfaces written for RmlUi 5 and lower. The compatibility adapter
    should be used as follows.

    1. In your legacy RenderInterface implementation, derive from Rml::RenderInterfaceCompatibility instead of
       Rml::RenderInterface.

           #include <RmlUi/Core/RenderInterfaceCompatibility.h>
           class MyRenderInterface : public Rml::RenderInterfaceCompatibility { ... };

    2. Use the adapted interface when setting the RmlUi render interface.

           Rml::SetRenderInterface(my_render_interface.GetAdaptedInterface());

    New rendering features are not supported when using the compatibility adapter.
*/

class RMLUICORE_API RenderInterfaceCompatibility : public NonCopyMoveable {
public:
	RenderInterfaceCompatibility();
	virtual ~RenderInterfaceCompatibility();

	virtual void RenderGeometry(Vertex* vertices, int num_vertices, int* indices, int num_indices, TextureHandle texture,
		const Vector2f& translation) = 0;

	virtual CompiledGeometryHandle CompileGeometry(Vertex* vertices, int num_vertices, int* indices, int num_indices, TextureHandle texture);
	virtual void RenderCompiledGeometry(CompiledGeometryHandle geometry, const Vector2f& translation);
	virtual void ReleaseCompiledGeometry(CompiledGeometryHandle geometry);

	virtual void EnableScissorRegion(bool enable) = 0;
	virtual void SetScissorRegion(int x, int y, int width, int height) = 0;

	virtual bool LoadTexture(TextureHandle& texture_handle, Vector2i& texture_dimensions, const String& source);
	virtual bool GenerateTexture(TextureHandle& texture_handle, const byte* source, const Vector2i& source_dimensions);
	virtual void ReleaseTexture(TextureHandle texture);

	virtual void SetTransform(const Matrix4f* transform);

	RenderInterface* GetAdaptedInterface();

private:
	UniquePtr<RenderInterfaceAdapter> adapter;
};

/*
    The render interface adapter takes calls from the render interface, makes any necessary conversions, and passes the
    calls on to the legacy render interface.
*/
class RMLUICORE_API RenderInterfaceAdapter : public RenderInterface {
public:
	CompiledGeometryHandle CompileGeometry(Span<const Vertex> vertices, Span<const int> indices) override;
	void RenderGeometry(CompiledGeometryHandle handle, Vector2f translation, TextureHandle texture) override;
	void ReleaseGeometry(CompiledGeometryHandle handle) override;

	void EnableScissorRegion(bool enable) override;
	void SetScissorRegion(Rectanglei region) override;

	TextureHandle LoadTexture(Vector2i& texture_dimensions, const String& source) override;
	TextureHandle GenerateTexture(Span<const byte> source_data, Vector2i source_dimensions) override;
	void ReleaseTexture(TextureHandle texture_handle) override;

	void EnableClipMask(bool enable) override;
	void RenderToClipMask(ClipMaskOperation operation, CompiledGeometryHandle geometry, Vector2f translation) override;

	void SetTransform(const Matrix4f* transform) override;

private:
	using LegacyCompiledGeometryHandle = CompiledGeometryHandle;

	struct AdaptedGeometry {
		Vector<Vertex> vertices;
		Vector<int> indices;
		SmallUnorderedMap<TextureHandle, LegacyCompiledGeometryHandle> textures;
	};

	RenderInterfaceAdapter(RenderInterfaceCompatibility& legacy);

	RenderInterfaceCompatibility& legacy;

	friend Rml::RenderInterfaceCompatibility::RenderInterfaceCompatibility();
};

} // namespace Rml
#endif
