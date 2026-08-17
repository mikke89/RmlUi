#pragma once

#include <RmlUi/Core/RenderInterface.h>
#include <RmlUi/Core/Types.h>
#include <SDL3/SDL.h>

class RenderInterface_SDL_GPU : public Rml::RenderInterface {
public:
	RenderInterface_SDL_GPU(SDL_GPUDevice* device, SDL_Window* window);
	~RenderInterface_SDL_GPU() override;

	// Releases all GPU resources owned by the renderer. Called automatically by the destructor, and safe to call
	// multiple times.
	void Shutdown();

	// Prepares the renderer to take rendering commands from RmlUi.
	void BeginFrame(SDL_GPUCommandBuffer* command_buffer, SDL_GPUTexture* swapchain_texture, uint32_t width, uint32_t height);
	// Ends any active pass. The command buffer is submitted by the caller.
	void EndFrame();

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

private:
	// Buffers are handed out in power-of-two sized buckets so that the same allocations can be reused across frames
	// instead of creating a new buffer for every distinct mesh size.
	static constexpr uint32_t min_buffer_size = 4096;
	static constexpr int num_buffer_buckets = 20;
	// Free buffers that have not been used for this many frames are released back to the driver.
	static constexpr int buffer_retention_frames = 120;
	// Transfers are batched into a single command buffer until this much data has been queued.
	static constexpr uint32_t max_pending_upload_bytes = 8 * 1024 * 1024;

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

	void CreatePipelines();

	bool EnsureRenderPass();
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

	void ApplyScissor();
	void InvalidateRenderPassState();

	SDL_GPUDevice* device = nullptr;
	SDL_Window* window = nullptr;

	SDL_GPUGraphicsPipeline* texture_pipeline = nullptr;
	SDL_GPUGraphicsPipeline* color_pipeline = nullptr;
	SDL_GPUSampler* linear_sampler = nullptr;

	// Frame state, valid between BeginFrame() and EndFrame().
	SDL_GPUCommandBuffer* command_buffer = nullptr;
	SDL_GPUTexture* swapchain_texture = nullptr;
	uint32_t swapchain_width = 0;
	uint32_t swapchain_height = 0;
	int frame_index = 0;

	SDL_GPURenderPass* render_pass = nullptr;

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

	bool shutdown_complete = false;
};
