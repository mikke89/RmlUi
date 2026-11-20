#pragma once

#include <RmlUi/Core/RenderInterface.h>
#include <deque>

#ifdef RMLUI_PLATFORM_WIN32
	#include "RmlUi_Include_Windows.h"
	#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include "RmlUi_Include_Vulkan.h"

// The 23 rendering programs (pipelines) of this backend. Defined in the .cpp, mirrors the DX12 renderer 1:1.
enum class ProgramId;

namespace Gfx {
struct FramebufferData;
}

/*
 * Vulkan port of the DirectX 12 renderer backend (see RmlUi_Renderer_DX12).
 *
 * The architecture mirrors the DX12 renderer one-to-one:
 *  - BufferMemoryManager: growable pool of persistently-mapped, host-visible VMA buffers suballocated through
 *    VmaVirtualBlock (the analog of D3D12MA virtual blocks over UPLOAD heaps). Geometry is memcpy'd straight into
 *    it; constant-buffer payloads are suballocated with dynamic-UBO alignment and bound through per-buffer
 *    descriptor sets using dynamic offsets (the analog of root constant buffer views).
 *  - TextureMemoryManager: VMA-allocated images. Small textures are suballocated by VMA automatically (the analog
 *    of DX12's placed resources), large ones become dedicated allocations (the analog of committed resources).
 *    Uploads go through a staging buffer and are fully synchronous, like the DX12 copy-queue path.
 *  - RenderLayerStack: layer framebuffers (optionally MSAA, sharing one depth-stencil) and postprocess
 *    framebuffers (single-sample), matching the DX12 render target stack.
 *  - 23 pipelines (ProgramId) with identical blend/depth-stencil state as the DX12 PSOs. Stencil reference and
 *    blend constants are dynamic state (the analog of OMSetStencilRef / OMSetBlendFactor).
 *  - DX12 resource state barriers map to Vulkan image layout transitions; ResolveSubresource/CopyResource map to
 *    vkCmdResolveImage/vkCmdCopyImage; OMSetRenderTargets maps to ending/beginning render passes.
 *
 * Vulkan-specific notes (deliberate deviations forced by API differences):
 *  - The projection matrix is pre-multiplied by the GL->Vulkan clip correction (Y-flip, Z remap to [0,1]) instead
 *    of using a negative viewport height, so the backend stays compatible with Vulkan 1.0 (no VK_KHR_maintenance1
 *    requirement). As a result framebuffer space is top-left-origin exactly like DX12, and none of the shaders
 *    need the UV Y-flips the DX12 shaders carry.
 *  - There is no root-parameter-index bookkeeping on geometry: a single dynamic-UBO descriptor binding is visible
 *    to both vertex and fragment stages, so one bind covers what DX12 does with up to three root CBV indices.
 *  - Swapchain images are acquired from the presentation engine, so there is no user-selectable backbuffer index
 *    equivalent; UserSetBackbufferIndex() is intentionally not overridden.
 *
 * Frame-level batching (RMLUI_RENDER_BACKEND_FIELD_BATCHING_ENABLED): eligible RenderGeometry draws (ordinary
 * color/texture geometry; no postprocess, override-constant-buffer, or clip-mask state) are not drawn immediately.
 * Instead GeometryBatcher copies their vertices (with the draw translation baked into the positions, which the
 * shader's pre-transform translation makes exactly equivalent) and indices (with the vertex base baked in) into
 * per-ring-slot persistently-mapped arenas, and appends a 16-byte BatchDraw record keyed by unsigned-char ids into
 * per-run intern tables (texture, scissor, transform). Consecutive records with identical keys merge into one draw.
 * Anything that ends the render pass, changes the render target, or takes the legacy path first flushes the
 * accumulated records (FlushBatches): one draw per record, emitting only the state that changed between records.
 * Arena cursors reset only when the slot's fence passes at BeginFrame — never at flush, since draws recorded earlier
 * in the frame still reference the data; arena growth doubles the buffer and retires the old one through the same
 * frame-counter-stamped deferred destruction as geometry. Set to 0 to compile the batching code out entirely.
 */
class RenderInterface_VK : public Rml::RenderInterface {
private:
	// Amount of rendering programs (ProgramId::Count), kept identical to the DX12 renderer.
	static constexpr size_t NumPrograms = 23;

public:
	static constexpr uint32_t kSwapchainBackBufferCount = 3;

	// Where a suballocation lives: index into BufferMemoryManager's buffer pool, plus byte offset/size inside it.
	struct GraphicsAllocationInfo {
		int buffer_index = -1;
		VkDeviceSize offset = 0;
		VkDeviceSize size = 0;
		VmaVirtualAllocation alloc_info = VK_NULL_HANDLE;
	};

	class TextureHandleType : Rml::NonCopyMoveable {
	public:
		TextureHandleType() = default;
		~TextureHandleType()
		{
#ifdef RMLUI_VK_DEBUG
			RMLUI_ASSERTMSG(m_is_destroyed, "The texture was not destroyed");
#endif
		}

		VkImage Get_Image() const noexcept { return m_p_image; }
		void Set_Image(VkImage p_image) noexcept { m_p_image = p_image; }

		VkImageView Get_ImageView() const noexcept { return m_p_image_view; }
		void Set_ImageView(VkImageView p_image_view) noexcept { m_p_image_view = p_image_view; }

		VkDescriptorSet Get_DescriptorSet() const noexcept { return m_p_descriptor_set; }
		void Set_DescriptorSet(VkDescriptorSet p_set) noexcept { m_p_descriptor_set = p_set; }

		VmaAllocation Get_Allocation() const noexcept { return m_p_allocation; }
		void Set_Allocation(VmaAllocation p_allocation) noexcept { m_p_allocation = p_allocation; }

		// Current image layout, tracked so the renderer can emit correct barriers (DX12 tracks resource states the same way).
		VkImageLayout Get_Layout() const noexcept { return m_layout; }
		void Set_Layout(VkImageLayout layout) noexcept { m_layout = layout; }

		int Get_Width() const noexcept { return m_width; }
		void Set_Width(int width) noexcept { m_width = width; }
		int Get_Height() const noexcept { return m_height; }
		void Set_Height(int height) noexcept { m_height = height; }

