#pragma once

#include <RmlUi/Core/RenderInterface.h>
#include <RmlUi/Core/Types.h>
#include <SDL3/SDL.h>

class RenderInterface_SDL_GPU : public Rml::RenderInterface {
public:
	RenderInterface_SDL_GPU(SDL_GPUDevice* device, SDL_Window* window);
	~RenderInterface_SDL_GPU() override;

	void Shutdown();

	void BeginFrame(SDL_GPUCommandBuffer* command_buffer, SDL_GPUTexture* swapchain_texture, uint32_t width, uint32_t height);
	void EndFrame();

	// Rows come back bottom-up. Must be called after EndFrame() and before the next BeginFrame().
	bool CaptureScreen(int& width, int& height, int& num_components, Rml::UniquePtr<Rml::byte[]>& data);

	// -- Inherited from Rml::RenderInterface --

	Rml::CompiledGeometryHandle CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices) override;
	void RenderGeometry(Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation, Rml::TextureHandle texture) override;
	void ReleaseGeometry(Rml::CompiledGeometryHandle geometry) override;

	Rml::TextureHandle LoadTexture(Rml::Vector2i& texture_dimensions, const Rml::String& source) override;
	Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i source_dimensions) override;
	void ReleaseTexture(Rml::TextureHandle texture_handle) override;

	void EnableScissorRegion(bool enable) override;
	void SetScissorRegion(Rml::Rectanglei region) override;

	void EnableClipMask(bool enable) override;
	void RenderToClipMask(Rml::ClipMaskOperation operation, Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation) override;

	void SetTransform(const Rml::Matrix4f* new_transform) override;

	Rml::LayerHandle PushLayer() override;
	void CompositeLayers(Rml::LayerHandle source, Rml::LayerHandle destination, Rml::BlendMode blend_mode,
		Rml::Span<const Rml::CompiledFilterHandle> filters) override;
	void PopLayer() override;

	Rml::TextureHandle SaveLayerAsTexture() override;

	Rml::CompiledFilterHandle SaveLayerAsMaskImage() override;

	Rml::CompiledFilterHandle CompileFilter(const Rml::String& name, const Rml::Dictionary& parameters) override;
	void ReleaseFilter(Rml::CompiledFilterHandle filter) override;

	Rml::CompiledShaderHandle CompileShader(const Rml::String& name, const Rml::Dictionary& parameters) override;
	void RenderShader(Rml::CompiledShaderHandle shader, Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation,
		Rml::TextureHandle texture) override;
	void ReleaseShader(Rml::CompiledShaderHandle shader) override;

