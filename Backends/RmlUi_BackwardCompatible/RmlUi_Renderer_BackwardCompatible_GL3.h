#pragma once

#include <RmlUi/Core/RenderInterfaceCompatibility.h>
#include <RmlUi/Core/Types.h>

namespace Gfx {
struct ShadersData;
}

/*
    The GL3 renderer from RmlUi 5, only modified to derive from the compatibility interface.

    Implemented for testing and demonstration purposes, not recommended for production use.
*/

class RenderInterface_BackwardCompatible_GL3 : public Rml::RenderInterfaceCompatibility {
public:
	RenderInterface_BackwardCompatible_GL3();
	~RenderInterface_BackwardCompatible_GL3();

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

	struct GLStateBackup {
		bool enable_cull_face;
		bool enable_blend;
		bool enable_stencil_test;
		bool enable_scissor_test;

		int viewport[4];
		int scissor[4];

		int stencil_clear_value;
		float color_clear_value[4];

		int blend_equation_rgb;
		int blend_equation_alpha;
		int blend_src_rgb;
		int blend_dst_rgb;
		int blend_src_alpha;
		int blend_dst_alpha;

		struct Stencil {
			int func;
			int ref;
			int value_mask;
			int writemask;
			int fail;
			int pass_depth_fail;
			int pass_depth_pass;
		};
		Stencil stencil_front;
		Stencil stencil_back;
	};
	GLStateBackup glstate_backup = {};
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