		// Marks the resource as released. Actual destruction is done by TextureMemoryManager::Free_Texture; do not call manually.
		void Mark_Destroyed() noexcept
		{
#ifdef RMLUI_VK_DEBUG
			RMLUI_ASSERTMSG(!m_is_destroyed, "The texture has already been destroyed");
			m_is_destroyed = true;
#endif
		}

#ifdef RMLUI_VK_DEBUG
		const Rml::String& Get_ResourceName() const { return m_debug_resource_name; }
		void Set_ResourceName(const Rml::String& resource_name) { m_debug_resource_name = resource_name; }
#endif

	private:
		VkImage m_p_image = nullptr;
		VkImageView m_p_image_view = nullptr;
		// Descriptor set of the 'texture' layout (single combined image sampler), used to bind this texture per draw.
		VkDescriptorSet m_p_descriptor_set = nullptr;
		VmaAllocation m_p_allocation = nullptr;
		VkImageLayout m_layout = VK_IMAGE_LAYOUT_UNDEFINED;
		int m_width = 0;
		int m_height = 0;

#ifdef RMLUI_VK_DEBUG
		Rml::String m_debug_resource_name;
		bool m_is_destroyed = false;
#endif
	};

	struct ConstantBufferType {
		GraphicsAllocationInfo m_alloc_info;
		// Persistently-mapped base pointer of the pool buffer this constant buffer lives in.
		void* m_p_gpu_start_memory_for_binding_data = nullptr;
	};

	class GeometryHandleType : Rml::NonCopyMoveable {
	public:
		GeometryHandleType() = default;

		void Set_InfoVertex(const GraphicsAllocationInfo& info) { m_info_vertex = info; }
		const GraphicsAllocationInfo& Get_InfoVertex() const noexcept { return m_info_vertex; }

		void Set_InfoIndex(const GraphicsAllocationInfo& info) { m_info_index = info; }
		const GraphicsAllocationInfo& Get_InfoIndex() const noexcept { return m_info_index; }

		void Set_NumVertices(int num) { m_num_vertices = num; }
		int Get_NumVertices() const { return m_num_vertices; }

		void Set_NumIndices(int num) { m_num_indices = num; }
		int Get_NumIndices() const { return m_num_indices; }

		void Set_SizeOfOneVertex(size_t size) { m_one_element_vertex_size = size; }
		size_t Get_SizeOfOneVertex() const { return m_one_element_vertex_size; }

		void Set_SizeOfOneIndex(size_t size) { m_one_element_index_size = size; }
		size_t Get_SizeOfOneIndex() const { return m_one_element_index_size; }

		int Get_HistoryBackBufferFrameIndex(void) const { return m_history_backbuffer_frame_index; }
		void Set_HistoryBackBufferFrameIndex(int frame_index) { m_history_backbuffer_frame_index = frame_index; }

		// An override constant buffer, used when this geometry is drawn as a fullscreen postprocess quad.
		void Set_ConstantBuffer(ConstantBufferType* p_constant_buffer)
		{
			RMLUI_ASSERTMSG(p_constant_buffer, "must be valid constant buffer!");
			m_p_constant_buffer_override = p_constant_buffer;
		}

		void Reset_ConstantBuffer() { m_p_constant_buffer_override = nullptr; }

		ConstantBufferType* Get_ConstantBuffer() const { return m_p_constant_buffer_override; }

	private:
		int m_num_vertices = {};
		int m_num_indices = {};
		int m_history_backbuffer_frame_index = -1;
		ConstantBufferType* m_p_constant_buffer_override = {};
		size_t m_one_element_vertex_size = {};
		size_t m_one_element_index_size = {};
		GraphicsAllocationInfo m_info_vertex;
		GraphicsAllocationInfo m_info_index;
	};

	class UploadResourceManager {
		struct upload_buffer_data_t {
			size_t creation_size;
			VkBuffer m_p_vk_buffer;
			VmaAllocation m_p_vma_allocation;
		};

	public:
		UploadResourceManager() : m_p_device{}, m_p_fence{}, m_p_command_buffer{}, m_p_command_pool{}, m_p_graphics_queue{} {}
		~UploadResourceManager() {}

		void Initialize(VkDevice p_device, VkQueue p_queue, VmaAllocator p_allocator, uint32_t queue_family_index, size_t staging_buffer_size);
		void Shutdown(VmaAllocator p_allocator);

		// Records p_user_commands into the one-time command buffer, submits and blocks until the GPU finished.
		template <typename Func>
		void UploadToGPU(Func&& p_user_commands) noexcept
		{
			RMLUI_VK_ASSERTMSG(m_p_command_buffer, "you didn't initialize VkCommandBuffer");

			VkCommandBufferBeginInfo info_command = {};

			info_command.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			info_command.pNext = nullptr;
			info_command.pInheritanceInfo = nullptr;
			info_command.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			VkResult status = vkBeginCommandBuffer(m_p_command_buffer, &info_command);

			RMLUI_VK_ASSERTMSG(status == VkResult::VK_SUCCESS, "failed to vkBeginCommandBuffer");

			p_user_commands(m_p_command_buffer);

			status = vkEndCommandBuffer(m_p_command_buffer);

			RMLUI_VK_ASSERTMSG(status == VkResult::VK_SUCCESS, "faield to vkEndCommandBuffer");

			Submit();
			Wait();
		}

		// Cached staging buffer, reused between uploads unless the request exceeds it.
		const upload_buffer_data_t& Get_UploadBuffer() const noexcept { return m_upload_buffer; }
		upload_buffer_data_t& Get_UploadBuffer() noexcept { return m_upload_buffer; }
		size_t Get_UploadBufferSize() const noexcept { return m_upload_buffer.creation_size; }

		// Allocates a temporary staging buffer (used when the cached one is too small).
		upload_buffer_data_t Create_StagingBuffer(VmaAllocator p_allocator, size_t requested_size);
		void Destroy_StagingBuffer(VmaAllocator p_allocator, upload_buffer_data_t& data);

	private:
		void Create_Fence() noexcept;
		void Create_CommandBuffer() noexcept;
		void Create_StagingBuffer(VmaAllocator p_allocator, size_t requested_size, upload_buffer_data_t* init_buffer);
		void Create_CommandPool(uint32_t queue_family_index) noexcept;
		void Create_All(uint32_t queue_family_index, VmaAllocator p_allocator, size_t staging_buffer_size) noexcept;
		void Wait() noexcept;
		void Submit() noexcept;

	private:
		VkDevice m_p_device;
		VkFence m_p_fence;
		VkCommandBuffer m_p_command_buffer;
		VkCommandPool m_p_command_pool;
		VkQueue m_p_graphics_queue;