private:
	// Buffers are handed out in power-of-two sized buckets so that the same allocations can be reused across frames
	// instead of creating a new buffer for every distinct mesh size.
	static constexpr uint32_t min_buffer_size = 4096;
	static constexpr int num_buffer_buckets = 20;
	// Free buffers that have not been used for this many frames are released back to the driver.
	static constexpr int buffer_retention_frames = 120;
	// Transfers are batched into a single command buffer until this much data has been queued.
	static constexpr uint32_t max_pending_upload_bytes = 8 * 1024 * 1024;
	// Layers use a fixed format rather than the swapchain's, so that pipelines do not depend on the window. The
	// conversion to whatever the swapchain wants happens once per frame, in the final blit.
	static constexpr SDL_GPUTextureFormat layer_format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
	// The format of the pixels RmlUi hands to GenerateTexture(). Deliberately separate from layer_format: the two
	// happen to agree today, but one describes what the application uploads and the other what the renderer draws into.
	static constexpr SDL_GPUTextureFormat content_format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
	// Postprocess targets, used as both input and output when moving a layer's contents around. Four of them: filters
	// read and write the first two in turn, a blurred drop shadow needs a third while its own blur runs, and the
	// fourth holds the image saved by SaveLayerAsMaskImage().
	static constexpr int num_postprocess_targets = 4;
	// Mirrors MAX_NUM_STOPS in RmlUi_SDL_GPU/shader_common.hlsli. A gradient with more stops than this is truncated,
	// as it is in every other backend.
	static constexpr int max_num_stops = 16;
	// Stop positions are packed four to a row of the constant buffer; see the shader for why.
	static constexpr int max_num_stops_packed = (max_num_stops + 3) / 4;
	// The blur kernel, mirroring BLUR_SIZE in RmlUi_SDL_GPU/shader_common.hlsli. The weights are symmetric, so only
	// half of them plus the centre are sent.
	static constexpr int blur_size = 7;
	static constexpr int blur_num_weights = (blur_size + 1) / 2;
	// How deep clip masks may nest before the stencil runs out of room. Every Intersect raises the mask by one, so a
	// generation has to keep this much space above it; past that the value clamps and the mask stops narrowing.
	static constexpr int max_clip_mask_depth = 64;
	// The clip mask is not cleared between masks; each one takes a stencil value never used before instead. The
	// buffer holds eight bits, so the supply runs out and the stencil has to be cleared for real. A Set arriving at
	// or above this asks for that clear first, which is what keeps the reserve above it free.
	static constexpr int max_stencil_generation = 0xFF - max_clip_mask_depth;

	struct Buffer {
		SDL_GPUTransferBuffer* transfer_buffer = nullptr;
		SDL_GPUBuffer* buffer = nullptr;
		uint32_t capacity = 0;
		int bucket = 0;
		int last_used_frame = 0;
		// Index into pending_uploads, or -1 when this buffer has no upload queued.
		int pending_upload = -1;
		// Scratch flag used by Trim() to mark a buffer for removal.
		bool expired = false;
	};

	// A pool of buffers of a single usage type. A buffer lives until it has sat unused in a free list for
	// buffer_retention_frames, at which point Trim() destroys it; acquiring and releasing only move it in and out of
	// the per-bucket free lists. Raw Buffer pointers held elsewhere stay valid only for as long as that holds, which is
	// why Trim() exempts buffers with an upload still queued in pending_uploads.
	class BufferPool {
	public:
		void Initialize(SDL_GPUDevice* device, SDL_GPUBufferUsageFlags usage, const char* debug_name);
		void ReleaseAll();

		Buffer* Acquire(uint32_t size, int frame);
		void Release(Buffer* buffer);
		// Releases free buffers that have not been used recently.
		void Trim(int frame);

	private:
		SDL_GPUDevice* device = nullptr;
		SDL_GPUBufferUsageFlags usage = {};
		const char* debug_name = nullptr;

		Rml::Vector<Rml::UniquePtr<Buffer>> buffers;
		Rml::Vector<Buffer*> free_lists[num_buffer_buckets];
	};

	struct GeometryView {
		Buffer* vertex_buffer = nullptr;
		Buffer* index_buffer = nullptr;
		int num_indices = 0;
	};

	// Must match the definitions at the top of shader_frag_gradient.frag.
	enum class ShaderGradientFunction { Linear, Radial, Conic, RepeatingLinear, RepeatingRadial, RepeatingConic };

	/*
	    A shader RmlUi has asked for and will render with later. Only the parameters are kept; the GPU-side work
	    happens in RenderShader().
	*/
	enum class CompiledShaderType { Invalid = 0, Gradient, Creation };
	struct CompiledShader {
		CompiledShaderType type = CompiledShaderType::Invalid;

		// Gradient.
		ShaderGradientFunction gradient_function = ShaderGradientFunction::Linear;
		Rml::Vector2f p;
		Rml::Vector2f v;
		Rml::Vector<float> stop_positions;
		Rml::Vector<Rml::Colourf> stop_colors;

		// Creation.
		Rml::Vector2f dimensions;
	};

	enum class FilterType : uint8_t { Invalid = 0, Passthrough, Blur, DropShadow, ColorMatrix, MaskImage };
	struct CompiledFilter {
		FilterType type = FilterType::Invalid;

		// Passthrough.
		float blend_factor = 0.f;

		// Blur, and the blur inside a drop shadow.
		float sigma = 0.f;

		// Drop shadow.
		Rml::Vector2f offset;
		Rml::ColourbPremultiplied color;

		// Color matrix.
		Rml::Matrix4f color_matrix;
	};

	/*
	    HLSL packs a constant buffer into 16-byte rows: a member never straddles a row boundary, and an array element
	    always starts one. Get this wrong and nothing fails loudly -- the shader compiles, the draw goes through, and
	    the values are read from the wrong offsets. The static_asserts below check what can be checked.
	*/
	struct GradientUniforms {
		Rml::Vector2f p;
		Rml::Vector2f v;
		int func;
		int num_stops;
		float padding[2];
		Rml::Colourf stop_colors[max_num_stops];
		float stop_positions[max_num_stops_packed * 4];
	};
	static_assert(sizeof(Rml::Colourf) == 16, "A colour is expected to be one row of a constant buffer");
	static_assert(sizeof(GradientUniforms) == 32 + 16 * max_num_stops + 16 * max_num_stops_packed,
		"GradientUniforms does not match the constant buffer rows of shader_frag_gradient.frag");

	struct CreationUniforms {
		Rml::Vector2f dimensions;
		float value;
		float padding;
	};
	static_assert(sizeof(CreationUniforms) == 16, "CreationUniforms does not match the constant buffer of shader_frag_creation.frag");

	struct ColorMatrixUniforms {
		Rml::Matrix4f color_matrix;
	};
	static_assert(sizeof(ColorMatrixUniforms) == 64, "ColorMatrixUniforms does not match the constant buffer of shader_frag_color_matrix.frag");

	struct BlurUniforms {
		// One row: the shader declares this a float4 and indexes it, so the weights have to be four to a row.
		float weights[blur_num_weights];
		Rml::Vector2f tex_coord_min;
		Rml::Vector2f tex_coord_max;
	};
	static_assert(blur_num_weights == 4, "The blur weights are sent as one float4 row, which fits exactly four of them");
	static_assert(sizeof(BlurUniforms) == 32, "BlurUniforms does not match the constant buffer of shader_frag_blur.frag");

	// The offsets the blur samples at are the same across a row, so they are computed once per vertex. Padded to a
	// full row: that is the size the shader's constant buffer has, whatever it reads of it.
	struct BlurVertexUniforms {
		Rml::Vector2f texel_offset;
		float padding[2];
	};
	static_assert(sizeof(BlurVertexUniforms) == 16, "BlurVertexUniforms does not match the constant buffer of shader_vert_blur.vert");

	struct DropShadowUniforms {
		Rml::Vector2f tex_coord_min;
		Rml::Vector2f tex_coord_max;
		Rml::Colourf color;
	};
	static_assert(sizeof(DropShadowUniforms) == 32, "DropShadowUniforms does not match the constant buffer of shader_frag_drop_shadow.frag");

	// A buffer upload waiting for the next copy pass.
	struct PendingUpload {
		Buffer* buffer = nullptr;
		uint32_t size = 0;
	};

	// A colour texture that can be both rendered into and sampled from.
	struct RenderTarget {
		SDL_GPUTexture* color = nullptr;
		int width = 0;
		int height = 0;
	};

	/*
	    Owns the render targets. Layers form a stack: geometry is drawn to the top one, and compositing moves a layer's
	    contents onto another. Layer zero is the base layer, whose contents become the frame.

	    Targets are reused rather than destroyed on pop, so pushing and popping repeatedly costs nothing after the first
	    few frames. Everything is recreated when the window size changes.

	    Postprocess targets are separate from the stack and are created on first use. Compositing borrows one whenever
	    a layer cannot be sampled straight into its destination; filters will read and write them in turn.
	*/
	class RenderLayerStack {
	public:
		void Initialize(SDL_GPUDevice* device);
		void ReleaseAll();

		// Recreates the targets if the size changed, then makes the base layer current.
		void BeginFrame(int width, int height);
		void EndFrame();

		Rml::LayerHandle PushLayer();
		void PopLayer();

		const RenderTarget& GetLayer(Rml::LayerHandle layer) const;
		const RenderTarget& GetTopLayer() const;
		Rml::LayerHandle GetTopLayerHandle() const;
		// The layer holding the frame. Unlike GetLayer(0) this does not require the stack to be non-empty, so the
		// result can still be read after EndFrame() has popped the base layer. Null before the first frame.
		const RenderTarget* GetBaseLayer() const { return layers.empty() ? nullptr : &layers[0]; }

		const RenderTarget& GetPostprocessPrimary() { return EnsurePostprocess(0); }
		const RenderTarget& GetPostprocessSecondary() { return EnsurePostprocess(1); }
		const RenderTarget& GetPostprocessTertiary() { return EnsurePostprocess(2); }
		const RenderTarget& GetBlendMask() { return EnsurePostprocess(3); }
		// Exchanges the two targets the filter chain reads and writes. References taken from the two accessors above
		// name the other target afterwards, so anything holding one has to take it again.
		void SwapPostprocessPrimarySecondary();

		SDL_GPUTexture* GetDepthStencil() const { return depth_stencil; }
		SDL_GPUTextureFormat GetDepthStencilFormat() const { return depth_stencil_format; }

		int GetWidth() const { return width; }
		int GetHeight() const { return height; }

	private:
		const RenderTarget& EnsurePostprocess(int index);
		bool CreateTarget(RenderTarget& target, const char* debug_name);
		void DestroyTargets();

		SDL_GPUDevice* device = nullptr;
		int width = 0;
		int height = 0;

		// The number of layers currently in use. The targets themselves are kept around for reuse.
		int layers_size = 0;
		Rml::Vector<RenderTarget> layers;
		// A fixed array rather than a growing vector: callers hold on to several of these references at once, which a
		// reallocation would invalidate.
		RenderTarget postprocess[num_postprocess_targets];

		SDL_GPUTexture* depth_stencil = nullptr;
		// INVALID while there is no stencil buffer to go with it. Kept apart from what the device supports so that a
		// failed allocation only disables clip masks until the next rebuild, not for the life of the renderer.
		SDL_GPUTextureFormat depth_stencil_format = SDL_GPU_TEXTUREFORMAT_INVALID;
		SDL_GPUTextureFormat supported_depth_stencil_format = SDL_GPU_TEXTUREFORMAT_INVALID;
	};

	enum class ShaderId : uint8_t {
		VertMain,
		VertPassthrough,
		VertBlur,
		FragColor,
		FragTexture,
		FragGradient,
		FragCreation,
		FragPassthrough,
		FragColorMatrix,
		FragBlendMask,
		FragBlur,
		FragDropShadow,
		Count,
	};
	static constexpr int num_shaders = static_cast<int>(ShaderId::Count);

	enum class ProgramId : uint8_t {
		// Programs that draw geometry submitted by RmlUi.
		Color,
		Texture,
		Gradient,
		Creation,
		// Programs that draw a quad over a region of a render target, used for compositing and filtering.
		Passthrough,
		ColorMatrix,
		BlendMask,
		Blur,
		DropShadow,
		Count,
	};

	// The pair of shaders a program is built from.
	struct ProgramShaders {
		ShaderId vertex = ShaderId::VertMain;
		ShaderId fragment = ShaderId::FragColor;
	};

	// Rml::BlendMode covers the first two; Constant scales the source instead of blending it, which is the opacity
	// filter.
	enum class Blending : uint8_t { Blend, Replace, Constant };

	enum class StencilMode : uint8_t {
		Off,            // No stencil test.
		TestEqual,      // Draw only where the stencil holds the reference value.
		WriteSet,       // Write the reference value over the drawn area.
		WriteIntersect, // Raise the drawn area by one where it already holds the reference, leaving the rest behind.
	};

	// Pipelines in SDL GPU are immutable, so every combination of program and state needs its own object. They are
	// built on demand and kept until shutdown; the set is small and bounded.
	struct PipelineKey {
		ProgramId program = ProgramId::Color;
		Blending blend = Blending::Blend;
		StencilMode stencil = StencilMode::Off;

		bool operator==(const PipelineKey& other) const
		{
			return program == other.program && blend == other.blend && stencil == other.stencil;
		}
	};
	struct PipelineEntry {
		PipelineKey key;
		// Null when creation failed. The entry is kept so that the failure is not retried, and logged, on every draw.
		SDL_GPUGraphicsPipeline* pipeline = nullptr;
	};

	// Shaders are built the first time a program needs them, so that the ones a document never reaches for cost
	// nothing. Null is returned, and logged once, if creation fails.
	// What a Blending and a StencilMode mean to SDL, kept apart from the pipeline they are built into: both are
	// tables of the API's own constants, and read as such once they are on their own.
	static SDL_GPUColorTargetBlendState GetBlendState(Blending blend, bool writes_stencil);
	static SDL_GPUDepthStencilState GetDepthStencilState(StencilMode stencil, bool writes_stencil);

	SDL_GPUShader* GetShader(ShaderId id);
	void ReleaseShaders();
	static ProgramShaders GetProgramShaders(ProgramId program);
	SDL_GPUGraphicsPipeline* GetPipeline(ProgramId program, Blending blend, StencilMode stencil);
	void ReleasePipelines();

	// Opens a render pass on the given target, ending the one in progress if it belongs to another target, or if
	// something is to be cleared. Pass state is invalidated whenever a new pass begins.
	bool EnsureRenderPass(const RenderTarget& target, bool clear_color = false, bool clear_stencil = false);
	void EndRenderPass();

	// Every transfer is recorded into a command buffer of its own rather than the frame's. That keeps the frame's
	// render pass open from the first draw to the last, and lets textures be generated outside of a frame. Ordering is
	// what makes it safe: the upload command buffer is submitted before the frame's, so its copies have run by the
	// time any draw reads from them.
	//
	// SDL GPU command buffers are thread-affine, and this one is acquired in CompileGeometry()/GenerateTexture() but
	// only submitted in EndFrame(), so every call into this interface must come from the same thread.
	bool EnsureUploadPass();
	void SubmitUploads();

	// Queues a buffer upload to be recorded by the next FlushGeometryUploads().
	void QueueUpload(Buffer* buffer, uint32_t size);
	// Drops a queued upload whose staging data turned out not to be written.
	void CancelUpload(Buffer* buffer);
	// Records every upload queued since the last flush.
	bool FlushGeometryUploads();

	// The state a single draw needs on top of its geometry. Every field is compared against what the open pass
	// already carries, so repeating a draw with the same state costs no more than the draw itself.
	struct DrawState {
		ProgramId program = ProgramId::Color;
		Blending blend = Blending::Blend;
		// The factor the source is scaled by under Blending::Constant.
		float blend_constant = 1.f;
		StencilMode stencil = StencilMode::Off;
		uint8_t stencil_reference = 0;
		SDL_GPUTexture* texture = nullptr;
		// The second texture of the blend mask program, and null for every other one.
		SDL_GPUTexture* mask_texture = nullptr;
		// Null to sample with the renderer's default sampler, which repeats. The postprocess passes clamp instead:
		// they offset and scale texture coordinates, and a wrapped sample would come back from the far edge.
		SDL_GPUSampler* sampler = nullptr;
		// Null to use the transform RmlUi last set. The renderer's own target-sized quads are in target coordinates
		// and pass the plain projection instead. Only read by the programs whose vertex stage takes a transform.
		const Rml::Matrix4f* transform = nullptr;
		Rml::Vector2f translation;
		// The constant buffer of a program that takes one, or null. Pushed on every draw rather than cached: this is
		// per-draw data, and comparing it would cost more than sending it.
		const void* fragment_uniforms = nullptr;
		uint32_t fragment_uniforms_size = 0;
		// The vertex stage constant buffer of the blur program, which takes one in place of the transform.
		const void* vertex_uniforms = nullptr;
		uint32_t vertex_uniforms_size = 0;
	};
	// Issues a draw into the open render pass. The pass must already be open on the wanted target. Returns false if
	// the draw could not be issued, which matters to callers that record what it was meant to leave behind.
	bool DrawGeometry(const GeometryView& geometry, const DrawState& state);

	// The stencil mode geometry submitted by RmlUi is drawn with.
	StencilMode GetClipMaskMode() const;
	// Whether clip masks can be drawn at all. Both the stencil buffer and the pipelines that use it can fail to be
	// created; either way masks are dropped and content is drawn unmasked, rather than tested against a mask that
	// was never written and so culled altogether.
	bool HasStencil() const;

	/*
	    Draws a quad over `region` of the destination, in destination pixels, with texture coordinates running from
	    `uv_offset` to `uv_offset + uv_scaling` across it. This is the one primitive every compositing and filtering
	    pass is built from: they differ in program, in which rectangle of which target they cover, and in where they
	    read from. The active scissor still applies and is what confines a pass to the region being composited.

	    A quad covering the whole target with plain texture coordinates is kept ready; anything else is built for the
	    draw and released again.
	*/
	bool DrawPostprocessQuad(const RenderTarget& destination, const DrawState& state, Rml::Rectanglei region, Rml::Vector2f uv_offset,
		Rml::Vector2f uv_scaling);
	bool DrawPostprocessQuad(const RenderTarget& destination, const DrawState& state);
	// Draws a texture over the whole of the destination target, honouring the active scissor and, where asked for,
	// the clip mask. Returns false if the draw could not be issued, which matters to the postprocess path: its second
	// hop reads what the first one was meant to leave behind.
	bool DrawTextureToTarget(const RenderTarget& destination, SDL_GPUTexture* source, Blending blend, StencilMode stencil = StencilMode::Off);
	// Draws the `source` rectangle of a target-sized texture onto the `destination` rectangle, scaling and filtering
	// as needed. Stands in for the framebuffer blits the other backends upscale a blurred image with, which cannot be
	// used here: SDL's blit takes no scissor, and the second of the two upscales relies on one.
	void BlitRegion(const RenderTarget& destination, const RenderTarget& source, Rml::Rectanglei source_region,
		Rml::Rectanglei destination_region);
	// Overwrites the active scissor region of the current target with transparent black.
	void ClearScissorRegion();
	// Opens a pass on the target and clears the active scissor region of it.
	void ClearRegion(const RenderTarget& target);
	// Rebuilds the quads used for compositing and clearing when the target size changes.
	bool EnsureQuads(int width, int height);
	void ReleaseQuads();

	// Runs the filter chain over the postprocess targets. The input is the primary target and, once this returns, so
	// is the output: each filter reads the primary, writes the secondary and swaps the two.
	void RenderFilters(Rml::Span<const Rml::CompiledFilterHandle> filters);
	// Blurs the `window` region of `source_destination` in place, using `temp` as scratch. Large radii are blurred at
	// a reduced resolution and scaled back up, since the kernel is a fixed seven samples wide.
	void RenderBlur(float sigma, const RenderTarget& source_destination, const RenderTarget& temp, Rml::Rectanglei window);

	SDL_GPUTexture* CreateTexture(int width, int height, SDL_GPUTextureFormat format, SDL_GPUTextureUsageFlags usage, const char* debug_name);

	void ApplyScissor();
	void InvalidateRenderPassState();
	// The scissor as RmlUi submitted it, without the clamp to the render target. This is the rectangle RmlUi records
	// the size of a saved layer texture from, so SaveLayerAsTexture() has to agree with it.
	Rml::Rectanglei GetScissorRegion() const;
	// The scissor clamped to the active target, as SDL requires.
	Rml::Rectanglei GetActiveScissor() const;
	// Takes the scissor away from RmlUi for the duration of a postprocess pass. The blur works on regions of its own
	// -- halved once per downscale, widened by a pixel to clear a border -- which have nothing to do with the region
	// RmlUi asked to draw into, and which have to be put back when the pass is over.
	void SetScissorOverride(Rml::Rectanglei region);
	void ClearScissorOverride();

	SDL_GPUDevice* device = nullptr;

	SDL_GPUShader* shaders[num_shaders] = {};
	// Set once creation of a shader has failed, so that the failure is not retried on every draw.
	bool shader_failed[num_shaders] = {};
	Rml::Vector<PipelineEntry> pipelines;
	// Whether the cached pipelines were built with a depth/stencil attachment, so that they can be rebuilt if the
	// availability of one changes. A pipeline whose target layout disagrees with the pass is invalid.
	SDL_GPUTextureFormat pipelines_depth_stencil_format = SDL_GPU_TEXTUREFORMAT_INVALID;
	// Set when a pipeline that uses the stencil buffer could not be built, which takes clip masks out of service
	// until the cache is rebuilt. Testing against a mask whose writing pipeline is missing would hide the document.
	bool stencil_pipelines_failed = false;

	// Repeats, which is what a texture RmlUi hands over expects. The postprocess passes use the clamping one: they
	// sample outside the image on purpose, and a repeated sample would return colours from the opposite edge.
	SDL_GPUSampler* linear_sampler = nullptr;
	SDL_GPUSampler* clamp_sampler = nullptr;

	// Frame state, valid between BeginFrame() and EndFrame().
	SDL_GPUCommandBuffer* command_buffer = nullptr;
	SDL_GPUTexture* swapchain_texture = nullptr;
	uint32_t swapchain_width = 0;
	uint32_t swapchain_height = 0;
	int frame_index = 0;
	// Set by BeginFrame() and cleared by EndFrame(). The backend skips both calls for a frame it cannot present, but
	// still calls EndFrame(), which must then leave the layer stack alone.
	bool frame_active = false;

	SDL_GPURenderPass* render_pass = nullptr;
	// The colour texture the open render pass draws into, or nullptr when no pass is open. Identified by texture
	// rather than by RenderTarget address, since the targets live in a vector that grows.
	SDL_GPUTexture* active_target_texture = nullptr;

	// Cached render pass state, so that redundant bindings and uniform pushes can be skipped. Bindings and scissor are
	// pass state and do not survive across passes; the uniforms belong to the command buffer, but re-pushing them
	// along with the rest is cheap and keeps a single invalidation point.
	SDL_GPUGraphicsPipeline* bound_pipeline = nullptr;
	SDL_GPUTexture* bound_texture = nullptr;
	SDL_GPUTexture* bound_mask_texture = nullptr;
	SDL_GPUSampler* bound_sampler = nullptr;
	SDL_GPUBuffer* bound_vertex_buffer = nullptr;
	SDL_GPUBuffer* bound_index_buffer = nullptr;
	Rml::Vector2f pushed_translation;
	bool transform_dirty = true;
	bool translation_dirty = true;
	bool scissor_dirty = true;
	float applied_blend_constant = 0.f;
	bool blend_constant_dirty = true;

	bool scissor_enabled = false;
	Rml::Rectanglei scissor_region;
	SDL_Rect applied_scissor = {};
	// While set, stands in for the scissor RmlUi asked for; see SetScissorOverride().
	bool scissor_override_active = false;
	Rml::Rectanglei scissor_override;

	// Clip mask state. Rather than clearing the stencil buffer for every mask, which SDL GPU can only do by breaking
	// the render pass, each mask claims a stencil value that has never been written since the last real clear.
	// stencil_high_water is an upper bound on what the buffer holds, so the next mask can pick a value above it and
	// know that nothing else can match.
	bool clip_mask_enabled = false;
	uint8_t stencil_high_water = 0;
	uint8_t stencil_test_value = 0;
	// Reported once: past the reserve the mask can no longer be narrowed, and the artefact is otherwise silent.
	bool stencil_reserve_exhausted = false;
	uint8_t applied_stencil_reference = 0;
	bool stencil_reference_dirty = true;

	Rml::Matrix4f transform;
	Rml::Matrix4f projection;

	BufferPool vertex_buffers;
	BufferPool index_buffers;
	Rml::Vector<PendingUpload> pending_uploads;

	SDL_GPUCommandBuffer* upload_command_buffer = nullptr;
	SDL_GPUCopyPass* upload_copy_pass = nullptr;
	uint32_t pending_upload_bytes = 0;

	RenderLayerStack render_layers;

	// A quad covering the whole target in clip space, which the compositing and filtering passes draw with, and a
	// transparent one in target coordinates for clearing a region.
	Rml::CompiledGeometryHandle fullscreen_quad = {};
	Rml::CompiledGeometryHandle clear_quad = {};
	int quad_width = 0;
	int quad_height = 0;

	bool shutdown_complete = false;
};
