/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019-2025 The RmlUi Team, and contributors
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

#ifndef RMLUI_BACKENDS_RENDERER_DX12_H
#define RMLUI_BACKENDS_RENDERER_DX12_H

#include <RmlUi/Core/RenderInterface.h>

/**
 * Include third-party dependencies.
 */

#ifndef RMLUI_PLATFORM_WIN32
	#error unable to compile platform specific renderer required Windows OS that support DirectX-12
#endif

// clang-format off
#include "RmlUi_Include_Windows.h"
#include "RmlUi_Include_DirectX_12.h"
// clang-format on

enum class ProgramId;

constexpr uint32_t _kRenderBackend_InvalidConstantBuffer_RootParameterIndex = std::numeric_limits<uint32_t>::max();
constexpr uint8_t _kRenderBackend_MaxConstantBuffersPerShader = 3;

class RenderLayerStack;
namespace Gfx {
struct ProgramData;
struct FramebufferData;

#if RMLUI_RENDER_BACKEND_FIELD_IGNORE_RENDERER_INVALID_STATES == 1
enum RenderState;
#endif
} // namespace Gfx

namespace Backend {
struct RmlRenderInitInfo;
struct RmlRendererSettings;
} // namespace Backend

/**
 * DirectX 12 render interface implementation for RmlUi
 * @author wh1t3lord (https://github.com/wh1t3lord)
 */

class RenderInterface_DX12 : public Rml::RenderInterface {
public:
#ifdef RMLUI_DX_DEBUG
	// only for your mouse course to see how it is in MBs, just move cursor to variable on left side from = and you see the value of MBs

	static constexpr size_t kDebugMB_BA = (RMLUI_RENDER_BACKEND_FIELD_VIDEOMEMORY_FOR_BUFFER_ALLOCATION / 1024) / 1024;
	static constexpr size_t kDebugMB_TA = (RMLUI_RENDER_BACKEND_FIELD_VIDEOMEMORY_FOR_TEXTURE_ALLOCATION / 1024) / 1024;
#endif

public:
	class GraphicsAllocationInfo {
	public:
		GraphicsAllocationInfo();
		~GraphicsAllocationInfo();

		const D3D12MA::VirtualAllocation& Get_VirtualAllocation(void) const noexcept;
		void Set_VirtualAllocation(const D3D12MA::VirtualAllocation& info_alloc) noexcept;

		size_t Get_Offset(void) const noexcept;
		void Set_Offset(size_t value) noexcept;

		size_t Get_Size(void) const noexcept;
		void Set_Size(size_t value) noexcept;

		int Get_BufferIndex(void) const noexcept;
		void Set_BufferIndex(int index) noexcept;

	private:
		int m_buffer_index;
		size_t m_offset;
		size_t m_size;
		D3D12MA::VirtualAllocation m_alloc_info;
	};

	class TextureHandleType {
	public:
		TextureHandleType() :
#ifdef RMLUI_DX_DEBUG
			m_is_destroyed{},
#endif
			m_p_resource{}
		{}
		~TextureHandleType()
		{
#ifdef RMLUI_DX_DEBUG
			RMLUI_ASSERT(this->m_is_destroyed && "you forgot to destroy the resource!");
#endif
		}

		const GraphicsAllocationInfo& Get_Info() const noexcept { return this->m_info; }

		void Set_Info(const GraphicsAllocationInfo& info) { this->m_info = info; }

		void Set_Resource(void* p_resource) { this->m_p_resource = p_resource; }

		void* Get_Resource() const noexcept { return this->m_p_resource; }

		/// @brief returns debug resource name. Implementation of this method exists only when DEBUG is enabled for this project
		/// @param  void, nothing
		/// @return debug resource name when the source string is passed in LoadTexture method
		const Rml::String& Get_ResourceName(void) const
		{
#ifdef RMLUI_DX_DEBUG
			return this->m_debug_resource_name;
#else
			return Rml::String();
#endif
		}

		/// @brief sets m_debug_resource_name field with specified argument as resource_name
		/// @param resource_name this string is getting from LoadTexture method from source argument
		void Set_ResourceName(const Rml::String& resource_name)
		{
#ifdef RMLUI_DX_DEBUG
			this->m_debug_resource_name = resource_name;
#else
			resource_name;
#endif
		}

