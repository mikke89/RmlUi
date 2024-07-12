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

#ifndef RMLUI_CORE_RENDERINTERFACE_H
#define RMLUI_CORE_RENDERINTERFACE_H

#include "Header.h"
#include "Traits.h"
#include "Types.h"
#include "Vertex.h"

namespace Rml {

enum class ClipMaskOperation {
	Set,        // Set the clip mask to the area of the rendered geometry, clearing any existing clip mask.
	SetInverse, // Set the clip mask to the area *outside* the rendered geometry, clearing any existing clip mask.
	Intersect,  // Intersect the clip mask with the area of the rendered geometry.
};
enum class BlendMode {
	Blend,   // Normal alpha blending.
	Replace, // Replace the destination colors from the source.
};

/**
    The abstract base class for application-specific rendering implementation. Your application must provide a concrete
    implementation of this class and install it through Rml::SetRenderInterface() in order for anything to be rendered.

    @author Peter Curry
 */

class RMLUICORE_API RenderInterface : public NonCopyMoveable {
public:
	RenderInterface();
	virtual ~RenderInterface();

	/**
	    @name Required functions for basic rendering.
	 */

	/// Called by RmlUi when it wants to compile geometry to be rendered later.
	/// @param[in] vertices The geometry's vertex data.
	/// @param[in] indices The geometry's index data.
	/// @return An application-specified handle to the geometry, or zero if it could not be compiled.
	/// @lifetime The pointed-to vertex and index data are guaranteed to be valid and immutable until ReleaseGeometry()
	/// is called with the geometry handle returned here.
	virtual CompiledGeometryHandle CompileGeometry(Span<const Vertex> vertices, Span<const int> indices) = 0;
	/// Called by RmlUi when it wants to render geometry.
	/// @param[in] geometry The geometry to render.
	/// @param[in] translation The translation to apply to the geometry.
	/// @param[in] texture The texture to be applied to the geometry, or zero if the geometry is untextured.
	virtual void RenderGeometry(CompiledGeometryHandle geometry, Vector2f translation, TextureHandle texture) = 0;
	/// Called by RmlUi when it wants to release geometry.
	/// @param[in] geometry The geometry to release.
	virtual void ReleaseGeometry(CompiledGeometryHandle geometry) = 0;

	/// Called by RmlUi when a texture is required by the library.
	/// @param[out] texture_dimensions The dimensions of the loaded texture, which must be set by the application.
	/// @param[in] source The application-defined image source, joined with the path of the referencing document.
	/// @return An application-specified handle identifying the texture, or zero if it could not be loaded.
	virtual TextureHandle LoadTexture(Vector2i& texture_dimensions, const String& source) = 0;
	/// Called by RmlUi when a texture is required to be generated from a sequence of pixels in memory.
	/// @param[in] source The raw texture data. Each pixel is made up of four 8-bit values, red, green, blue, and premultiplied alpha, in that order.
	/// @param[in] source_dimensions The dimensions, in pixels, of the source data.
	/// @return An application-specified handle identifying the texture, or zero if it could not be generated.
	virtual TextureHandle GenerateTexture(Span<const byte> source, Vector2i source_dimensions) = 0;
	/// Called by RmlUi when a loaded or generated texture is no longer required.
	/// @param[in] texture The texture handle to release.
	virtual void ReleaseTexture(TextureHandle texture) = 0;

	/// Called by RmlUi when it wants to enable or disable scissoring to clip content.
	/// @param[in] enable True if scissoring is to enabled, false if it is to be disabled.
	virtual void EnableScissorRegion(bool enable) = 0;
	/// Called by RmlUi when it wants to change the scissor region.
	/// @param[in] region The region to be rendered. All pixels outside this region should be clipped.
	/// @note The region should be applied in window coordinates regardless of any active transform.
	virtual void SetScissorRegion(Rectanglei region) = 0;

	/**
	    @name Optional functions for advanced rendering features.
	 */