		// system talks with these buffers when we want to upload texture
		// we avoid reallocation, but if the requested resource is bigger than upload buffer we temporarely create that staging temp buffer and then
		// delete, but this buffer we don't delete until session of RenderInterface_VK instance is not ended
		upload_buffer_data_t m_upload_buffer;
	};

	// If we need additional command buffers, we can add them to this list and retrieve them from the ring.
	enum class CommandBufferName { Primary, Count };

	// The command buffer ring stores a unique set of named command buffers for each buffered frame.
	// Explanation of how to use Vulkan efficiently: https://vkguide.dev/docs/chapter-4/double_buffering/
	class CommandBufferRing {
	public:
		static constexpr uint32_t kNumFramesToBuffer = kSwapchainBackBufferCount;
		static constexpr uint32_t kNumCommandBuffersPerFrame = static_cast<uint32_t>(CommandBufferName::Count);

		CommandBufferRing();

		void Initialize(VkDevice p_device, uint32_t queue_index_graphics) noexcept;
		void Shutdown();

		void OnBeginFrame();
		VkCommandBuffer GetCommandBufferForActiveFrame(CommandBufferName named_command_buffer);
		uint32_t Get_ActiveFrameIndex() const noexcept { return m_frame_index; }

	private:
		struct CommandBuffersPerFrame {
			Rml::Array<VkCommandPool, kNumCommandBuffersPerFrame> m_command_pools;
			Rml::Array<VkCommandBuffer, kNumCommandBuffersPerFrame> m_command_buffers;
		};

		VkDevice m_p_device;
		uint32_t m_frame_index;
		CommandBuffersPerFrame* m_p_current_frame;
		Rml::Array<CommandBuffersPerFrame, kNumFramesToBuffer> m_frames;
	};

	class DescriptorPoolManager {
	public:
		DescriptorPoolManager() : m_allocated_descriptor_count{}, m_p_descriptor_pool{} {}
		~DescriptorPoolManager()
		{
			RMLUI_VK_ASSERTMSG(m_allocated_descriptor_count <= 0, "something is wrong. You didn't free some VkDescriptorSet");
		}

		void Initialize(VkDevice p_device, uint32_t count_uniform_buffer, uint32_t count_image_sampler, uint32_t count_sampler,
			uint32_t count_storage_buffer) noexcept;

		void Shutdown(VkDevice p_device);

		uint32_t Get_AllocatedDescriptorCount() const noexcept { return m_allocated_descriptor_count; }

		bool Alloc_Descriptor(VkDevice p_device, VkDescriptorSetLayout* p_layouts, VkDescriptorSet* p_sets,
			uint32_t descriptor_count_for_creation = 1) noexcept;

		void Free_Descriptors(VkDevice p_device, VkDescriptorSet* p_sets, uint32_t descriptor_count = 1) noexcept;

	private:
		int m_allocated_descriptor_count;
		VkDescriptorPool m_p_descriptor_pool;
	};

	/**
	 * Growable pool of persistently-mapped host-visible buffers, suballocated with VmaVirtualBlock.
	 *
	 * The Vulkan analog of the DX12 renderer's BufferMemoryManager (D3D12MA virtual blocks over UPLOAD heaps):
	 * vertex/index data is memcpy'd straight into mapped memory, and constant-buffer payloads are suballocated
	 * with dynamic-UBO alignment. Each pool buffer owns one descriptor set of the constant-buffer layout which is
	 * bound per draw with a dynamic offset, like DX12's root CBV binding.
	 */
	class BufferMemoryManager : Rml::NonCopyMoveable {
	public:
		BufferMemoryManager();
		~BufferMemoryManager();

		void Initialize(VkDevice p_device, VmaAllocator p_allocator, DescriptorPoolManager* p_manager_descriptors,
			VkDescriptorSetLayout p_set_layout_constant_buffer, VkDeviceSize constant_buffer_alignment,
			size_t size_for_allocation = RMLUI_RENDER_BACKEND_FIELD_VIDEOMEMORY_FOR_BUFFER_ALLOCATION);
		void Shutdown();

		void Alloc_Vertex(const void* p_data, int num_vertices, size_t size_of_one_element_in_p_data, GeometryHandleType* p_handle);
		void Alloc_Index(const void* p_data, int num_indices, size_t size_of_one_element_in_p_data, GeometryHandleType* p_handle);

		GraphicsAllocationInfo Alloc_ConstantBuffer(ConstantBufferType* p_resource, size_t size);

		void Free_ConstantBuffer(ConstantBufferType* p_constantbuffer);
		void Free_Geometry(GeometryHandleType* p_geometryhandle);

		void* Get_WritableMemoryFromBufferByOffset(const GraphicsAllocationInfo& info);

		VkBuffer Get_BufferByIndex(int buffer_index);
		// Descriptor set of the constant-buffer layout (dynamic UBO) referencing the given pool buffer. The same set is
		// bound as set 0 (color/texture/gradient/creation pipelines) or as set 1 (postprocess pipelines with CB).
		VkDescriptorSet Get_ConstantBufferDescriptorSetByIndex(int buffer_index);

		// Releases pool blocks that were retired some frames ago (deferred, so in-flight command buffers stay valid).
		void Update_PendingForDeletion_Buffers(uint64_t frame_counter);

		bool Is_Initialized() const;

	private:
		void Alloc_Buffer(size_t size);

		// Searches for block that has enough memory for requested allocation size, otherwise returns nullptr that means no block!
		VmaVirtualBlock_T* Get_AvailableBlock(size_t size_for_allocation, int* p_result_buffer_index);
		VmaVirtualBlock_T* Get_NotOutOfMemoryAndAvailableBlock(size_t size_for_allocation, int* p_result_buffer_index);

		int Alloc(GraphicsAllocationInfo& info, size_t size, size_t alignment = 0);

		// Moves completely empty blocks to the deferred destruction list (at most one per call, like the DX12 renderer).
		void TryToFreeAvailableBlock(uint64_t frame_counter);
		void Destroy_BufferAtIndex(int buffer_index);

		VkDevice m_p_device;
		VmaAllocator m_p_allocator;
		DescriptorPoolManager* m_p_manager_descriptors;
		VkDescriptorSetLayout m_p_set_layout_constant_buffer;
		VkDeviceSize m_constant_buffer_alignment;
		size_t m_size_for_allocation_in_bytes;
		uint64_t m_frame_counter;