		// this method calls texture manager! don't call it manually because you must ensure that you freed virtual block if it had
		void Destroy()
		{
#ifdef RMLUI_DX_DEBUG
			if (!this->m_is_destroyed)
			{
#endif
				if (this->m_p_resource)
				{
					if (this->m_info.Get_BufferIndex() != -1)
					{
						ID3D12Resource* p_placed_resource = static_cast<ID3D12Resource*>(this->m_p_resource);

						p_placed_resource->Release();
					}
					else
					{
						D3D12MA::Allocation* p_committed_resource = static_cast<D3D12MA::Allocation*>(this->m_p_resource);

						if (p_committed_resource->GetResource())
						{
							p_committed_resource->GetResource()->Release();
						}

						p_committed_resource->Release();
					}

					this->m_info = GraphicsAllocationInfo();
					this->m_p_resource = nullptr;
#ifdef RMLUI_DX_DEBUG
					this->m_is_destroyed = true;
#endif
				}
#ifdef RMLUI_DX_DEBUG
			}
#endif
		}

		const OffsetAllocator::Allocation& Get_Allocation_DescriptorHeap(void) const noexcept { return this->m_allocation_descriptor_heap; }
		void Set_Allocation_DescriptorHeap(const OffsetAllocator::Allocation& allocation) noexcept
		{
			this->m_allocation_descriptor_heap = allocation;
		}

	private:
#ifdef RMLUI_DX_DEBUG
		bool m_is_destroyed;
#endif

		GraphicsAllocationInfo m_info;
		OffsetAllocator::Allocation m_allocation_descriptor_heap;
		void* m_p_resource; // placed or committed (CreateResource from D3D12MA)
#ifdef RMLUI_DX_DEBUG
		Rml::String m_debug_resource_name;
#endif
	};

	class ConstantBufferType {
	public:
		ConstantBufferType();
		~ConstantBufferType();

		const GraphicsAllocationInfo& Get_AllocInfo(void) const noexcept;
		void Set_AllocInfo(const GraphicsAllocationInfo& info) noexcept;

		void* Get_GPU_StartMemoryForBindingData(void);

		void Set_GPU_StartMemoryForBindingData(void* p_start_pointer);

		bool Is_Free(void) const { return this->m_is_free; }

	private:
		bool m_is_free;
		GraphicsAllocationInfo m_alloc_info;
		void* m_p_gpu_start_memory_for_binding_data;
	};

	class GeometryHandleType {
	public:
		GeometryHandleType(void) :
			m_num_vertices{}, m_num_indecies{}, m_p_constant_buffer_override{}, m_one_element_vertex_size{}, m_one_element_index_size{},
			m_constant_buffer_root_parameter_indicies{_kRenderBackend_InvalidConstantBuffer_RootParameterIndex,
				_kRenderBackend_InvalidConstantBuffer_RootParameterIndex, _kRenderBackend_InvalidConstantBuffer_RootParameterIndex}
		{}

		~GeometryHandleType(void) {}

		void Set_InfoVertex(const GraphicsAllocationInfo& info) { this->m_info_vertex = info; }
		const GraphicsAllocationInfo& Get_InfoVertex(void) const noexcept { return this->m_info_vertex; }

		void Set_InfoIndex(const GraphicsAllocationInfo& info) { this->m_info_index = info; }
		const GraphicsAllocationInfo& Get_InfoIndex(void) const noexcept { return this->m_info_index; }

		void Set_NumVertices(int num) { this->m_num_vertices = num; }

		int Get_NumVertices(void) const { return this->m_num_vertices; }

		void Set_NumIndecies(int num) { this->m_num_indecies = num; }

		int Get_NumIndecies(void) const { return this->m_num_indecies; }

		void Set_SizeOfOneVertex(size_t size) { this->m_one_element_vertex_size = size; }

		size_t Get_SizeOfOneVertex(void) const { return this->m_one_element_vertex_size; }

		void Set_SizeOfOneIndex(size_t size) { this->m_one_element_index_size = size; }

		size_t Get_SizeOfOneIndex(void) const { return this->m_one_element_index_size; }

		const OffsetAllocator::Allocation& Get_Allocation_DescriptorHeap(void) const noexcept { return this->m_allocation_descriptor_heap; }
		void Set_Allocation_DescriptorHeap(const OffsetAllocator::Allocation& allocation) noexcept
		{
			this->m_allocation_descriptor_heap = allocation;
		}

		void Set_ConstantBuffer(ConstantBufferType* p_constant_buffer)
		{
			RMLUI_ASSERT(p_constant_buffer && "must be valid constant buffer!");
			this->m_p_constant_buffer_override = p_constant_buffer;
		}

		void Reset_ConstantBuffer(void)
		{
			this->m_p_constant_buffer_override = nullptr;
			for (uint8_t i = 0; i < _kRenderBackend_MaxConstantBuffersPerShader; ++i)
			{
				this->m_constant_buffer_root_parameter_indicies[i] = _kRenderBackend_InvalidConstantBuffer_RootParameterIndex;
			}
		}

