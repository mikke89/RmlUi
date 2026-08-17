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

	void SetTransform(const Rml::Matrix4f* new_transform) override;

	Rml::LayerHandle PushLayer() override;
	void CompositeLayers(Rml::LayerHandle source, Rml::LayerHandle destination, Rml::BlendMode blend_mode,
		Rml::Span<const Rml::CompiledFilterHandle> filters) override;
	void PopLayer() override;

	Rml::TextureHandle SaveLayerAsTexture() override;

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
	// Postprocess targets, used as both input and output when moving a layer's contents around.
	static constexpr int num_postprocess_targets = 2;

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
		SDL_GPUTextureFormat depth_stencil_format = SDL_GPU_TEXTUREFORMAT_INVALID;
	};

	enum class ProgramId { Color, Texture };

	// Pipelines in SDL GPU are immutable, so every combination of program and state needs its own object. They are
	// built on demand and kept until shutdown; the set is small and bounded.
	struct PipelineKey {
		ProgramId program = ProgramId::Color;
		Rml::BlendMode blend = Rml::BlendMode::Blend;

		bool operator==(const PipelineKey& other) const { return program == other.program && blend == other.blend; }
	};
	struct PipelineEntry {
		PipelineKey key;
		// Null when creation failed. The entry is kept so that the failure is not retried, and logged, on every draw.
		SDL_GPUGraphicsPipeline* pipeline = nullptr;
	};

	bool CreateShaders();
	SDL_GPUGraphicsPipeline* GetPipeline(ProgramId program, Rml::BlendMode blend);
	void ReleasePipelines();

	// Opens a render pass on the given target, ending the one in progress if it belongs to another target. Pass state
	// is invalidated whenever a new pass begins.
	bool EnsureRenderPass(const RenderTarget& target, bool clear = false);
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

	// Draws a texture over the whole of the destination target, honouring the active scissor.
	void DrawTextureToTarget(const RenderTarget& destination, SDL_GPUTexture* source, Rml::BlendMode blend);
	// Overwrites the active scissor region of the current target with transparent black.
	void ClearScissorRegion();
	// Rebuilds the quads used for compositing and clearing when the target size changes.
	bool EnsureQuads(int width, int height);
	void ReleaseQuads();

	SDL_GPUTexture* CreateTexture(int width, int height, SDL_GPUTextureFormat format, SDL_GPUTextureUsageFlags usage, const char* debug_name);

	void ApplyScissor();
	void InvalidateRenderPassState();
	// The scissor as RmlUi submitted it, without the clamp to the render target. This is the rectangle RmlUi records
	// the size of a saved layer texture from, so SaveLayerAsTexture() has to agree with it.
	Rml::Rectanglei GetScissorRegion() const;
	// The scissor clamped to the active target, as SDL requires.
	Rml::Rectanglei GetActiveScissor() const;

	SDL_GPUDevice* device = nullptr;

	SDL_GPUShader* color_shader = nullptr;
	SDL_GPUShader* texture_shader = nullptr;
	SDL_GPUShader* vertex_shader = nullptr;
	Rml::Vector<PipelineEntry> pipelines;
	// Whether the cached pipelines were built with a depth/stencil attachment, so that they can be rebuilt if the
	// availability of one changes. A pipeline whose target layout disagrees with the pass is invalid.
	SDL_GPUTextureFormat pipelines_depth_stencil_format = SDL_GPU_TEXTUREFORMAT_INVALID;

	SDL_GPUSampler* linear_sampler = nullptr;

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
	SDL_GPUBuffer* bound_vertex_buffer = nullptr;
	SDL_GPUBuffer* bound_index_buffer = nullptr;
	Rml::Vector2f pushed_translation;
	bool transform_dirty = true;
	bool translation_dirty = true;
	bool scissor_dirty = true;

	bool scissor_enabled = false;
	Rml::Rectanglei scissor_region;
	SDL_Rect applied_scissor = {};

	Rml::Matrix4f transform;
	Rml::Matrix4f projection;

	BufferPool vertex_buffers;
	BufferPool index_buffers;
	Rml::Vector<PendingUpload> pending_uploads;

	SDL_GPUCommandBuffer* upload_command_buffer = nullptr;
	SDL_GPUCopyPass* upload_copy_pass = nullptr;
	uint32_t pending_upload_bytes = 0;

	RenderLayerStack render_layers;

	// Target-sized quads: one white for compositing a texture, one transparent for clearing a region.
	Rml::CompiledGeometryHandle composite_quad = {};
	Rml::CompiledGeometryHandle clear_quad = {};
	int quad_width = 0;
	int quad_height = 0;

	bool shutdown_complete = false;
};