		/// @brief this is for sub allocating purposes using 'Virtual Allocation' from VMA
		Rml::Vector<VmaVirtualBlock> m_virtual_blocks;
		/// @brief this is physical representation of VRAM and uses from CPU side for binding data
		Rml::Vector<VkBuffer> m_buffers;
		Rml::Vector<VmaAllocation> m_buffer_allocations;
		Rml::Vector<void*> m_buffers_mapped_memory;
		// Per pool buffer: descriptor set of the constant-buffer layout referencing it (index matches m_buffers).
		Rml::Vector<VkDescriptorSet> m_cb_descriptor_sets;
		// Retired pool blocks, stamped with the frame counter at retirement; destroyed once the GPU can't reference them.
		Rml::Vector<Rml::Pair<int, uint64_t>> m_pending_for_deletion_buffers;
	};

	/**
	 * The key feature of this manager is texture management.
	 *
	 * Unlike the DX12 renderer (which manually decides between placed and committed resources), Vulkan defers the
	 * suballocation decision to VMA: images are allocated with vmaCreateImage and VMA internally suballocates small
	 * images or dedicates memory to large ones. Uploads are synchronous through a staging buffer, mirroring the
	 * DX12 copy-queue path. Each texture owns an image view and a descriptor set for per-draw binding.
	 */
	class TextureMemoryManager : Rml::NonCopyMoveable {
	public:
		TextureMemoryManager();
		~TextureMemoryManager();

		void Initialize(RenderInterface_VK* p_renderer, VkDevice p_device, VmaAllocator p_allocator, UploadResourceManager* p_upload_manager,
			DescriptorPoolManager* p_manager_descriptors, VkDescriptorSetLayout p_set_layout_texture, VkSampler p_sampler);
		void Shutdown();

		// Creates a sampled texture, optionally uploading pixel data (nullptr data = uninitialized content).
		void Alloc_Texture(TextureHandleType* p_impl, Rml::Vector2i dimensions, const Rml::byte* p_data
#ifdef RMLUI_VK_DEBUG
			,
			const Rml::String& debug_name
#endif
		);

		// Creates a texture usable as a render target (color or depth-stencil) for framebuffers.
		void Alloc_Texture(TextureHandleType* p_impl, Rml::Vector2i dimensions, VkFormat format, VkSampleCountFlagBits sample_count,
			bool is_depth_stencil
#ifdef RMLUI_VK_DEBUG
			,
			const Rml::String& debug_name
#endif
		);

		void Free_Texture(TextureHandleType* p_texture);

		bool Is_Initialized() const;

	private:
		void Create_ImageView(TextureHandleType* p_impl, VkFormat format, VkImageAspectFlags aspect_mask, VkSampleCountFlagBits sample_count);
		void Create_DescriptorSet_ForTexture(TextureHandleType* p_impl);
		void Upload(TextureHandleType* p_texture, Rml::Vector2i dimensions, const Rml::byte* p_data);

		RenderInterface_VK* m_p_renderer;
		VkDevice m_p_device;
		VmaAllocator m_p_allocator;
		UploadResourceManager* m_p_upload_manager;
		DescriptorPoolManager* m_p_manager_descriptors;
		VkDescriptorSetLayout m_p_set_layout_texture;
		VkSampler m_p_sampler_linear;
	};

	/*
	    Manages render targets, including the layer stack and postprocessing framebuffers.

	    Layers can be pushed and popped, creating new framebuffers as needed. Typically, geometry is rendered to the top
	    layer. The layer framebuffers may have MSAA enabled.

	    Postprocessing framebuffers are separate from the layers, and are commonly used to apply texture-wide effects
	    such as filters. They are used both as input and output during rendering, and do not use MSAA.
	*/
	class RenderLayerStack : Rml::NonCopyMoveable {
	public:
		RenderLayerStack();
		~RenderLayerStack();

		void Initialize(RenderInterface_VK* p_owner);
		void Shutdown();

		// Push a new layer. All references to previously retrieved layers are invalidated.
		Rml::LayerHandle PushLayer();

		// Pop the top layer. All references to previously retrieved layers are invalidated.
		void PopLayer();

		const Gfx::FramebufferData& GetLayer(Rml::LayerHandle layer) const;
		const Gfx::FramebufferData& GetTopLayer() const;
		const Gfx::FramebufferData& Get_SharedDepthStencil_Layers();
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

		void CreateFramebuffer(Gfx::FramebufferData* p_result, int width, int height, int sample_count, bool is_depth_stencil);
		void DestroyFramebuffer(Gfx::FramebufferData* p_data);

		unsigned char m_msaa_sample_count;
		int m_layers_size;
		int m_width;
		int m_height;
		RenderInterface_VK* m_p_owner;
		Gfx::FramebufferData* m_p_shared_depth_stencil_for_layers;
		Rml::Vector<Gfx::FramebufferData> m_fb_layers;
		Rml::Vector<Gfx::FramebufferData> m_fb_postprocess;
	};

#if RMLUI_RENDER_BACKEND_FIELD_BATCHING_ENABLED == 1
	/**
	 * Frame-level geometry batcher (see the class-level comment of RenderInterface_VK).
	 *
	 * Owns only memory and batch records; the renderer keeps doing all GPU binding itself. The per-ring-slot arenas
	 * are persistently-mapped host-visible VMA buffers created with the same allocation flags as the buffer pool
	 * (vertex data in one buffer, index data in another). Growth doubles the buffer, copies the used range at
	 * identical offsets (batch records reference arena offsets, not buffer objects, and the buffers are only bound at
	 * flush time), and retires the old buffer into a frame-counter-stamped deferred-destruction list, the same idiom
	 * as the renderer's pending-for-deletion geometry.
	 */
	class GeometryBatcher : Rml::NonCopyMoveable {
	public:
		// One batched draw run over the frame index arena. 16 bytes, unsigned-char keyed — the minimal memory layout
		// is a hard requirement. ProgramId is incomplete in this header, so the program is stored as uint8_t and cast
		// at the use sites in the .cpp.
		struct BatchDraw {
			uint32_t first_index; // first index (in indices, not bytes) in the frame index arena
			uint32_t index_count; // number of indices in this run
			uint8_t texture_id;   // interned texture table index; kBatchNoTexture (0xFF) = color pipeline
			uint8_t scissor_id;   // 0 = full-frame scissor; 1..255 index into the scissor table
			uint8_t program_id;   // ProgramId (values < 32)
			uint8_t stencil_ref;  // stencil reference in effect (saturated at 255)
			uint8_t transform_id; // interned transform-matrix table index
			uint8_t reserved[3];  // zero
		};
		static_assert(sizeof(BatchDraw) == 16, "the batch record must stay 16 bytes");

