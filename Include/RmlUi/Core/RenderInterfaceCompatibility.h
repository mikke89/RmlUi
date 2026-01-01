#pragma once

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
