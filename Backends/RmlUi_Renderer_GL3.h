#pragma once

#include <RmlUi/Core/RenderInterface.h>
#include <RmlUi/Core/Types.h>
#include <bitset>

enum class ProgramId;
enum class UniformId;
class RenderLayerStack;
namespace Gfx {
struct ProgramData;
struct FramebufferData;
} // namespace Gfx

class RenderInterface_GL3 : public Rml::RenderInterface {
public:
	RenderInterface_GL3();
	~RenderInterface_GL3();

	// Returns true if the renderer was successfully constructed.
	explicit operator bool() const { return static_cast<bool>(program_data); }

	// The viewport should be updated whenever the window size changes.
	void SetViewport(int viewport_width, int viewport_height, int viewport_offset_x = 0, int viewport_offset_y = 0);

	// Sets up OpenGL states for taking rendering commands from RmlUi.
	void BeginFrame();
	// Draws the result to the backbuffer and restores OpenGL state.
	void EndFrame();

	// Optional, can be used to clear the active framebuffer.
	void Clear();

	// -- Inherited from Rml::RenderInterface --

	Rml::CompiledGeometryHandle CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices) override;
	void RenderGeometry(Rml::CompiledGeometryHandle handle, Rml::Vector2f translation, Rml::TextureHandle texture) override;
	void ReleaseGeometry(Rml::CompiledGeometryHandle handle) override;

	Rml::TextureHandle LoadTexture(Rml::Vector2i& texture_dimensions, const Rml::String& source) override;
	Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte> source_data, Rml::Vector2i source_dimensions) override;
	void ReleaseTexture(Rml::TextureHandle texture_handle) override;

	void EnableScissorRegion(bool enable) override;
	void SetScissorRegion(Rml::Rectanglei region) override;

	void EnableClipMask(bool enable) override;
	void RenderToClipMask(Rml::ClipMaskOperation mask_operation, Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation) override;

	void SetTransform(const Rml::Matrix4f* transform) override;

	Rml::LayerHandle PushLayer() override;
	void CompositeLayers(Rml::LayerHandle source, Rml::LayerHandle destination, Rml::BlendMode blend_mode,
		Rml::Span<const Rml::CompiledFilterHandle> filters) override;
	void PopLayer() override;

	Rml::TextureHandle SaveLayerAsTexture() override;

	Rml::CompiledFilterHandle SaveLayerAsMaskImage() override;

	Rml::CompiledFilterHandle CompileFilter(const Rml::String& name, const Rml::Dictionary& parameters) override;
	void ReleaseFilter(Rml::CompiledFilterHandle filter) override;

	Rml::CompiledShaderHandle CompileShader(const Rml::String& name, const Rml::Dictionary& parameters) override;
	void RenderShader(Rml::CompiledShaderHandle shader_handle, Rml::CompiledGeometryHandle geometry_handle, Rml::Vector2f translation,
		Rml::TextureHandle texture) override;
	void ReleaseShader(Rml::CompiledShaderHandle effect_handle) override;

	// Can be passed to RenderGeometry() to enable texture rendering without changing the bound texture.
	static constexpr Rml::TextureHandle TextureEnableWithoutBinding = Rml::TextureHandle(-1);
	// Can be passed to RenderGeometry() to leave the bound texture and used program unchanged.
	static constexpr Rml::TextureHandle TexturePostprocess = Rml::TextureHandle(-2);

	// -- Utility functions for clients --

	const Rml::Matrix4f& GetTransform() const;
	void ResetProgram();

private:
	void UseProgram(ProgramId program_id);
	int GetUniformLocation(UniformId uniform_id) const;
	void SubmitTransformUniform(Rml::Vector2f translation);

	void BlitLayerToPostprocessPrimary(Rml::LayerHandle layer_handle);
	void RenderFilters(Rml::Span<const Rml::CompiledFilterHandle> filter_handles);

	void SetScissor(Rml::Rectanglei region, bool vertically_flip = false);

	void DrawFullscreenQuad();
	void DrawFullscreenQuad(Rml::Vector2f uv_offset, Rml::Vector2f uv_scaling = Rml::Vector2f(1.f));

	void RenderBlur(float sigma, const Gfx::FramebufferData& source_destination, const Gfx::FramebufferData& temp, Rml::Rectanglei window_flipped);

	static constexpr size_t MaxNumPrograms = 32;
	std::bitset<MaxNumPrograms> program_transform_dirty;

	Rml::Matrix4f transform;
	Rml::Matrix4f projection;

	ProgramId active_program = {};
	Rml::Rectanglei scissor_state;

	int viewport_width = 0;
	int viewport_height = 0;
	int viewport_offset_x = 0;
	int viewport_offset_y = 0;

	Rml::CompiledGeometryHandle fullscreen_quad_geometry = {};

	Rml::UniquePtr<const Gfx::ProgramData> program_data;

	/*
	    Manages render targets, including the layer stack and postprocessing framebuffers.

	    Layers can be pushed and popped, creating new framebuffers as needed. Typically, geometry is rendered to the top
	    layer. The layer framebuffers may have MSAA enabled.

	    Postprocessing framebuffers are separate from the layers, and are commonly used to apply texture-wide effects
	    such as filters. They are used both as input and output during rendering, and do not use MSAA.
	*/
	class RenderLayerStack {
	public:
		RenderLayerStack();
		~RenderLayerStack();

		// Push a new layer. All references to previously retrieved layers are invalidated.
		Rml::LayerHandle PushLayer();

		// Pop the top layer. All references to previously retrieved layers are invalidated.
		void PopLayer();

		const Gfx::FramebufferData& GetLayer(Rml::LayerHandle layer) const;
		const Gfx::FramebufferData& GetTopLayer() const;
		Rml::LayerHandle GetTopLayerHandle() const;

		const Gfx::FramebufferData& GetPostprocessPrimary() { return EnsureFramebufferPostprocess(0); }
		const Gfx::FramebufferData& GetPostprocessSecondary() { return EnsureFramebufferPostprocess(1); }
		const Gfx::FramebufferData& GetPostprocessTertiary() { return EnsureFramebufferPostprocess(2); }
		const Gfx::FramebufferData& GetBlendMask() { return EnsureFramebufferPostprocess(3); }

		void SwapPostprocessPrimarySecondary();

		void BeginFrame(int new_width, int new_height);
		void EndFrame();

	private:
		void DestroyFramebuffers();
		const Gfx::FramebufferData& EnsureFramebufferPostprocess(int index);

		int width = 0, height = 0;

		// The number of active layers is manually tracked since we re-use the framebuffers stored in the fb_layers stack.
		int layers_size = 0;

		Rml::Vector<Gfx::FramebufferData> fb_layers;
		Rml::Vector<Gfx::FramebufferData> fb_postprocess;
	};

	RenderLayerStack render_layers;

	struct GLStateBackup {
		bool enable_cull_face;
		bool enable_blend;
		bool enable_stencil_test;
		bool enable_scissor_test;
		bool enable_depth_test;

		int viewport[4];
		int scissor[4];

		int active_texture;

		int stencil_clear_value;
		float color_clear_value[4];
		unsigned char color_writemask[4];

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