		// texture_id sentinel: the draw runs a color pipeline and binds no texture
		static constexpr uint8_t kBatchNoTexture = 0xFF;
		// 0xFF is the none-sentinel, so at most 254 real texture entries fit
		static constexpr uint32_t kMaxInternedTextures = 254;
		// scissor id 0 is reserved for the full-frame scissor; ids 1..255 map to table entries 0..254
		static constexpr uint32_t kMaxInternedScissors = 255;
		static constexpr uint32_t kMaxInternedTransforms = 255;

		enum class AccumulateResult {
			Done,        // the draw was merged into (or appended to) the accumulation run
			NeedFlush,   // an intern table ran full; the renderer must flush the run and retry the accumulate
			NotBatchable // the draw cannot be accumulated after all (a flush broke the texture-reuse chain)
		};

		GeometryBatcher();
		~GeometryBatcher();

		void Initialize(VkDevice p_device, VmaAllocator p_allocator, BufferMemoryManager* p_manager_buffer) noexcept;
		// Also drains the retired-buffer list; the device must be idle (Destroy_Resources runs after a GPU drain).
		void Shutdown();

		// Restarts the slot's arena cursors (its fence just passed), resets the accumulation state, and destroys
		// retired arena buffers that aged a full swapchain cycle out.
		void BeginFrame(uint32_t slot_index, uint64_t frame_counter) noexcept;

		// Batchable: ordinary color/texture draws only — no postprocess passthrough, no override constant buffer, no
		// clip-mask operation. TextureEnableWithoutBinding additionally requires a textured previous record to reuse.
		bool CanAccumulate(Rml::TextureHandle texture, const GeometryHandleType* p_handle_geometry, int current_clip_operation) const noexcept;

		// Appends the draw to the current run: copies the vertices (baking the translation into their positions) and
		// the indices (baking the vertex base) into the slot's arenas, then extends or pushes a BatchDraw record.
		AccumulateResult Accumulate(Rml::TextureHandle texture, Rml::Vector2f translation, const GeometryHandleType* p_handle_geometry,
			ProgramId program_id, bool is_scissor_set, const Rml::Rectanglei& scissor, const Rml::Matrix4f& transform, uint32_t stencil_ref);

		// Clears the records and intern tables after a flush; the arena cursors are NOT reset here (draws recorded
		// earlier in the frame still reference the arena data).
		void ResetAccumulation() noexcept;

		const Rml::Vector<BatchDraw>& Get_Records() const noexcept { return m_records; }
		// geometries merged into the current accumulation run so far (for the per-flush label)
		uint32_t Get_AccumulatedGeometryCount() const noexcept { return m_geometry_count; }
		Rml::TextureHandle Get_TextureById(uint8_t texture_id) const noexcept;
		const Rml::Rectanglei& Get_ScissorById(uint8_t scissor_id) const noexcept;
		const Rml::Matrix4f& Get_TransformById(uint8_t transform_id) const noexcept { return m_transforms[transform_id]; }
		uint32_t Get_TransformCount() const noexcept { return static_cast<uint32_t>(m_transforms.size()); }
		VkBuffer Get_VertexArenaBuffer() const noexcept { return m_arenas[m_current_slot].p_buffer_vertex; }
		VkBuffer Get_IndexArenaBuffer() const noexcept { return m_arenas[m_current_slot].p_buffer_index; }

	private:
		// Per-ring-slot arena pair. The used cursors restart only in BeginFrame, once the slot's fence passed.
		struct ArenaSlot {
			VkBuffer p_buffer_vertex = nullptr;
			VmaAllocation p_allocation_vertex = nullptr;
			std::uint8_t* p_mapped_vertex = nullptr;
			size_t capacity_vertex = 0; // bytes
			size_t used_vertex = 0;     // bytes

			VkBuffer p_buffer_index = nullptr;
			VmaAllocation p_allocation_index = nullptr;
			std::uint8_t* p_mapped_index = nullptr;
			size_t capacity_index = 0; // bytes
			uint32_t used_index = 0;   // in indices (uint32_t units); the first free index cursor
		};

		// An arena buffer replaced by growth; destroyed once a full swapchain cycle passed.
		struct RetiredArenaBuffer {
			VkBuffer p_buffer;
			VmaAllocation p_allocation;
			uint64_t frame_stamp;
		};

		void Create_ArenaBuffer(ArenaSlot& slot, bool is_vertex, size_t size);
		void Grow_Arena(ArenaSlot& slot, bool is_vertex, size_t required_size);
		void Destroy_ArenaBuffer(VkBuffer& p_buffer, VmaAllocation& p_allocation) noexcept;

		VkDevice m_p_device;
		VmaAllocator m_p_allocator;
		// the source geometry bytes are read out of the buffer pool's persistently-mapped memory
		BufferMemoryManager* m_p_manager_buffer;
		uint32_t m_current_slot;
		uint64_t m_frame_counter;

		Rml::Vector<BatchDraw> m_records;
		uint32_t m_geometry_count = 0;
		Rml::TextureHandle m_textures[kMaxInternedTextures];
		uint8_t m_texture_count;
		Rml::Rectanglei m_scissors[kMaxInternedScissors];
		uint8_t m_scissor_count;
		Rml::Vector<Rml::Matrix4f> m_transforms;

		Rml::Array<ArenaSlot, CommandBufferRing::kNumFramesToBuffer> m_arenas;
		Rml::Vector<RetiredArenaBuffer> m_retired_buffers;
	};
#endif

	struct PhysicalDeviceWrapper {
		VkPhysicalDevice m_p_physical_device;
		VkPhysicalDeviceProperties m_physical_device_properties;
	};

	using PhysicalDeviceWrapperList = Rml::Vector<PhysicalDeviceWrapper>;
	using LayerPropertiesList = Rml::Vector<VkLayerProperties>;
	using ExtensionPropertiesList = Rml::Vector<VkExtensionProperties>;

	RenderInterface_VK();
	~RenderInterface_VK();

	using CreateSurfaceCallback = bool (*)(VkInstance instance, VkSurfaceKHR* out_surface);

	bool Initialize(Rml::Vector<const char*> required_extensions, CreateSurfaceCallback create_surface_callback);
	void Shutdown();

	void BeginFrame();
	void EndFrame();

	// The viewport should be updated whenever the window size changes.
	void SetViewport(int width, int height);
	// Optional, can be used to clear the active framebuffer.
	void Clear();
	bool IsSwapchainValid();
	void RecreateSwapchain();

