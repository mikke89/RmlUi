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

#ifndef RMLUI_BACKENDS_RENDERER_GL3_H
#define RMLUI_BACKENDS_RENDERER_GL3_H

#include <RmlUi/Core/RenderInterface.h>
#include <RmlUi/Core/Types.h>

namespace Gfx {
struct ShadersData;
}

class RenderInterface_GL3 : public Rml::RenderInterface {
public:
	RenderInterface_GL3();
	~RenderInterface_GL3();

	// Returns true if the renderer was successfully constructed.
	explicit operator bool() const { return static_cast<bool>(shaders); }

	// The viewport should be updated whenever the window size changes.
	void SetViewport(int viewport_width, int viewport_height);

	// Sets up OpenGL states for taking rendering commands from RmlUi.
	void BeginFrame();
	void EndFrame();

	// Optional, can be used to clear the framebuffer.
	void Clear();

	// -- Inherited from Rml::RenderInterface --

	void RenderGeometry(Rml::Vertex* vertices, int num_vertices, int* indices, int num_indices, Rml::TextureHandle texture,
		const Rml::Vector2f& translation) override;

	Rml::CompiledGeometryHandle CompileGeometry(Rml::Vertex* vertices, int num_vertices, int* indices, int num_indices,
		Rml::TextureHandle texture) override;
	void RenderCompiledGeometry(Rml::CompiledGeometryHandle geometry, const Rml::Vector2f& translation) override;
	void ReleaseCompiledGeometry(Rml::CompiledGeometryHandle geometry) override;

	void EnableScissorRegion(bool enable) override;
	void SetScissorRegion(int x, int y, int width, int height) override;

	bool LoadTexture(Rml::TextureHandle& texture_handle, Rml::Vector2i& texture_dimensions, const Rml::String& source) override;
	bool GenerateTexture(Rml::TextureHandle& texture_handle, const Rml::byte* source, const Rml::Vector2i& source_dimensions) override;
	void ReleaseTexture(Rml::TextureHandle texture_handle) override;

	void SetTransform(const Rml::Matrix4f* transform) override;

	// Can be passed to RenderGeometry() to enable texture rendering without changing the bound texture.
	static const Rml::TextureHandle TextureEnableWithoutBinding = Rml::TextureHandle(-1);

private:
	enum class ProgramId { None, Texture = 1, Color = 2, All = (Texture | Color) };
	void SubmitTransformUniform(ProgramId program_id, int uniform_location);

	Rml::Matrix4f transform, projection;
	ProgramId transform_dirty_state = ProgramId::All;
	bool transform_active = false;

	enum class ScissoringState { Disable, Scissor, Stencil };
	ScissoringState scissoring_state = ScissoringState::Disable;

	int viewport_width = 0;
	int viewport_height = 0;

	Rml::UniquePtr<Gfx::ShadersData> shaders;
};

/**
    Helper functions for the OpenGL 3 renderer.
 */
namespace RmlGL3 {

// Loads OpenGL functions. Optionally, the out message describes the loaded GL version or an error message on failure.
bool Initialize(Rml::String* out_message = nullptr);

// Unloads OpenGL functions.
void Shutdown();

} // namespace RmlGL3

#endif