		ConstantBufferType* Get_ConstantBuffer(void) const { return this->m_p_constant_buffer_override; }

		// use this only for shared CBV that will be used among all shaders otherwise we need to provide a new array that will hold a GPU addresses to
		// different cbv and their indicies but for now it is only for ONE CBV that can be used among vertex and pixel shaders
		void Add_ConstantBufferRootParameterIndicies(uint32_t index)
		{
#ifdef RMLUI_DEBUG
			bool found_empty_slot = false;
#endif

			for (unsigned char i = 0; i < _kRenderBackend_MaxConstantBuffersPerShader; ++i)
			{
				if (m_constant_buffer_root_parameter_indicies[i] == _kRenderBackend_InvalidConstantBuffer_RootParameterIndex)
				{
#ifdef RMLUI_DEBUG
					found_empty_slot = true;
#endif

					m_constant_buffer_root_parameter_indicies[i] = index;

					break;
				}
			}

#ifdef RMLUI_DEBUG
			RMLUI_ASSERT(found_empty_slot &&
				"failed to obtain empty slot in such case you have to set another limits for engine using "
				"_kRenderBackend_MaxConstantBuffersPerShader field");
#endif
		}

		const uint32_t* Get_ConstantBufferRootParameterIndicies(uint8_t& amount_of_indicies) const
		{
			amount_of_indicies = 0;

			// might be slow for big arrays otherwise provide a cache field for this class just like field that will show current size of
			// m_constant_buffer_root_parameter_indicies member in terms of current entries that are filled the array for now just to reduce the
			// memory footprint for this class and I don't use this cache field as size so we determine in runtime
			for (uint8_t i = 0; i < _kRenderBackend_MaxConstantBuffersPerShader; ++i)
			{
				if (this->m_constant_buffer_root_parameter_indicies[i] != _kRenderBackend_InvalidConstantBuffer_RootParameterIndex)
				{
					amount_of_indicies = i + 1;
				}
			}

			return this->m_constant_buffer_root_parameter_indicies;
		}

	private:
		int m_num_vertices;
		int m_num_indecies;
		ConstantBufferType* m_p_constant_buffer_override;
		size_t m_one_element_vertex_size;
		size_t m_one_element_index_size;
		uint32_t m_constant_buffer_root_parameter_indicies[_kRenderBackend_MaxConstantBuffersPerShader];
		GraphicsAllocationInfo m_info_vertex;
		GraphicsAllocationInfo m_info_index;
		OffsetAllocator::Allocation m_allocation_descriptor_heap;
	};

	class BufferMemoryManager {
	public:
		BufferMemoryManager();
		~BufferMemoryManager();

		void Initialize(ID3D12Device* m_p_device, D3D12MA::Allocator* p_allocator,
			OffsetAllocator::Allocator* p_offset_allocator_for_descriptor_heap_srv_cbv_uav, D3D12_CPU_DESCRIPTOR_HANDLE* p_handle,
			uint32_t size_descriptor_element, size_t size_for_allocation = RMLUI_RENDER_BACKEND_FIELD_VIDEOMEMORY_FOR_BUFFER_ALLOCATION,
			size_t size_alignment = RMLUI_RENDER_BACKEND_FIELD_ALIGNMENT_FOR_BUFFER);
		void Shutdown();

		void Alloc_Vertex(const void* p_data, int num_vertices, size_t size_of_one_element_in_p_data, GeometryHandleType* p_handle);
		void Alloc_Index(const void* p_data, int num_vertices, size_t size_of_one_element_in_p_data, GeometryHandleType* p_handle);

		GraphicsAllocationInfo Alloc_ConstantBuffer(ConstantBufferType* p_resource, size_t size);

		void Free_ConstantBuffer(ConstantBufferType* p_constantbuffer);
		void Free_Geometry(GeometryHandleType* p_geometryhandle);

		void* Get_WritableMemoryFromBufferByOffset(const GraphicsAllocationInfo& info);

		D3D12MA::Allocation* Get_BufferByIndex(int buffer_index);

		bool Is_Initialized(void) const;

	private:
		void Alloc_Buffer(size_t size
#ifdef RMLUI_DX_DEBUG
			,
			const std::wstring& debug_name
#endif
		);

		/// @brief searches for block that has enough memory for requested allocation size otherwise returns nullptr that means no block!
		/// @param size_for_allocation
		/// @return
		D3D12MA::VirtualBlock* Get_AvailableBlock(size_t size_for_allocation, int* p_result_buffer_index);

		D3D12MA::VirtualBlock* Get_NotOutOfMemoryAndAvailableBlock(size_t size_for_allocation, int* p_result_buffer_index);

		int Alloc(GraphicsAllocationInfo& info, size_t size, size_t alignment = 0);

