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
enum class LayerFill {
	None,  // No operation necessary, does not care about the layer color.
	Clear, // Clear the layer to transparent black.
	Clone, // Copy the color data from the previous layer.
};
enum class BlendMode {
	Blend,   // Normal alpha blending.
	Replace, // Replace the destination colors from the source.
	Discard, // Leave the destination colors unaltered.
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

	/// Called by RmlUi when it wants to render geometry that the application does not wish to optimise. Note that
	/// RmlUi renders everything as triangles.
	/// @param[in] vertices The geometry's vertex data.
	/// @param[in] num_vertices The number of vertices passed to the function.
	/// @param[in] indices The geometry's index data.
	/// @param[in] num_indices The number of indices passed to the function. This will always be a multiple of three.
	/// @param[in] texture The texture to be applied to the geometry. This may be nullptr, in which case the geometry is untextured.
	/// @param[in] translation The translation to apply to the geometry.
	virtual void RenderGeometry(Vertex* vertices, int num_vertices, int* indices, int num_indices, TextureHandle texture,
		const Vector2f& translation) = 0;

	/// Called by RmlUi when it wants to compile geometry it believes will be static for the forseeable future.
	/// If supported, this should return a handle to an optimised, application-specific version of the data. If
	/// not, do not override the function or return zero; the simpler RenderGeometry() will be called instead.
	/// @param[in] vertices The geometry's vertex data.
	/// @param[in] num_vertices The number of vertices passed to the function.
	/// @param[in] indices The geometry's index data.
	/// @param[in] num_indices The number of indices passed to the function. This will always be a multiple of three.
	/// @return The application-specific compiled geometry. Compiled geometry will be stored and rendered using RenderCompiledGeometry() in future
	/// calls, and released with ReleaseCompiledGeometry() when it is no longer needed.
	virtual CompiledGeometryHandle CompileGeometry(Vertex* vertices, int num_vertices, int* indices, int num_indices);
	/// Called by RmlUi when it wants to render application-compiled geometry.
	/// @param[in] geometry The application-specific compiled geometry to render.
	/// @param[in] translation The translation to apply to the geometry.
	/// @param[in] texture The texture to be applied to the geometry. This may be nullptr, in which case the geometry is untextured.
	virtual void RenderCompiledGeometry(CompiledGeometryHandle geometry, const Vector2f& translation, TextureHandle texture);
	/// Called by RmlUi when it wants to release application-compiled geometry.
	/// @param[in] geometry The application-specific compiled geometry to release.
	virtual void ReleaseCompiledGeometry(CompiledGeometryHandle geometry);

	/// Called by RmlUi when a texture is required by the library.
	/// @param[out] texture_handle The handle to write the texture handle for the loaded texture to.
	/// @param[out] texture_dimensions The variable to write the dimensions of the loaded texture.
	/// @param[in] source The application-defined image source, joined with the path of the referencing document.
	/// @return True if the load attempt succeeded and the handle and dimensions are valid, false if not.
	virtual bool LoadTexture(TextureHandle& texture_handle, Vector2i& texture_dimensions, const String& source);
	/// Called by RmlUi when a texture is required to be built from an internally-generated sequence of pixels.
	/// @param[out] texture_handle The handle to write the texture handle for the generated texture to.
	/// @param[in] source The raw texture data. Each pixel is made up of four 8-bit values, red, green, blue, and premultiplied alpha, in that order.
	/// @param[in] source_dimensions The dimensions, in pixels, of the source data.
	/// @return True if the texture generation succeeded and the handle is valid, false if not.
	virtual bool GenerateTexture(TextureHandle& texture_handle, const byte* source, const Vector2i& source_dimensions);
	/// Called by RmlUi when a loaded texture is no longer required.
	/// @param[in] texture The texture handle to release.
	virtual void ReleaseTexture(TextureHandle texture);

	/// Called by RmlUi when it wants to enable or disable scissoring to clip content.
	/// @param[in] enable True if scissoring is to enabled, false if it is to be disabled.
	virtual void EnableScissorRegion(bool enable) = 0;
	/// Called by RmlUi when it wants to change the scissor region.
	/// @param[in] x The left-most pixel to be rendered. All pixels to the left of this should be clipped.
	/// @param[in] y The top-most pixel to be rendered. All pixels to the top of this should be clipped.
	/// @param[in] width The width of the scissored region. All pixels to the right of (x + width) should be clipped.
	/// @param[in] height The height of the scissored region. All pixels to below (y + height) should be clipped.
	virtual void SetScissorRegion(int x, int y, int width, int height) = 0;