	/// Called by RmlUi when it wants to enable or disable the clip mask.
	/// @param[in] enable True if the clip mask is to be enabled, false if it is to be disabled.
	virtual void EnableClipMask(bool enable);
	/// Called by RmlUi when it wants to set or modify the contents of the clip mask.
	/// @param[in] operation Describes how the geometry should affect the clip mask.
	/// @param[in] geometry The compiled geometry to render.
	/// @param[in] translation The translation to apply to the geometry.
	/// @note When enabled, the clip mask should hide any rendered contents outside the area of the mask.
	/// @note The clip mask applies exclusively to all other functions that render with a geometry handle, in addition
	/// to the layer compositing function while rendering to its destination.
	virtual void RenderToClipMask(ClipMaskOperation operation, CompiledGeometryHandle geometry, Vector2f translation);

	/// Called by RmlUi when it wants the renderer to use a new transform matrix.
	/// @param[in] transform The new transform to apply, or nullptr if no transform applies to the current element.
	/// @note When nullptr is submitted, the renderer should use an identity transform matrix or otherwise omit the
	/// multiplication with the transform.
	/// @note The transform applies to all functions that render with a geometry handle, and only those.
	virtual void SetTransform(const Matrix4f* transform);

	/// Called by RmlUi when it wants to push a new layer onto the render stack, setting it as the new render target.
	/// @return An application-specified handle representing the new layer. The value 'zero' is reserved for the initial base layer.
	/// @note The new layer should be initialized to transparent black within the current scissor region.
	virtual LayerHandle PushLayer();
	/// Composite two layers with the given blend mode and apply filters.
	/// @param[in] source The source layer.
	/// @param[in] destination The destination layer.
	/// @param[in] blend_mode The mode used to blend the source layer onto the destination layer.
	/// @param[in] filters A list of compiled filters which should be applied before blending.
	/// @note Source and destination can reference the same layer.
	virtual void CompositeLayers(LayerHandle source, LayerHandle destination, BlendMode blend_mode, Span<const CompiledFilterHandle> filters);
	/// Called by RmlUi when it wants to pop the render layer stack, setting the new top layer as the render target.
	virtual void PopLayer();

	/// Called by RmlUi when it wants to store the current layer as a new texture to be rendered later with geometry.
	/// @return An application-specified handle to the new texture.
	/// @note The texture should be extracted using the bounds defined by the active scissor region, thereby matching its size.
	virtual TextureHandle SaveLayerAsTexture();

	/// Called by RmlUi when it wants to store the current layer as a mask image, to be applied later as a filter.
	/// @return An application-specified handle to a new filter representing the stored mask image.
	virtual CompiledFilterHandle SaveLayerAsMaskImage();

	/// Called by RmlUi when it wants to compile a new filter.
	/// @param[in] name The name of the filter.
	/// @param[in] parameters The list of name-value parameters specified for the filter.
	/// @return An application-specified handle representing the compiled filter.
	virtual CompiledFilterHandle CompileFilter(const String& name, const Dictionary& parameters);
	/// Called by RmlUi when it no longer needs a previously compiled filter.
	/// @param[in] filter The handle to a previously compiled filter.
	virtual void ReleaseFilter(CompiledFilterHandle filter);

	/// Called by RmlUi when it wants to compile a new shader.
	/// @param[in] name The name of the shader.
	/// @param[in] parameters The list of name-value parameters specified for the filter.
	/// @return An application-specified handle representing the shader.
	virtual CompiledShaderHandle CompileShader(const String& name, const Dictionary& parameters);
	/// Called by RmlUi when it wants to render geometry using the given shader.
	/// @param[in] shader The handle to a previously compiled shader.
	/// @param[in] geometry The handle to a previously compiled geometry.
	/// @param[in] translation The translation to apply to the geometry.
	/// @param[in] texture The texture to use when rendering the geometry, or zero for no texture.
	virtual void RenderShader(CompiledShaderHandle shader, CompiledGeometryHandle geometry, Vector2f translation, TextureHandle texture);
	/// Called by RmlUi when it no longer needs a previously compiled shader.
	/// @param[in] shader The handle to a previously compiled shader.
	virtual void ReleaseShader(CompiledShaderHandle shader);
};

} // namespace Rml
#endif