		/// @brief suppose we have a such situtation that user goes to really high load document page and it was allocated too much blocks and then
		/// all its sessions are in low loaded documents and it means that we will have in that session block that weren't freed and thus we don't
		/// spend that memory for textures or for anything else, so if we have blocks that aren't in use it is better to free them
		void TryToFreeAvailableBlock();

	private:
		uint32_t m_descriptor_increment_size_srv_cbv_uav;
		size_t m_size_for_allocation_in_bytes;
		size_t m_size_alignment_in_bytes;
		ID3D12Device* m_p_device;
		D3D12MA::Allocator* m_p_allocator;
		OffsetAllocator::Allocator* m_p_offset_allocator_for_descriptor_heap_srv_cbv_uav;
		D3D12_CPU_DESCRIPTOR_HANDLE* m_p_start_pointer_of_descriptor_heap_srv_cbv_uav;
		/// @brief this is for sub allocating purposes using 'Virtual Allocation' from D3D12MA
		Rml::Vector<D3D12MA::VirtualBlock*> m_virtual_buffers;
		/// @brief this is physical representation of VRAM and uses from CPU side for binding data
		Rml::Vector<Rml::Pair<D3D12MA::Allocation*, void*>> m_buffers;
	};

	/*
	 * the key feature of this manager is texture management and if texture size is less or equal to 1.0 MB
	 * it will allocate heap for placing resources and if resource is less (or equal) to 1 MB then I will be written to that heap
	 * Otherwise will be used heap per texture (committed resource)
	 */
	class TextureMemoryManager {
	public:
		TextureMemoryManager();
		~TextureMemoryManager();

		/// @brief
		/// @param p_allocator from main manager
		/// @param size_for_placed_heap by default it is 4Mb in bytes
		void Initialize(D3D12MA::Allocator* p_allocator, OffsetAllocator::Allocator* p_offset_allocator_for_descriptor_heap_srv_cbv_uav,
			ID3D12Device* p_device, ID3D12GraphicsCommandList* p_copy_command_list, ID3D12CommandAllocator* p_copy_allocator_command,
			ID3D12DescriptorHeap* p_descriptor_heap_srv, ID3D12DescriptorHeap* p_descriptor_heap_rtv, ID3D12DescriptorHeap* p_descriptor_heap_dsv,
			ID3D12CommandQueue* p_copy_queue, D3D12_CPU_DESCRIPTOR_HANDLE* p_handle, RenderInterface_DX12* p_renderer,
			size_t size_for_placed_heap = RMLUI_RENDER_BACKEND_FIELD_VIDEOMEMORY_FOR_TEXTURE_ALLOCATION);
		void Shutdown();

		ID3D12Resource* Alloc_Texture(D3D12_RESOURCE_DESC& desc, TextureHandleType* p_impl, const Rml::byte* p_data
#ifdef RMLUI_DX_DEBUG
			,
			const Rml::String& debug_name
#endif
		);

		// if you want to create texture for rendering to it aka render target texture
		ID3D12Resource* Alloc_Texture(D3D12_RESOURCE_DESC& desc, Gfx::FramebufferData* p_impl, D3D12_RESOURCE_FLAGS flags,
			D3D12_RESOURCE_STATES initial_state
#ifdef RMLUI_DX_DEBUG
			,
			const Rml::String& debug_name
#endif
		);

		void Free_Texture(TextureHandleType* type);

		void Free_Texture(Gfx::FramebufferData* p_texture);

		void Free_Texture(TextureHandleType* p_allocated_texture_with_class, bool is_rt, D3D12MA::VirtualAllocation& allocation);

		bool Is_Initialized(void) const;

	private:
		bool CanAllocate(size_t total_memory_for_allocation, D3D12MA::VirtualBlock* p_block);

		bool CanBePlacedResource(size_t total_memory_for_allocation);

		// don't use it for MSAA texture because we don't implement a such feature!
		bool CanBeSmallResource(size_t base_memory);

		// use index for obtaining heap from m_heaps_placed
		D3D12MA::VirtualBlock* Get_AvailableBlock(size_t total_memory_for_allocation, int* result_index);

		void Alloc_As_Committed(size_t base_memory, size_t total_memory, D3D12_RESOURCE_DESC& desc, TextureHandleType* p_impl,
			const Rml::byte* p_data);

		// we don't upload to GPU because it is render target and needed to be written
		void Alloc_As_Committed(size_t base_memory, size_t total_memory, D3D12_RESOURCE_DESC& desc, D3D12_RESOURCE_STATES initial_state,
			TextureHandleType* p_texture, Gfx::FramebufferData* p_impl);

