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

#ifndef RMLUI_SHELL_SHELLRENDERINTERFACEOPENGL_H
#define RMLUI_SHELL_SHELLRENDERINTERFACEOPENGL_H

#include <RmlUi/Core/RenderInterface.h>
#include "ShellOpenGL.h"

/**
	Low level OpenGL render interface for RmlUi
	@author Peter Curry
 */

class ShellRenderInterfaceOpenGL : public Rml::RenderInterface,  public ShellRenderInterfaceExtensions
{
public:
	ShellRenderInterfaceOpenGL();

	/// Called by RmlUi when it wants to render geometry that it does not wish to optimise.
	void RenderGeometry(Rml::Vertex* vertices, int num_vertices, int* indices, int num_indices, Rml::TextureHandle texture, const Rml::Vector2f& translation) override;

	/// Called by RmlUi when it wants to compile geometry it believes will be static for the forseeable future.
	Rml::CompiledGeometryHandle CompileGeometry(Rml::Vertex* vertices, int num_vertices, int* indices, int num_indices, Rml::TextureHandle texture) override;

	/// Called by RmlUi when it wants to render application-compiled geometry.
	void RenderCompiledGeometry(Rml::CompiledGeometryHandle geometry, const Rml::Vector2f& translation) override;
	/// Called by RmlUi when it wants to release application-compiled geometry.
	void ReleaseCompiledGeometry(Rml::CompiledGeometryHandle geometry) override;

	/// Called by RmlUi when it wants to enable or disable scissoring to clip content.
	void EnableScissorRegion(bool enable) override;
	/// Called by RmlUi when it wants to change the scissor region.
	void SetScissorRegion(int x, int y, int width, int height) override;

	/// Called by RmlUi when a texture is required by the library.
	bool LoadTexture(Rml::TextureHandle& texture_handle, Rml::Vector2i& texture_dimensions, const Rml::String& source) override;
	/// Called by RmlUi when a texture is required to be built from an internally-generated sequence of pixels.
	bool GenerateTexture(Rml::TextureHandle& texture_handle, const Rml::byte* source, const Rml::Vector2i& source_dimensions) override;
	/// Called by RmlUi when a loaded texture is no longer required.
	void ReleaseTexture(Rml::TextureHandle texture_handle) override;

	/// Called by RmlUi when it wants to set the current transform matrix to a new matrix.
	void SetTransform(const Rml::Matrix4f* transform) override;

	// Extensions used by the test suite
	struct Image {
		int width = 0;
		int height = 0;
		int num_components = 0;
		Rml::UniquePtr<Rml::byte[]> data;
	};
	Image CaptureScreen();

	// ShellRenderInterfaceExtensions
	void SetViewport(int width, int height) override;
	bool AttachToNative(void *nativeWindow) override;
	void DetachFromNative(void) override;
	void PrepareRenderBuffer(void) override;
	void PresentRenderBuffer(void) override;

protected:
	int m_width;
	int m_height;
	bool m_transform_enabled;
	
#if defined(RMLUI_PLATFORM_MACOSX)
	AGLContext gl_context;
#elif defined(RMLUI_PLATFORM_LINUX)
	struct __X11NativeWindowData nwData;
	GLXContext gl_context;
#elif defined(RMLUI_PLATFORM_WIN32)
	HWND window_handle;
	HDC device_context;
	HGLRC render_context;
#else
#error Platform is undefined, this must be resolved so gl_context is usable.
#endif
};

#endif