	/**
	    @name Remaining functions are optional to implement for advanced effects.
	 */

	/// Called by RmlUi when it wants to enable or disable the clip mask.
	/// @param[in] enable True if the clip mask is to be enabled, false if it is to be disabled.
	/// @note When enabled, the clip mask should hide any rendered contents outside the area of the mask.
	virtual void EnableClipMask(bool enable);
	/// Called by RmlUi when it wants to set or modify the contents of the clip mask.
	/// @param[in] operation Describes how the geometry should affect the clip mask.
	/// @param[in] geometry The compiled geometry to render.
	/// @param[in] translation The translation to apply to the geometry.
	virtual void RenderToClipMask(ClipMaskOperation operation, CompiledGeometryHandle geometry, Vector2f translation);

	/// Called by RmlUi when it wants the renderer to use a new transform matrix.
	/// This will only be called if 'transform' properties are encountered. If no transform applies to the current element, nullptr
	/// is submitted. Then it expects the renderer to use an identity matrix or otherwise omit the multiplication with the transform.
	/// @param[in] transform The new transform to apply, or nullptr if no transform applies to the current element.
	virtual void SetTransform(const Matrix4f* transform);

	/// Called by RmlUi when it wants to push a new layer onto the render stack.
	/// @param[in] layer_fill Specifies how the color data of the new layer should be filled.
	virtual void PushLayer(LayerFill layer_fill);
	/// Called by RmlUi when it wants to pop the render layer stack, after applying filters to the top layer and blending it into the layer below.
	/// @param[in] blend_mode The mode used to blend the top layer into the one below.
	/// @param[in] filters A list of compiled filters which should be applied to the top layer before blending.
	virtual void PopLayer(BlendMode blend_mode, const FilterHandleList& filters);

	/// Called by RmlUi when it wants to store the current layer as a new texture to be rendered later with geometry.
	/// @param[in] dimensions The dimensions of the texture, to be copied from the top-left part of the viewport.
	/// @return The handle to the new texture.
	virtual TextureHandle SaveLayerAsTexture(Vector2i dimensions);

	/// Called by RmlUi when it wants to store the current layer as a mask image, to be applied later as a filter.
	/// @return The handle to a new filter representng the stored mask image.
	virtual CompiledFilterHandle SaveLayerAsMaskImage();

	/// Called by RmlUi when it wants to compile a new filter.
	/// @param[in] name The name of the filter.
	/// @param[in] parameters The list of name-value parameters specified for the filter.
	/// @return The handle representing the compiled filter.
	virtual CompiledFilterHandle CompileFilter(const String& name, const Dictionary& parameters);
	/// Called by RmlUi when it no longer needs a previously compiled filter.
	/// @param[in] filter The handle to a previously compiled filter.
	virtual void ReleaseCompiledFilter(CompiledFilterHandle filter);

	/// Called by RmlUi when it wants to compile a new shader.
	/// @param[in] name The name of the shader.
	/// @param[in] parameters The list of name-value parameters specified for the filter.
	/// @return The handle representing the compiled shader.
	virtual CompiledShaderHandle CompileShader(const String& name, const Dictionary& parameters);
	/// Called by RmlUi when it wants to render geometry using the given shader.
	/// @param[in] shader The handle to a previously compiled shader.
	/// @param[in] geometry The handle to a previously compiled geometry.
	/// @param[in] translation The translation to apply to the geometry.
	/// @param[in] texture The texture to use when rendering the geometry, or nullptr if no texture is required.
	virtual void RenderShader(CompiledShaderHandle shader, CompiledGeometryHandle geometry, Vector2f translation, TextureHandle texture);
	/// Called by RmlUi when it no longer needs a previously compiled shader.
	/// @param[in] shader The handle to a previously compiled shader.
	virtual void ReleaseCompiledShader(CompiledShaderHandle shader);
};

} // namespace Rml
#endif