		void Alloc_As_Placed(size_t base_memory, size_t total_memory, D3D12_RESOURCE_DESC& desc, TextureHandleType* p_impl, const Rml::byte* p_data);

		void Upload(bool is_committed, TextureHandleType* p_texture_handle, const D3D12_RESOURCE_DESC& desc, const Rml::byte* p_data,
			ID3D12Resource* p_impl);

		size_t BytesPerPixel(DXGI_FORMAT format);

		size_t BitsPerPixel(DXGI_FORMAT format);

		Rml::Pair<ID3D12Heap*, D3D12MA::VirtualBlock*> Create_HeapPlaced(size_t size_for_creation);

		D3D12_CPU_DESCRIPTOR_HANDLE Alloc_RenderTargetResourceView(ID3D12Resource* p_resource, D3D12MA::VirtualAllocation* p_allocation);
		D3D12_CPU_DESCRIPTOR_HANDLE Alloc_DepthStencilResourceView(ID3D12Resource* p_resource, D3D12MA::VirtualAllocation* p_allocation);

	private:
		size_t m_size_for_placed_heap;
		/// @brief limit size that we define as acceptable. On 4 Mb it is enough for placing 1 Mb but higher it is bad. 1 Mb occupies 25% from 4 Mb
		/// and that means that m_size_limit_for_being_placed will be determined as 25% * m_size_for_placed_heap;
		size_t m_size_limit_for_being_placed;
		size_t m_size_srv_cbv_uav_descriptor;
		size_t m_size_rtv_descriptor;
		size_t m_size_dsv_descriptor;
		size_t m_fence_value;
		D3D12MA::Allocator* m_p_allocator;
		OffsetAllocator::Allocator* m_p_offset_allocator_for_descriptor_heap_srv_cbv_uav;
		ID3D12Device* m_p_device;
		ID3D12GraphicsCommandList* m_p_command_list;
		ID3D12CommandAllocator* m_p_command_allocator;
		ID3D12DescriptorHeap* m_p_descriptor_heap_srv;
		ID3D12CommandQueue* m_p_copy_queue;
		ID3D12Fence* m_p_fence;
		HANDLE m_p_fence_event;
		D3D12_CPU_DESCRIPTOR_HANDLE* m_p_handle;
		RenderInterface_DX12* m_p_renderer;
		D3D12MA::VirtualBlock* m_p_virtual_block_for_render_target_heap_allocations;
		D3D12MA::VirtualBlock* m_p_virtual_block_for_depth_stencil_heap_allocations;
		ID3D12DescriptorHeap* m_p_descriptor_heap_rtv;
		ID3D12DescriptorHeap* m_p_descriptor_heap_dsv;
		Rml::Vector<D3D12MA::VirtualBlock*> m_blocks;
		Rml::Vector<ID3D12Heap*> m_heaps_placed;
	};

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

		void Initialize(RenderInterface_DX12* p_owner);
		void Shutdown();
		// Push a new layer. All references to previously retrieved layers are invalidated.
		Rml::LayerHandle PushLayer();

		// Pop the top layer. All references to previously retrieved layers are invalidated.
		void PopLayer();

		const Gfx::FramebufferData& GetLayer(Rml::LayerHandle layer) const;
		const Gfx::FramebufferData& GetTopLayer() const;
		const Gfx::FramebufferData& Get_SharedDepthStencil_Layers();
		//	const Gfx::FramebufferData& Get_SharedDepthStencil_Postprocess();
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

	private:
		unsigned char m_msaa_sample_count;
		int m_width, m_height;
		// The number of active layers is manually tracked since we re-use the framebuffers stored in the fb_layers stack.
		int m_layers_size;
		TextureMemoryManager* m_p_manager_texture;
		BufferMemoryManager* m_p_manager_buffer;
		ID3D12Device* m_p_device;
		Gfx::FramebufferData* m_p_depth_stencil_for_layers;
		Rml::Vector<Gfx::FramebufferData> m_fb_layers;
		Rml::Vector<Gfx::FramebufferData> m_fb_postprocess;
	};

