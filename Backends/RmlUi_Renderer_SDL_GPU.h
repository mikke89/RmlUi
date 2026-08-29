#pragma once

#include <RmlUi/Core/RenderInterface.h>
#include <RmlUi/Core/Types.h>
#include <SDL3/SDL.h>

// How many samples the layers are rendered with; one turns multisampling off. A device that does not offer exactly
// this many gets the nearest it does, preferring more to fewer. See SelectSampleCount().
#ifndef RMLUI_SDL_GPU_NUM_MSAA_SAMPLES
	#define RMLUI_SDL_GPU_NUM_MSAA_SAMPLES 2
#endif

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
	static constexpr uint32_t geometry_block_size = 1024 * 1024;
	// Space given up by a released mesh is held back this long: the GPU may still be reading it, and a range inside a
	// shared block cannot be cycled the way a buffer of its own could. Two frames may be in flight, so the third is safe.
	static constexpr int frames_before_reuse = 3;
	static constexpr int geometry_retention_frames = 120;
	static constexpr uint32_t max_pending_upload_bytes = 8 * 1024 * 1024;
	static constexpr SDL_GPUTextureFormat layer_format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
	static constexpr SDL_GPUTextureFormat content_format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
	// Filters read and write the first two in turn, a blurred drop shadow needs a third while its own blur runs, and
	// the fourth holds the image saved by SaveLayerAsMaskImage().
	static constexpr int num_postprocess_targets = 4;
	static constexpr int max_num_stops = 16;
	static constexpr int max_num_stops_packed = (max_num_stops + 3) / 4;
	// Mirrors BLUR_SIZE in RmlUi_SDL_GPU/shader_common.hlsli. The weights are symmetric, so only half plus the centre
	// are sent.
	static constexpr int blur_size = 7;
	static constexpr int blur_num_weights = (blur_size + 1) / 2;
	// How deep clip masks may nest before the stencil runs out of room. Every Intersect raises the mask by one, so a
	// generation has to keep this much space above it; past that the value clamps and the mask stops narrowing.
	static constexpr int max_clip_mask_depth = 64;
	// The clip mask is not cleared between masks; each one takes a stencil value never used before instead. The
	// buffer holds eight bits, so the supply runs out and the stencil has to be cleared for real. A Set arriving at
	// or above this asks for that clear first, which is what keeps the reserve above it free.
	static constexpr int max_stencil_generation = 0xFF - max_clip_mask_depth;

	/*
	    A transfer buffer, together with the awkward half of keeping one: a transfer buffer cannot be resized, so
	    growing means letting the old one go and creating another. Both the geometry uploads and the texture uploads
	    need that much; how large the new buffer should be they answer differently, and each works it out for itself.
	*/
	struct TransferBuffer {
		// Replaces the buffer with one of `new_capacity` bytes. Whatever was written into the old one goes with it,
		// hence `used` starting over. False if no buffer could be created, leaving none at all.
		bool Recreate(SDL_GPUDevice* device, uint32_t new_capacity);
		void Release(SDL_GPUDevice* device);

		SDL_GPUTransferBuffer* buffer = nullptr;
		uint32_t capacity = 0;
		// How much of it the writes since the last cycle have taken, for a caller that hands out slices rather than
		// writing from the start.
		uint32_t used = 0;
	};

	/*
	    A few large GPU buffers with the meshes placed inside them, rather than a pair of buffers for every mesh: a
	    draw reaches its own mesh through the first_index and vertex_offset arguments, and the buffers are bound once
	    and stay bound. Binding is what this avoids -- SDL records the resource against the command buffer each time.

	    Sizes and offsets are counted in units, one vertex or one index, since that is what a draw call addresses in.
	*/
	class GeometryArena {
	public:
		struct Range {
			uint32_t offset = 0;
			uint32_t size = 0;
		};
		struct Block {
			SDL_GPUBuffer* buffer = nullptr;
			uint32_t capacity = 0;
			Rml::Vector<Range> free_ranges;
			int last_used_frame = 0;
		};
		struct Allocation {
			Block* block = nullptr;
			uint32_t offset = 0;
			uint32_t size = 0;
		};

		void Initialize(SDL_GPUDevice* device, SDL_GPUBufferUsageFlags usage, uint32_t unit_size, const char* debug_name);
		void ReleaseAll();

		bool Allocate(const void* data, uint32_t num_units, int frame, Allocation& out_allocation);
		void Free(const Allocation& allocation, int frame);
		void BeginFrame(int frame);
		bool Flush(SDL_GPUCopyPass* copy_pass);
		uint32_t GetPendingBytes() const { return static_cast<uint32_t>(staging.size()); }

	private:
		struct PendingUpload {
			Block* block = nullptr;
			uint32_t offset = 0;
			uint32_t size = 0;
			size_t staging_offset = 0;
		};
		struct PendingFree {
			Allocation allocation;
			int frame = 0;
		};

		Block* CreateBlock(uint32_t capacity, int frame);
		static bool TakeRange(Block& block, uint32_t num_units, uint32_t& out_offset);
		void ReturnRange(Block& block, Range range);
		bool EnsureTransferBuffer(uint32_t byte_size);

		SDL_GPUDevice* device = nullptr;
		SDL_GPUBufferUsageFlags usage = {};
		uint32_t unit_size = 0;
		const char* debug_name = nullptr;

		Rml::Vector<Rml::UniquePtr<Block>> blocks;
		Rml::Vector<PendingUpload> pending_uploads;
		Rml::Vector<PendingFree> pending_frees;
		Rml::Vector<Rml::byte> staging;

		TransferBuffer transfers;
	};

	struct GeometryView {
		GeometryArena::Allocation vertices;
		GeometryArena::Allocation indices;
		int num_indices = 0;
	};

	enum class ShaderGradientFunction { Linear, Radial, Conic, RepeatingLinear, RepeatingRadial, RepeatingConic };

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

	struct QuadUniforms {
		float position[4];
		float tex_coord[4];
	};
	static_assert(sizeof(QuadUniforms) == 32, "QuadUniforms does not match the constant buffer of shader_vert_passthrough.vert");

	struct BlurVertexUniforms {
		QuadUniforms quad;
		Rml::Vector2f texel_offset;
		float padding[2];
	};
	static_assert(sizeof(BlurVertexUniforms) == 48, "BlurVertexUniforms does not match the constant buffer of shader_vert_blur.vert");

	struct DropShadowUniforms {
		Rml::Vector2f tex_coord_min;
		Rml::Vector2f tex_coord_max;
		Rml::Colourf color;
	};
	static_assert(sizeof(DropShadowUniforms) == 32, "DropShadowUniforms does not match the constant buffer of shader_frag_drop_shadow.frag");

	struct RenderTarget {
		SDL_GPUTexture* color = nullptr;
		int width = 0;
		int height = 0;
		SDL_GPUSampleCount sample_count = SDL_GPU_SAMPLECOUNT_1;
		// Only the layers attach it: the postprocess passes neither test nor write the clip mask, and once the layers
		// are multisampled the shared buffer no longer matches them in sample count anyway.
		bool use_depth_stencil = false;
	};

	/*
	    Layers form a stack: geometry is drawn to the top one, and layer zero holds the frame. Targets are reused
	    rather than destroyed on pop, and everything is recreated when the window size changes.

	    Postprocess targets are separate from the stack and created on first use. Compositing borrows one whenever a
	    layer cannot be sampled straight into its destination; filters read and write them in turn.
	*/
	class RenderLayerStack {
	public:
		void Initialize(SDL_GPUDevice* device);
		void ReleaseAll();

		void BeginFrame(int width, int height);
		void EndFrame();

		Rml::LayerHandle PushLayer();
		void PopLayer();

		const RenderTarget& GetLayer(Rml::LayerHandle layer) const;
		const RenderTarget& GetTopLayer() const;
		Rml::LayerHandle GetTopLayerHandle() const;
		const RenderTarget* GetBaseLayer() const { return layers.empty() ? nullptr : &layers[0]; }

		const RenderTarget& GetPostprocessPrimary() { return EnsurePostprocess(0); }
		const RenderTarget& GetPostprocessSecondary() { return EnsurePostprocess(1); }
		const RenderTarget& GetPostprocessTertiary() { return EnsurePostprocess(2); }
		const RenderTarget& GetBlendMask() { return EnsurePostprocess(3); }
		void SwapPostprocessPrimarySecondary();

		// Created on first use: a document that renders no clip mask never needs it, and on multisampled layers it is
		// megabytes. Null if it could not be created, which takes clip masks out of service until the next rebuild.
		SDL_GPUTexture* EnsureDepthStencil();
		SDL_GPUTextureFormat GetDepthStencilFormat() const { return depth_stencil_format; }
		SDL_GPUSampleCount GetSampleCount() const { return sample_count; }
		bool IsMultisampled() const { return sample_count != SDL_GPU_SAMPLECOUNT_1; }

		int GetWidth() const { return width; }
		int GetHeight() const { return height; }

	private:
		const RenderTarget& EnsurePostprocess(int index);
		bool CreateTarget(RenderTarget& target, const char* debug_name, bool is_layer);
		void DestroyTargets();

		SDL_GPUDevice* device = nullptr;
		int width = 0;
		int height = 0;
		SDL_GPUSampleCount sample_count = SDL_GPU_SAMPLECOUNT_1;

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

	struct PipelineKey {
		ProgramId program = ProgramId::Color;
		Blending blend = Blending::Blend;
		StencilMode stencil = StencilMode::Off;
		SDL_GPUSampleCount sample_count = SDL_GPU_SAMPLECOUNT_1;
		bool depth_stencil = false;

		bool operator==(const PipelineKey& other) const
		{
			return program == other.program && blend == other.blend && stencil == other.stencil &&
				sample_count == other.sample_count && depth_stencil == other.depth_stencil;
		}
	};
	struct PipelineEntry {
		PipelineKey key;
		SDL_GPUGraphicsPipeline* pipeline = nullptr;
	};

	// What a Blending and a StencilMode mean to SDL, kept apart from the pipeline they are built into: both are
	// tables of the API's own constants, and read as such once they are on their own.
	static SDL_GPUColorTargetBlendState GetBlendState(Blending blend, bool writes_stencil);
	static SDL_GPUDepthStencilState GetDepthStencilState(StencilMode stencil, bool writes_stencil);

	SDL_GPUShader* GetShader(ShaderId id);
	void ReleaseShaders();
	static ProgramShaders GetProgramShaders(ProgramId program);
	SDL_GPUGraphicsPipeline* GetPipeline(ProgramId program, Blending blend, StencilMode stencil, SDL_GPUSampleCount sample_count,
		bool depth_stencil);
	void WarmPipelineCache();
	void ReleasePipelines();

	bool EnsureRenderPass(const RenderTarget& target, bool clear_color = false, bool clear_stencil = false);
	void EndRenderPass();

	// Transfers go into a command buffer of their own rather than the frame's, which keeps the frame's render pass
	// open from the first draw to the last and lets textures be generated outside a frame. Ordering makes it safe:
	// the upload buffer is submitted first, so its copies have run before any draw reads them.
	//
	// SDL GPU command buffers are thread-affine, and this one is acquired in CompileGeometry()/GenerateTexture() but
	// submitted in EndFrame(), so every call into this interface must come from the same thread.
	bool EnsureUploadPass();
	void SubmitUploads();

	bool FlushGeometryUploads();

	struct DrawState {
		ProgramId program = ProgramId::Color;
		Blending blend = Blending::Blend;
		float blend_constant = 1.f;
		StencilMode stencil = StencilMode::Off;
		uint8_t stencil_reference = 0;
		SDL_GPUTexture* texture = nullptr;
		SDL_GPUTexture* mask_texture = nullptr;
		// Null to sample with the renderer's default sampler, which repeats. The postprocess passes clamp instead:
		// they offset and scale texture coordinates, and a wrapped sample would come back from the far edge.
		SDL_GPUSampler* sampler = nullptr;
		// Null to use the transform RmlUi last set. The renderer's own target-sized quad is in target coordinates and
		// passes the plain projection instead. Only read by the programs whose vertex stage takes a transform.
		const Rml::Matrix4f* transform = nullptr;
		Rml::Vector2f translation;
		// Compared against what was last pushed: gradients and colour matrices repeat unchanged across a document, and
		// a push costs more than the comparison.
		const void* fragment_uniforms = nullptr;
		uint32_t fragment_uniforms_size = 0;
		// Postprocess programs take one in place of the transform. The quad at the front is filled in by
		// DrawPostprocessQuad(); a caller sets this only to add what follows, today the blur's texel offset.
		const void* vertex_uniforms = nullptr;
		uint32_t vertex_uniforms_size = 0;
	};
	bool DrawGeometry(const GeometryView& geometry, const DrawState& state);

	StencilMode GetClipMaskMode() const;
	bool HasStencil() const;

	bool DrawPostprocessQuad(const RenderTarget& destination, const DrawState& state, Rml::Rectanglei region, Rml::Vector2f uv_offset,
		Rml::Vector2f uv_scaling);
	bool DrawPostprocessQuad(const RenderTarget& destination, const DrawState& state);
	bool DrawTextureToTarget(const RenderTarget& destination, SDL_GPUTexture* source, Blending blend, StencilMode stencil = StencilMode::Off);
	bool BlitLayerToPostprocessPrimary(const RenderTarget& layer);
	bool ResolveTarget(SDL_GPUCommandBuffer* in_command_buffer, const RenderTarget& source, const RenderTarget& destination,
		bool keep_samples);
	void BlitRegion(const RenderTarget& destination, const RenderTarget& source, Rml::Rectanglei source_region,
		Rml::Rectanglei destination_region);
	void ClearScissorRegion();
	void ClearRegion(const RenderTarget& target);
	bool EnsureQuads(int width, int height);
	void ReleaseQuads();

	void RenderFilters(Rml::Span<const Rml::CompiledFilterHandle> filters);
	void RenderBlur(float sigma, const RenderTarget& source_destination, const RenderTarget& temp, Rml::Rectanglei window);

	SDL_GPUTexture* CreateTexture(int width, int height, SDL_GPUTextureFormat format, SDL_GPUTextureUsageFlags usage, const char* debug_name);

	void ApplyScissor();
	void InvalidateRenderPassState();
	Rml::Rectanglei GetScissorRegion() const;
	Rml::Rectanglei GetActiveScissor() const;
	void SetScissorOverride(Rml::Rectanglei region);
	void ClearScissorOverride();

	SDL_GPUDevice* device = nullptr;
	SDL_Window* window = nullptr;

	SDL_GPUShader* shaders[num_shaders] = {};
	bool shader_failed[num_shaders] = {};
	Rml::Vector<PipelineEntry> pipelines;
	PipelineKey last_pipeline_key;
	SDL_GPUGraphicsPipeline* last_pipeline = nullptr;
	bool last_pipeline_valid = false;
	SDL_GPUTextureFormat pipelines_depth_stencil_format = SDL_GPU_TEXTUREFORMAT_INVALID;
	// Takes clip masks out of service until the cache is rebuilt: testing against a mask whose writing pipeline is
	// missing would hide the document.
	bool stencil_pipelines_failed = false;

	// Textures take slices of it one after another, and it is cycled only when the next no longer fits. That is what
	// makes writing to it safe while recorded uploads are still in flight.
	TransferBuffer texture_transfers;

	// The postprocess passes use the clamping one: they sample outside the image on purpose, and a repeated sample
	// would come back from the opposite edge.
	SDL_GPUSampler* linear_sampler = nullptr;
	SDL_GPUSampler* clamp_sampler = nullptr;

	SDL_GPUCommandBuffer* command_buffer = nullptr;
	SDL_GPUTexture* swapchain_texture = nullptr;
	uint32_t swapchain_width = 0;
	uint32_t swapchain_height = 0;
	int frame_index = 0;
	// Set by BeginFrame() and cleared by EndFrame(). The backend skips both calls for a frame it cannot present, but
	// still calls EndFrame(), which must then leave the layer stack alone.
	bool frame_active = false;
	// Whether EndFrame() left the finished frame in the postprocess primary target. It does when it had to resolve
	// there to reach the swapchain, and then it also drops the layer's samples -- so this is both what lets
	// CaptureScreen() read the frame without resolving again and what tells it that resolving again is not an option.
	bool frame_resolved_into_postprocess = false;

	// Everything RmlUi asked for and has not given back. RmlUi releases each of them during Rml::Shutdown(), which
	// clients call before shutting the backend down, so all four are zero by the time Shutdown() runs. A leak in a
	// client that drives the interface itself, or in the interface, shows up there as a number instead of going
	// unnoticed until the driver complains; a negative number is the same accounting seen from the other side.
	int live_geometry = 0;
	int live_textures = 0;
	int live_shaders = 0;
	int live_filters = 0;

	SDL_GPURenderPass* render_pass = nullptr;
	SDL_GPUTexture* active_target_texture = nullptr;
	SDL_GPUSampleCount active_sample_count = SDL_GPU_SAMPLECOUNT_1;
	bool active_depth_stencil = false;

	// So that redundant bindings and uniform pushes can be skipped. Bindings and scissor do not survive a pass; the
	// uniforms belong to the command buffer, but re-pushing them along with the rest keeps one invalidation point.
	SDL_GPUGraphicsPipeline* bound_pipeline = nullptr;
	SDL_GPUTexture* bound_texture = nullptr;
	SDL_GPUTexture* bound_mask_texture = nullptr;
	SDL_GPUSampler* bound_sampler = nullptr;
	SDL_GPUBuffer* bound_vertex_buffer = nullptr;
	SDL_GPUBuffer* bound_index_buffer = nullptr;
	Rml::Vector2f pushed_translation;
	Rml::byte pushed_fragment_uniforms[sizeof(GradientUniforms)] = {};
	uint32_t pushed_fragment_uniforms_size = 0;
	bool transform_dirty = true;
	bool translation_dirty = true;
	bool scissor_dirty = true;
	float applied_blend_constant = 0.f;
	bool blend_constant_dirty = true;

	bool scissor_enabled = false;
	Rml::Rectanglei scissor_region;
	SDL_Rect applied_scissor = {};
	bool scissor_override_active = false;
	Rml::Rectanglei scissor_override;

	// Clearing the stencil buffer for every mask would mean breaking the render pass, which is all SDL GPU offers, so
	// each mask claims a value never written since the last real clear. stencil_high_water bounds what the buffer
	// holds, and the next mask picks above it.
	bool clip_mask_enabled = false;
	// Until a mask has been rendered, passes carry no stencil attachment: every pass loads and stores it, which on a
	// multisampled layer is megabytes of traffic for a buffer nothing would read.
	bool frame_has_clip_mask = false;
	uint8_t stencil_high_water = 0;
	uint8_t stencil_test_value = 0;
	bool stencil_reserve_exhausted = false;
	uint8_t applied_stencil_reference = 0;
	bool stencil_reference_dirty = true;

	Rml::Matrix4f transform;
	Rml::Matrix4f projection;

	GeometryArena vertex_arena;
	GeometryArena index_arena;

	SDL_GPUCommandBuffer* upload_command_buffer = nullptr;
	SDL_GPUCopyPass* upload_copy_pass = nullptr;
	uint32_t pending_upload_bytes = 0;

	int frame_num_draws = 0;
	int frame_num_passes = 0;
	int frame_num_resolves = 0;

	RenderLayerStack render_layers;

	Rml::CompiledGeometryHandle postprocess_quad = {};
	Rml::CompiledGeometryHandle clear_quad = {};
	int quad_width = 0;
	int quad_height = 0;

	bool shutdown_complete = false;
};