	// Captures the presented backbuffer contents into CPU memory (3 components, bottom-up rows, like the DX12 renderer).
	bool CaptureScreen(int& width, int& height, int& num_components, Rml::UniquePtr<Rml::byte[]>& data);

	// -- Inherited from Rml::RenderInterface --

	/// Called by RmlUi when it wants to compile geometry it believes will be static for the forseeable future.
	Rml::CompiledGeometryHandle CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices) override;
	/// Called by RmlUi when it wants to render application-compiled geometry.
	void RenderGeometry(Rml::CompiledGeometryHandle handle, Rml::Vector2f translation, Rml::TextureHandle texture) override;
	/// Called by RmlUi when it wants to release application-compiled geometry.
	void ReleaseGeometry(Rml::CompiledGeometryHandle geometry) override;

	/// Called by RmlUi when a texture is required by the library.
	Rml::TextureHandle LoadTexture(Rml::Vector2i& texture_dimensions, const Rml::String& source) override;
	/// Called by RmlUi when a texture is required to be built from an internally-generated sequence of pixels.
	Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte> source_data, Rml::Vector2i source_dimensions) override;
	/// Called by RmlUi when a loaded texture is no longer required.
	void ReleaseTexture(Rml::TextureHandle texture_handle) override;

	/// Called by RmlUi when it wants to enable or disable scissoring to clip content.
	void EnableScissorRegion(bool enable) override;
	/// Called by RmlUi when it wants to change the scissor region.
	void SetScissorRegion(Rml::Rectanglei region) override;

	/// Called by RmlUi when it wants to set the current transform matrix to a new matrix.
	void SetTransform(const Rml::Matrix4f* transform) override;

	void EnableClipMask(bool enable) override;
	void RenderToClipMask(Rml::ClipMaskOperation mask_op, Rml::CompiledGeometryHandle geom, Rml::Vector2f translation) override;

	Rml::LayerHandle PushLayer() override;
	void CompositeLayers(Rml::LayerHandle src, Rml::LayerHandle dest, Rml::BlendMode blend,
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

private:
	Rml::TextureHandle CreateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i dimensions, const Rml::String& name);

	void Initialize_Instance(Rml::Vector<const char*> required_extensions) noexcept;
	void Initialize_Device() noexcept;
	void Initialize_PhysicalDevice(VkPhysicalDeviceProperties& out_physical_device_properties) noexcept;
	void Initialize_Swapchain(VkExtent2D window_extent) noexcept;
	void Initialize_Surface(CreateSurfaceCallback create_surface_callback) noexcept;
	void Initialize_QueueIndecies() noexcept;
	void Initialize_Queues() noexcept;
	void Initialize_SyncPrimitives() noexcept;
	void Initialize_Resources(const VkPhysicalDeviceProperties& physical_device_properties) noexcept;
	void Initialize_Allocator() noexcept;

	void Destroy_Instance() noexcept;
	void Destroy_Device() noexcept;
	void Destroy_Swapchain() noexcept;
	void Destroy_Surface() noexcept;
	void Destroy_SyncPrimitives() noexcept;
	void Destroy_Resources() noexcept;
	void Destroy_Allocator() noexcept;

	void QueryInstanceLayers(LayerPropertiesList& result) noexcept;
	void QueryInstanceExtensions(ExtensionPropertiesList& result, const LayerPropertiesList& instance_layer_properties) noexcept;
	bool AddLayerToInstance(Rml::Vector<const char*>& result, const LayerPropertiesList& instance_layer_properties,
		const char* p_instance_layer_name) noexcept;
	bool AddExtensionToInstance(Rml::Vector<const char*>& result, const ExtensionPropertiesList& instance_extension_properties,
		const char* p_instance_extension_name) noexcept;
	void CreatePropertiesFor_Instance(Rml::Vector<const char*>& instance_layer_names, Rml::Vector<const char*>& instance_extension_names) noexcept;

	bool IsLayerPresent(const LayerPropertiesList& properties, const char* p_layer_name) noexcept;
	bool IsExtensionPresent(const ExtensionPropertiesList& properties, const char* p_extension_name) noexcept;

	bool AddExtensionToDevice(Rml::Vector<const char*>& result, const ExtensionPropertiesList& device_extension_properties,
		const char* p_device_extension_name) noexcept;
	void CreatePropertiesFor_Device(ExtensionPropertiesList& result) noexcept;

	void CreateReportDebugCallback() noexcept;
	void Destroy_ReportDebugCallback() noexcept;

	uint32_t GetUserAPIVersion() const noexcept;
	uint32_t GetRequiredVersionAndValidateMachine() noexcept;

	void CollectPhysicalDevices(PhysicalDeviceWrapperList& out_physical_devices) noexcept;
	const PhysicalDeviceWrapper* ChoosePhysicalDevice(const PhysicalDeviceWrapperList& physical_devices, VkPhysicalDeviceType device_type) noexcept;

	VkSurfaceFormatKHR ChooseSwapchainFormat() noexcept;
	VkSurfaceTransformFlagBitsKHR CreatePretransformSwapchain() noexcept;
	VkCompositeAlphaFlagBitsKHR ChooseSwapchainCompositeAlpha() noexcept;
	int Choose_SwapchainImageCount(uint32_t user_swapchain_count_for_creation = kSwapchainBackBufferCount, bool if_failed_choose_min = true) noexcept;
	VkPresentModeKHR GetPresentMode(VkPresentModeKHR type = VkPresentModeKHR::VK_PRESENT_MODE_FIFO_KHR) noexcept;
	VkSurfaceCapabilitiesKHR GetSurfaceCapabilities() noexcept;

	VkExtent2D GetValidSurfaceExtent() noexcept;

	void Create_Shaders() noexcept;
	void Create_DescriptorSetLayouts() noexcept;
	void Create_PipelineLayouts() noexcept;
	void Create_Samplers() noexcept;
	void Create_Pipelines() noexcept;

	void Create_Pipeline_Color();
	void Create_Pipeline_Texture();
	void Create_Pipeline_Gradient();
	void Create_Pipeline_Creation();
	void Create_Pipeline_Passthrough();
	void Create_Pipeline_Passthrough_NoBlend();
	void Create_Pipeline_ColorMatrix();
	void Create_Pipeline_BlendMask();
	void Create_Pipeline_Blur();
	void Create_Pipeline_DropShadow();

	// Render passes: one per framebuffer configuration (mirrors the DX12 render-target/PSO sample-count matrix).
	void Create_RenderPasses() noexcept;

	void Create_SwapchainFrameBuffers(const VkExtent2D& real_render_image_size) noexcept;

	// This method is called in Views, so don't call it manually
	void Create_SwapchainImages() noexcept;
	void Create_SwapchainImageViews() noexcept;

	void Create_DepthStencilImage() noexcept;
	void Create_DepthStencilImageViews() noexcept;

	void Create_ResourcesDependentOnSize(const VkExtent2D& real_render_image_size) noexcept;

	void Destroy_Textures() noexcept;
	void Destroy_Geometries() noexcept;
	void Destroy_Texture(TextureHandleType* p_texture) noexcept;

	void DestroyResourcesDependentOnSize() noexcept;
	void DestroySwapchainImageViews() noexcept;
	void DestroySwapchainFrameBuffers() noexcept;
	void DestroyRenderPasses() noexcept;
	void Destroy_Pipelines() noexcept;
	void DestroyDescriptorSetLayouts() noexcept;
	void DestroyPipelineLayouts() noexcept;
	void DestroySamplers() noexcept;
	void Destroy_Shaders() noexcept;

	void Wait() noexcept;

	void Update_PendingForDeletion_Textures() noexcept;
	void Update_PendingForDeletion_Geometries() noexcept;

	void Submit() noexcept;
	void Present() noexcept;

	// Program (pipeline) selection, the analog of DX12's UseProgram (PSO + root signature bind).
	void UseProgram(ProgramId id);

	// Scissor handling. Unlike DX12 there is no vertically_flip variant: Vulkan framebuffer space in this backend is
	// already top-left-origin (see the projection correction matrix), so scissor rectangles are used as-is.
	void SetScissor(Rml::Rectanglei region);

	void SubmitTransformUniform(ConstantBufferType& constant_buffer, const Rml::Vector2f& translation);