public:
	// RenderInterface_DX12(ID3D12Device* p_user_device, ID3D12CommandQueue* p_user_command_queue,
	//	ID3D12GraphicsCommandList* p_user_graphics_command_list);

	RenderInterface_DX12(ID3D12Device* p_user_device, ID3D12GraphicsCommandList* p_command_list, IDXGIAdapter* p_user_adapter,
		bool is_execute_when_end_frame_issued, int initial_width, int initial_height, const Backend::RmlRendererSettings* settings);
	RenderInterface_DX12(void* p_window_handle, const Backend::RmlRendererSettings* settings);
	~RenderInterface_DX12();

	// using CreateSurfaceCallback = bool (*)(VkInstance instance, VkSurfaceKHR* out_surface);

	// Returns true if the renderer was successfully constructed.
	explicit operator bool() const;

	// The viewport should be updated whenever the window size changes.
	void SetViewport(int viewport_width, int viewport_height);

	// Sets up OpenGL states for taking rendering commands from RmlUi.
	void BeginFrame();
	// Draws the result to the backbuffer and restores OpenGL state.
	void EndFrame();

	// Optional, can be used to clear the active framebuffer.
	void Clear();

	// -- Inherited from Rml::RenderInterface --

	Rml::CompiledGeometryHandle CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices) override;
	void RenderGeometry(Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation, Rml::TextureHandle texture) override;
	void ReleaseGeometry(Rml::CompiledGeometryHandle geometry) override;

	void EnableScissorRegion(bool enable) override;
	void SetScissorRegion(Rml::Rectanglei region) override;

	void EnableClipMask(bool enable) override;
	void RenderToClipMask(Rml::ClipMaskOperation mask_operation, Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation) override;

	Rml::TextureHandle LoadTexture(Rml::Vector2i& texture_dimensions, const Rml::String& source) override;
	Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte> source_data, Rml::Vector2i source_dimensions) override;
	void ReleaseTexture(Rml::TextureHandle texture_handle) override;

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

	void Shutdown() noexcept;
	void Initialize(void) noexcept;

	bool IsSwapchainValid() noexcept;
	void RecreateSwapchain() noexcept;

	ID3D12Fence* Get_Fence(void);
	HANDLE Get_FenceEvent(void);
	Rml::Array<uint64_t, RMLUI_RENDER_BACKEND_FIELD_SWAPCHAIN_BACKBUFFER_COUNT>& Get_FenceValues(void);
	uint32_t Get_CurrentFrameIndex(void);

	ID3D12Device* Get_Device(void) const;
	TextureMemoryManager& Get_TextureManager(void);
	BufferMemoryManager& Get_BufferManager(void);

	unsigned char Get_MSAASampleCount(void) const;

	void Set_UserFramebufferIndex(unsigned char framebuffer_index);
	void Set_UserRenderTarget(void* rtv_where_we_render_to);
	void Set_UserDepthStencil(void* dsv_where_we_render_to);

	bool CaptureScreen(int& width, int& height, int& num_components, int& row_pitch, Rml::byte*& raw_pixels, size_t& pixels_count) override;

private:
	void BeginFrame_Shell();
	void BeginFrame_Integration();

	void EndFrame_Shell();
	void EndFrame_Integration();

	void Clear_Shell();
	void Clear_Integration();

	void SetViewport_Shell(int viewport_width, int viewport_height);
	void SetViewport_Integration(int viewport_width, int viewport_height);

	void Initialize_Device(void) noexcept;
	void Initialize_Adapter(void) noexcept;
	void Initialize_DebugLayer(void) noexcept;

	void Initialize_Swapchain(int width, int height) noexcept;
	void Initialize_SyncPrimitives(void) noexcept;
	void Initialize_SyncPrimitives_Screenshot(void) noexcept;
	void Initialize_CommandAllocators(void);

	void Initialize_Allocator(void) noexcept;

	void Destroy_Swapchain() noexcept;
	void Destroy_SyncPrimitives(void) noexcept;
	void Destroy_CommandAllocators(void) noexcept;
	void Destroy_CommandList(void) noexcept;
	void Destroy_Allocator(void) noexcept;
	void Destroy_SyncPrimitives_Screenshot(void) noexcept;

	void Flush() noexcept;
	uint64_t Signal(uint32_t frame_index) noexcept;
	void WaitForFenceValue(uint32_t frame_index);

	void Create_Resources_DependentOnSize() noexcept;
	void Destroy_Resources_DependentOnSize() noexcept;

	ID3D12DescriptorHeap* Create_Resource_DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags,
		uint32_t descriptor_count) noexcept;

	ID3D12CommandAllocator* Create_CommandAllocator(D3D12_COMMAND_LIST_TYPE type);
	ID3D12GraphicsCommandList* Create_CommandList(ID3D12CommandAllocator* p_allocator, D3D12_COMMAND_LIST_TYPE type) noexcept;

	ID3D12CommandQueue* Create_CommandQueue(D3D12_COMMAND_LIST_TYPE type) noexcept;

	void Create_Resource_RenderTargetViews();
	void Destroy_Resource_RenderTagetViews();

	// pipelines

	void Create_Resource_For_Shaders(void);
	void Destroy_Resource_For_Shaders(void);

	void Create_Resource_Pipelines();
	void Create_Resource_Pipeline_Color();
	void Create_Resource_Pipeline_Texture();
	void Create_Resource_Pipeline_Gradient();
	void Create_Resource_Pipeline_Creation();
	void Create_Resource_Pipeline_Passthrough();
	void Create_Resource_Pipeline_Passthrough_ColorMask();
	void Create_Resource_Pipeline_Passthrough_NoBlend();
	void Create_Resource_Pipeline_ColorMatrix();
	void Create_Resource_Pipeline_BlendMask();
	void Create_Resource_Pipeline_Blur();
	void Create_Resource_Pipeline_DropShadow();
	void Create_Resource_Pipeline_Count();

	void Create_Resource_DepthStencil();
	void Destroy_Resource_DepthStencil();

	void Destroy_Resource_Pipelines();

	bool CheckTearingSupport() noexcept;

	IDXGIAdapter* Get_Adapter(bool is_use_warp) noexcept;

	void PrintAdapterDesc(IDXGIAdapter* p_adapter);

	void SetScissor(Rml::Rectanglei region, bool vertically_flip = false);

	void SubmitTransformUniform(ConstantBufferType& constant_buffer, const Rml::Vector2f& translation);

	void UseProgram(ProgramId pipeline_id);

	ConstantBufferType* Get_ConstantBuffer(uint32_t current_back_buffer_index);

	void Free_Geometry(GeometryHandleType* p_handle);
	void Free_Texture(TextureHandleType* p_handle);

	void Update_PendingForDeletion_Geometry();
	void Update_PendingForDeletion_Texture();

	void BlitLayerToPostprocessPrimary(Rml::LayerHandle layer_id);

	void RenderFilters(Rml::Span<const Rml::CompiledFilterHandle> filter_handles);
	void RenderBlur(float sigma, const Gfx::FramebufferData& source_destination, const Gfx::FramebufferData& temp,
		const Rml::Rectanglei window_flipped);

	void DrawFullscreenQuad(ConstantBufferType* p_override_constant_buffer = nullptr);
	void DrawFullscreenQuad(Rml::Vector2f uv_offset, Rml::Vector2f uv_scaling = Rml::Vector2f(1.f),
		ConstantBufferType* p_override_constant_buffer = nullptr);

	void BindTexture(TextureHandleType* p_texture, UINT root_parameter_index = 0);
	void BindRenderTarget(const Gfx::FramebufferData& framebuffer, bool depth_included = true);

	void OverrideConstantBufferOfGeometry(Rml::CompiledGeometryHandle geometry, ConstantBufferType* p_override_constant_buffer);

	// 1 means not supported
	// otherwise return max value of supported multisample count
	unsigned char GetMSAASupportedSampleCount(unsigned char max_samples);

	void BlitFramebuffer(const Gfx::FramebufferData& source, const Gfx::FramebufferData& dest, int srcX0, int srcY0, int srcX1, int srcY1, int dstX0,
		int dstY0, int dstX1, int dstY1);

	// debug only
	void ValidateTextureAllocationNotAsPlaced(const Gfx::FramebufferData& data);

	ID3D12Resource* GetResourceFromFramebufferData(const Gfx::FramebufferData& data);