#if RMLUI_RENDER_BACKEND_FIELD_BATCHING_ENABLED == 1
	// -- Batching (RMLUI_RENDER_BACKEND_FIELD_BATCHING_ENABLED) --

	// Draws the accumulated batch records in order, emitting only the state that changed between records (one
	// vkCmdDrawIndexed per record), then restores the dynamic state (scissor, stencil reference) the legacy path
	// expects to find. No-op when nothing was accumulated. Requires an active render pass.
	void FlushBatches();
	// Program selection for the batch path; duplicates the legacy RenderGeometry if-chain (left untouched) for the
	// batchable cases (m_current_clip_operation == -1 is guaranteed by GeometryBatcher::CanAccumulate).
	ProgramId SelectBatchProgramId(bool is_textured) const noexcept;
	// Explicit-transform overload used by the batch path: the transform comes from the intern table, not the member.
	void SubmitTransformUniform(ConstantBufferType& constant_buffer, const Rml::Matrix4f& transform, const Rml::Vector2f& translation);
	// Emits the scissor matching the current renderer state (m_is_scissor_was_set/m_scissor, already clamped by
	// SetScissor). While batching is enabled SetScissor records nothing, so every legacy-path draw calls this first.
	void EmitCurrentScissorState();
#endif

	ConstantBufferType* Get_ConstantBuffer(uint32_t current_back_buffer_index);

	void Free_Geometry(GeometryHandleType* p_handle);
	void Free_Texture(TextureHandleType* p_handle);

	// -- Render pass / barrier helpers (the Vulkan analog of DX12's explicit resource state barriers) --

	// Ends the currently recorded render pass (if any). Must be called before barriers/copies/resolves.
	void EndActiveRenderPass() noexcept;
	// Begins (or keeps) the render pass targeting the given framebuffer, binding it as the active render target.
	void BindRenderTarget(const Gfx::FramebufferData& framebuffer, bool depth_included = true);
	// Begins the layer render pass in its CLEAR variant (color attachment cleared to transparent black on begin, the
	// analog of the DX12 renderer clearing the pushed layer's RTV) and marks it as the active render target. When
	// clear_depth_stencil is set, the depth-stencil attachment is cleared instead (color is preserved via LOAD) —
	// used by RenderToClipMask when the stencil clear happens to be the first command of a fresh pass.
	void BindRenderTarget_Clear(const Gfx::FramebufferData& framebuffer, bool clear_depth_stencil = false);
	// Image memory barrier on the currently recording command buffer; updates the tracked layout.
	void TransitionImageLayout(TextureHandleType* p_texture, VkImageLayout new_layout,
		VkImageAspectFlags aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT) noexcept;

	void BlitLayerToPostprocessPrimary(Rml::LayerHandle layer_id);

	void RenderFilters(Rml::Span<const Rml::CompiledFilterHandle> filter_handles);
	void RenderBlur(float sigma, const Gfx::FramebufferData& source_destination, const Gfx::FramebufferData& temp, const Rml::Rectanglei window);

	void DrawFullscreenQuad(ConstantBufferType* p_override_constant_buffer = nullptr);
	void DrawFullscreenQuad(Rml::Vector2f uv_offset, Rml::Vector2f uv_scaling = Rml::Vector2f(1.f),
		ConstantBufferType* p_override_constant_buffer = nullptr);

	void BindTexture(TextureHandleType* p_texture, uint32_t set_index = 0);

	void OverrideConstantBufferOfGeometry(Rml::CompiledGeometryHandle geometry, ConstantBufferType* p_override_constant_buffer);

	// 1 means not supported, otherwise returns max value of supported multisample count
	unsigned char GetMSAASupportedSampleCount(unsigned char max_samples);

	void BlitFramebuffer(const Gfx::FramebufferData& source, const Gfx::FramebufferData& dest, int srcX0, int srcY0, int srcX1, int srcY1, int dstX0,
		int dstY0, int dstX1, int dstY1);

	VkFormat Get_SupportedDepthFormat();

	// Full GPU drain, the analog of the DX12 renderer's Flush().
	void Flush() noexcept;

private:
	bool m_is_transform_enabled;
	bool m_is_scissor_was_set;
	bool m_is_stencil_enabled;
	bool m_is_stencil_equal;
	bool m_is_use_msaa;

	unsigned char m_msaa_sample_count;

	int m_width;
	int m_height;

	// Current clip-mask operation being recorded (Rml::ClipMaskOperation cast, -1 when none), like the DX12 renderer.
	int m_current_clip_operation;
	ProgramId m_active_program_id;
	Rml::Rectanglei m_scissor;
	uint32_t m_stencil_ref_value;

	uint32_t m_queue_index_present;
	uint32_t m_queue_index_graphics;
	uint32_t m_queue_index_compute;
	uint32_t m_semaphore_index;
	uint32_t m_semaphore_index_previous;
	uint32_t m_image_index;

	// Monotonically increasing count of submitted frames; deferred-deletion entries are stamped with it.
	uint64_t m_frame_counter;

	VkInstance m_p_instance;
	VkDevice m_p_device;
	VkPhysicalDevice m_p_physical_device;
	VkSurfaceKHR m_p_surface;
	VkSwapchainKHR m_p_swapchain;
	VmaAllocator m_p_allocator;
	// @ obtained from the command buffer ring, see BeginFrame method
	VkCommandBuffer m_p_current_command_buffer;

	// Descriptor set layouts: 'transform' (dynamic UBO, vertex+fragment; bound as set 0 for color/texture/gradient/
	// creation pipelines and as set 1 for postprocess pipelines with constant buffers), 'texture' (single combined
	// image sampler), 'blend_mask' (two combined image samplers).
	VkDescriptorSetLayout m_p_descriptor_set_layout_transform;
	VkDescriptorSetLayout m_p_descriptor_set_layout_texture;
	VkDescriptorSetLayout m_p_descriptor_set_layout_blend_mask;

	VkPipelineLayout m_p_pipeline_layout_transform;
	VkPipelineLayout m_p_pipeline_layout_transform_texture;
	VkPipelineLayout m_p_pipeline_layout_texture;
	VkPipelineLayout m_p_pipeline_layout_texture_effect;
	VkPipelineLayout m_p_pipeline_layout_blend_mask;

	// The global descriptor set bound as set 0 for the transform UBO is owned by the buffer manager (per pool buffer).
	VkRenderPass m_p_render_pass_layer;           // MSAA color + MSAA depth-stencil (UI layers)
	VkRenderPass m_p_render_pass_layer_clear;     // same, but the color load-op is CLEAR (layer pushes / frame begin)
	VkRenderPass m_p_render_pass_layer_clear_all; // same, but both color and depth-stencil load-ops are CLEAR (Clear())
	// same, but only the depth-stencil load-op is CLEAR (clip-mask stencil clear at the start of a fresh pass)
	VkRenderPass m_p_render_pass_layer_clear_ds;
	VkRenderPass m_p_render_pass_postprocess; // single-sample color only (filters, blur, ...)
	VkRenderPass m_p_render_pass_swapchain;   // swapchain color + main depth-stencil (final composite)
	VkSampler m_p_sampler_linear;
	VkRect2D m_scissor_original;
	VkViewport m_viewport;

	// Render-pass recording state: which framebuffer (and its render pass) is currently being recorded into.
	VkRenderPass m_p_active_render_pass;
	VkFramebuffer m_p_active_framebuffer;

	VkQueue m_p_queue_present;
	VkQueue m_p_queue_graphics;
	VkQueue m_p_queue_compute;

#ifdef RMLUI_VK_DEBUG
	VkDebugUtilsMessengerEXT m_debug_messenger;
	// VK_EXT_debug_utils entry points for RenderDoc command labels (loaded at device creation, null when unsupported)
	PFN_vkCmdBeginDebugUtilsLabelEXT m_pfn_cmd_begin_debug_utils_label = nullptr;
	PFN_vkCmdEndDebugUtilsLabelEXT m_pfn_cmd_end_debug_utils_label = nullptr;
#endif

	VkSurfaceFormatKHR m_swapchain_format;
	TextureHandleType m_texture_depthstencil;

	VkPipeline m_pipelines[NumPrograms];

	Rml::Matrix4f m_projection;
	Rml::Matrix4f m_constant_buffer_data_transform;
	Rml::Vector<VkFence> m_executed_fences;
	// Tracks which fence slots were actually submitted to the GPU (fences are created unsignaled); guards the
	// wait/reset in Wait() so a fence is never waited or submitted while in an invalid state.
	Rml::Array<bool, kSwapchainBackBufferCount> m_submitted_fences;
	Rml::Vector<VkSemaphore> m_semaphores_image_available;
	// one per SWAPCHAIN IMAGE (not per frame slot): the presentation engine holds the semaphore a frame's present
	// waits on until that image is re-acquired, so indexing by the acquired image index is the only safe reuse scheme
	// (see VUID-vkQueueSubmit-pSignalSemaphores-00067 / the swapchain semaphore reuse guide)
	Rml::Vector<VkSemaphore> m_semaphores_finished_render;
	Rml::Vector<VkFramebuffer> m_swapchain_frame_buffers;
	Rml::Vector<VkImage> m_swapchain_images;
	Rml::Vector<VkImageView> m_swapchain_image_views;
	VkShaderModule m_shaders[static_cast<int>(eVKShaderID::count)];
	Rml::CompiledGeometryHandle m_precompiled_fullscreen_quad_geometry;

	// Per backbuffer-slot constant buffer ring (per draw), like the DX12 renderer's m_constantbuffers.
	// std::deque is used on purpose: emplace_back must never invalidate ConstantBufferType* handed out earlier in the
	// frame (geometry override constant buffers point into this storage), which std::vector reallocation would break.
	Rml::Array<std::deque<ConstantBufferType>, kSwapchainBackBufferCount> m_constantbuffers;
	Rml::Array<size_t, kSwapchainBackBufferCount> m_constant_buffer_count_per_frame;

	// Resources deferred for destruction. Each entry is stamped with the frame counter at release time; the memory is
	// recycled once a full swapchain cycle passed, i.e. when the GPU has finished every frame that could reference it.
	Rml::Vector<Rml::Pair<GeometryHandleType*, uint64_t>> m_pending_for_deletion_geometries;
	Rml::Vector<Rml::Pair<TextureHandleType*, uint64_t>> m_pending_for_deletion_textures;

	CommandBufferRing m_command_buffer_ring;
	UploadResourceManager m_upload_manager;
	DescriptorPoolManager m_manager_descriptors;
	BufferMemoryManager m_manager_buffer;
	TextureMemoryManager m_manager_texture;
	RenderLayerStack m_manager_render_layer;

#if RMLUI_RENDER_BACKEND_FIELD_BATCHING_ENABLED == 1
	// the last member on purpose: it destructs first, before the buffer manager whose mapped memory it reads
	GeometryBatcher m_manager_batch;
	// per-frame batching stats: reset in BeginFrame, logged in EndFrame under RMLUI_VK_DEBUG
	uint32_t m_batch_stats_geometry_draws = 0;
	uint32_t m_batch_stats_draw_calls = 0;
#endif
};