private:
	bool m_is_full_initialization;
	bool m_is_shutdown_called;
	bool m_is_use_vsync;
	bool m_is_use_tearing;
	bool m_is_scissor_was_set;
	bool m_is_stencil_enabled;
	bool m_is_stencil_equal;
	bool m_is_use_msaa;
	bool m_is_execute_when_end_frame_issued;
	bool m_is_command_list_user;
	unsigned char m_msaa_sample_count;
	unsigned char m_user_framebuffer_index;
	/// @brief current viewport's width
	int m_width;
	/// @brief current viewport's height
	int m_height;
	int m_current_clip_operation;
	ProgramId m_active_program_id;
	Rml::Rectanglei m_scissor;
	uint32_t m_size_descriptor_heap_render_target_view;
	uint32_t m_size_descriptor_heap_shaders;
	UINT m_current_back_buffer_index;
	UINT m_stencil_ref_value;
	/// @brief depends on compile build type if it is debug it means D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION otherwise it is 0
	UINT m_default_shader_flags;
	std::bitset<RMLUI_RENDER_BACKEND_FIELD_MAXNUMPROGRAMS> m_program_state_transform_dirty;
	ID3D12Device* m_p_device;
	ID3D12CommandQueue* m_p_command_queue;
	ID3D12CommandQueue* m_p_copy_queue;
	IDXGISwapChain4* m_p_swapchain;
	ID3D12GraphicsCommandList* m_p_command_graphics_list;
	ID3D12GraphicsCommandList* m_p_command_graphics_list_screenshot;
	ID3D12CommandAllocator* m_p_command_allocator_screenshot;
	ID3D12DescriptorHeap* m_p_descriptor_heap_render_target_view;
	ID3D12DescriptorHeap* m_p_descriptor_heap_render_target_view_for_texture_manager;
	ID3D12DescriptorHeap* m_p_descriptor_heap_depth_stencil_view_for_texture_manager;
	// cbv; srv; uav; all in one
	ID3D12DescriptorHeap* m_p_descriptor_heap_shaders;
	ID3D12DescriptorHeap* m_p_descriptor_heap_depthstencil;
	D3D12MA::Allocation* m_p_depthstencil_resource;
	ID3D12Fence* m_p_backbuffer_fence;
	ID3D12Fence* m_p_fence_screenshot;
	IDXGIAdapter* m_p_adapter;

	ID3D12PipelineState* m_pipelines[23];
	ID3D12RootSignature* m_root_signatures[23];

	ID3D12CommandAllocator* m_p_copy_allocator;
	ID3D12GraphicsCommandList* m_p_copy_command_list;

	D3D12MA::Allocator* m_p_allocator;
	OffsetAllocator::Allocator* m_p_offset_allocator_for_descriptor_heap_shaders;
	// D3D12MA::Allocation* m_p_constant_buffers[kSwapchainBackBufferCount];
	// where user wants to render rmlui final image
	D3D12_CPU_DESCRIPTOR_HANDLE* m_p_user_rtv_present;
	// as well as rtv just dsv
	D3D12_CPU_DESCRIPTOR_HANDLE* m_p_user_dsv_present;
	HWND m_p_window_handle;
	HANDLE m_p_fence_event;
	HANDLE m_p_fence_event_screenshot;
	uint64_t m_fence_value;
	uint64_t m_fence_screenshot_value;
	Rml::CompiledGeometryHandle m_precompiled_fullscreen_quad_geometry;

	Rml::Array<ID3D12Resource*, RMLUI_RENDER_BACKEND_FIELD_SWAPCHAIN_BACKBUFFER_COUNT> m_backbuffers_resources;
	Rml::Array<ID3D12CommandAllocator*, RMLUI_RENDER_BACKEND_FIELD_SWAPCHAIN_BACKBUFFER_COUNT> m_backbuffers_allocators;
	Rml::Array<uint64_t, RMLUI_RENDER_BACKEND_FIELD_SWAPCHAIN_BACKBUFFER_COUNT> m_backbuffers_fence_values;
	Rml::Array<size_t, RMLUI_RENDER_BACKEND_FIELD_SWAPCHAIN_BACKBUFFER_COUNT> m_constant_buffer_count_per_frame;
	Rml::Array<size_t, RMLUI_RENDER_BACKEND_FIELD_SWAPCHAIN_BACKBUFFER_COUNT> m_vertex_and_index_buffer_count_per_frame;
	// per object (per draw)
	Rml::Array<Rml::Vector<ConstantBufferType>, RMLUI_RENDER_BACKEND_FIELD_SWAPCHAIN_BACKBUFFER_COUNT> m_constantbuffers;
	Rml::Vector<GeometryHandleType*> m_pending_for_deletion_geometry;
	// todo: delete this if final implementation doesn't use this
	Rml::Vector<TextureHandleType*> m_pending_for_deletion_textures;
	// ConstantBufferType m_constantbuffer;

	// this represents user's data from initialization structure about multisampling features
	DXGI_SAMPLE_DESC m_desc_sample;
	D3D12_CPU_DESCRIPTOR_HANDLE m_handle_shaders;
	BufferMemoryManager m_manager_buffer;
	TextureMemoryManager m_manager_texture;
	Rml::Matrix4f m_constant_buffer_data_transform;
	Rml::Matrix4f m_constant_buffer_data_projection;
	Rml::Matrix4f m_projection;
	RenderLayerStack m_manager_render_layer;
};

// forward declaration
namespace Backend {
struct RmlRenderInitInfo;
}

namespace RmlDX12 {

// If you pass a second argument and the second argument is valid you will initialize renderer partially, it means that it will be integrated into
// your engine thus it WILL NOT create device, command queue and etc, but if you don't have own renderer system and you don't want to initialize
// DirectX by your own you just don't need to pass anything (or nullptr) to second argument, so Rml will initialize renderer fully. Optionally, the
// out message describes the loaded GL version or an error message on failure.
RenderInterface_DX12* Initialize(Rml::String* out_message, Backend::RmlRenderInitInfo* p_info);

/// @brief you need to destroy allocated object manually it just calls shutdown method!
/// @param p_instance allocated instance from RmlDX12::Initialize method
void Shutdown(RenderInterface_DX12* p_instance);

} // namespace RmlDX12

#endif
