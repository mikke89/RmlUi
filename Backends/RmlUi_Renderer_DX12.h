/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019-2023 The RmlUi Team, and contributors
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
#ifdef RMLUI_PLATFORM_WIN32
	#include "RmlUi_Include_Windows.h"
#endif

#if (_MSC_VER > 0)
	#define RMLUI_DISABLE_ALL_COMPILER_WARNINGS_PUSH _Pragma("warning(push, 0)")
	#define RMLUI_DISABLE_ALL_COMPILER_WARNINGS_POP _Pragma("warning(pop)")
#elif __clang__
	#define RMLUI_DISABLE_ALL_COMPILER_WARNINGS_PUSH                                                                                   \
		_Pragma("clang diagnostic push") _Pragma("clang diagnostic ignored \"-Wall\"") _Pragma("clang diagnostic ignored \"-Wextra\"") \
			_Pragma("clang diagnostic ignored \"-Wnullability-extension\"")                                                            \
				_Pragma("clang diagnostic ignored \"-Wgnu-zero-variadic-macro-arguments\"")                                            \
					_Pragma("clang diagnostic ignored \"-Wnullability-completeness\"")
	#define RMLUI_DISABLE_ALL_COMPILER_WARNINGS_POP _Pragma("clang diagnostic pop")
#elif __GNUC__
	#define RMLUI_DISABLE_ALL_COMPILER_WARNINGS_PUSH                                                                                       \
		_Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wimplicit-fallthrough\"")                                        \
			_Pragma("GCC diagnostic ignored \"-Wunused-function\"") _Pragma("GCC diagnostic ignored \"-Wunused-parameter\"")               \
				_Pragma("GCC diagnostic ignored \"-Wunused-variable\"") _Pragma("GCC diagnostic ignored \"-Wmissing-field-initializers\"") \
					_Pragma("GCC diagnostic ignored \"-Wswitch\"") _Pragma("GCC diagnostic ignored \"-Wpedantic\"")                        \
						_Pragma("GCC diagnostic ignored \"-Wattributes\"") _Pragma("GCC diagnostic ignored \"-Wignored-qualifiers\"")      \
							_Pragma("GCC diagnostic ignored \"-Wparentheses\"")
	#define RMLUI_DISABLE_ALL_COMPILER_WARNINGS_POP _Pragma("GCC diagnostic pop")
#else
	#define RMLUI_DISABLE_ALL_COMPILER_WARNINGS_PUSH
	#define RMLUI_DISABLE_ALL_COMPILER_WARNINGS_POP
#endif

RMLUI_DISABLE_ALL_COMPILER_WARNINGS_PUSH

RMLUI_DISABLE_ALL_COMPILER_WARNINGS_POP

#ifdef RMLUI_DEBUG
	#define RMLUI_DX_ASSERTMSG(statement, msg) RMLUI_ASSERTMSG(SUCCEEDED(statement), msg)

	// Uncomment the following line to enable additional DirectX debugging.
	#define RMLUI_DX_DEBUG
#else
	#define RMLUI_DX_ASSERTMSG(statement, msg) static_cast<void>(statement)
#endif

#ifdef RMLUI_PLATFORM_WIN32
	#include <directx/d3dx12.h>
	#include <d3dcompiler.h>
	#include <dxgi1_6.h>
	#include <chrono>
	#include "RmlUi_DirectX/D3D12MemAlloc.h"
	#include "RmlUi_DirectX/offsetAllocator.hpp"
	#include <bitset>

	// user's preprocessor overrides

	#ifdef RMLUI_RENDER_BACKEND_OVERRIDE_FIELD_RESERVECOUNT_OF_RENDERSTACK_LAYERS
		// system field (it is not supposed to specify by initialization structure)
		// on some render backend implementations (Vulkan/DirectX-12) we have to check memory leaks but
		// if we don't reserve memory for a field that contains layers (it is vector)
		// at runtime we will get a called dtor of move-to-copy object (because of reallocation)
		// and for that matter we will get a false-positive trigger of assert and it is not right generally
		#define RMLUI_RENDER_BACKEND_FIELD_RESERVECOUNT_OF_RENDERSTACK_LAYERS RMLUI_RENDER_BACKEND_OVERRIDE_FIELD_RESERVECOUNT_OF_RENDERSTACK_LAYERS
	#else
		// system field (it is not supposed to specify by initialization structure)
		// on some render backend implementations (Vulkan/DirectX-12) we have to check memory leaks but
		// if we don't reserve memory for a field that contains layers (it is vector)
		// at runtime we will get a called dtor of move-to-copy object (because of reallocation)
		// and for that matter we will get a false-positive trigger of assert and it is not right generally
		#define RMLUI_RENDER_BACKEND_FIELD_RESERVECOUNT_OF_RENDERSTACK_LAYERS 6
	#endif

	#ifdef RMLUI_RENDER_BACKEND_OVERRIDE_FIELD_SWAPCHAIN_BACKBUFFER_COUNT
		// this field specifies the default amount of swapchain buffer that will be created
		// but it is used only when user provided invalid input in initialization structure of backend
		#define RMLUI_RENDER_BACKEND_FIELD_SWAPCHAIN_BACKBUFFER_COUNT RMLUI_RENDER_BACKEND_OVERRIDE_FIELD_SWAPCHAIN_BACKBUFFER_COUNT
static_assert(RMLUI_RENDER_BACKEND_FIELD_SWAPCHAIN_BACKBUFFER_COUNT > 0 && "invalid value for RMLUI_RENDER_BACKEND_FIELD_SWAPCHAIN_BACKBUFFER_COUNT");
	#else
		// this field specifies the default amount of swapchain buffer that will be created
		// but it is used only when user provided invalid input in initialization structure of backend
		#define RMLUI_RENDER_BACKEND_FIELD_SWAPCHAIN_BACKBUFFER_COUNT 3
	#endif

	#ifdef RMLUI_RENDER_BACKEND_OVERRIDE_FIELD_VIDEOMEMORY_FOR_BUFFER_ALLOCATION
		// this field specifies the default amount of videomemory that will be used on creation
		// this field is used when user provided invalid input in initialization structure of backend
		#define RMLUI_RENDER_BACKEND_FIELD_VIDEOMEMORY_FOR_BUFFER_ALLOCATION RMLUI_RENDER_BACKEND_OVERRIDE_FIELD_VIDEOMEMORY_FOR_BUFFER_ALLOCATION
static_assert(RMLUI_RENDER_BACKEND_FIELD_VIDEOMEMORY_FOR_BUFFER_ALLOCATION > 0 &&
	"invalid value for RMLUI_RENDER_BACKEND_FIELD_VIDEOMEMORY_FOR_BUFFER_ALLOCATION");
	#else
		// this field specifies the default amount of videomemory that will be used on creation
		// this field is used when user provided invalid input in initialization structure of backend
		#define RMLUI_RENDER_BACKEND_FIELD_VIDEOMEMORY_FOR_BUFFER_ALLOCATION 1048576 // (4 * 512 * 512 = bytes or 1 Megabytes)
	#endif

	#ifdef RMLUI_RENDER_BACKEND_OVERRIDE_FIELD_VIDEOMEMORY_FOR_TEXTURE_ALLOCATION
		// this field specifies the default amount of videomemory that will be used for texture creation
		// on some backends it determines the size of a temp buffer that might be used not trivial (just for uploading texture by copying data in it)
		// like on DirectX-12 it has a placement resources feature and making this value lower (4 Mb) the placement resource feature becomes pointless
		// and doesn't gain any performance related boosting
		#define RMLUI_RENDER_BACKEND_FIELD_VIDEOMEMORY_FOR_TEXTURE_ALLOCATION RMLUI_RENDER_BACKEND_OVERRIDE_FIELD_VIDEOMEMORY_FOR_TEXTURE_ALLOCATION
static_assert(RMLUI_RENDER_BACKEND_FIELD_VIDEOMEMORY_FOR_TEXTURE_ALLOCATION > 0 &&
	"invalid value for RMLUI_RENDER_BACKEND_FIELD_VIDEOMEMORY_FOR_TEXTURE_ALLOCATION");
	#else
		// this field specifies the default amount of videomemory that will be used for texture creation
		// on some backends it determines the size of a temp buffer that might be used not trivial (just for uploading texture by copying data in it)
		// like on DirectX-12 it has a placement resources feature and making this value lower (4 Mb) the placement resource feature becomes pointless
		// and doesn't gain any performance related boosting
		#define RMLUI_RENDER_BACKEND_FIELD_VIDEOMEMORY_FOR_TEXTURE_ALLOCATION 4194304 // (4 * 1024 * 1024 = bytes or 4 Megabytes)
	#endif

	#ifdef RMLUI_RENDER_BACKEND_OVERRIDE_FIELD_ALIGNMENT_FOR_BUFFER
		// system field that is not supposed to be in initialization structure and used as default (input user doesn't affect it at all like
		// we always use it, not just because we handling invalid input from user)
		#define RMLUI_RENDER_BACKEND_FIELD_ALIGNMENT_FOR_BUFFER RMLUI_RENDER_BACKEND_OVERRIDE_FIELD_ALIGNMENT_FOR_BUFFER
	#else
		// system field that is not supposed to be in initialization structure and used as default (input user doesn't affect it at all like we always
		// use it, not just because we handling invalid input from user)
		#define RMLUI_RENDER_BACKEND_FIELD_ALIGNMENT_FOR_BUFFER D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT
	#endif

	#ifdef RMLUI_RENDER_BACKEND_OVERRIDE_FIELD_COLOR_TEXTURE_FORMAT
		// system field that is not supposed to be in initialization structure and used as default (input user doesn't affect it at all like we always
		// use it, not just because we handling invalid input from user)
		#define RMLUI_RENDER_BACKEND_FIELD_COLOR_TEXTURE_FORMAT RMLUI_RENDER_BACKEND_OVERRIDE_FIELD_COLOR_TEXTURE_FORMAT
	#else
		// system field that is not supposed to be in initialization structure and used as default (input user doesn't affect it at all like we always
		// use it, not just because we handling invalid input from user)
		#define RMLUI_RENDER_BACKEND_FIELD_COLOR_TEXTURE_FORMAT DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM
	#endif

	#ifdef RMLUI_REDNER_BACKEND_OVERRIDE_FIELD_DEPTHSTENCIL_TEXTURE_FORMAT
		#define RMLUI_RENDER_BACKEND_FIELD_DEPTHSTENCIL_TEXTURE_FORMAT RMLUI_REDNER_BACKEND_OVERRIDE_FIELD_DEPTHSTENCIL_TEXTURE_FORMAT
	#else
		// system field that is not supposed to be in initialization structure and used as default (input user doesn't affect it at all like we always
		// use it, not just because we handling invalid input from user)
		#define RMLUI_RENDER_BACKEND_FIELD_DEPTHSTENCIL_TEXTURE_FORMAT DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT
	#endif

	#ifdef RMLUI_RENDER_BACKEND_OVERRIDE_FIELD_DESCRIPTORAMOUNT_FOR_SRV_CBV_UAV
		// system field that is not supposed to be in initialization structure and used as default (input user doesn't affect it at all like we always
		// use it, not just because we handling invalid input from user)
		// notice: this field is shared for all srv and cbv and uav it doesn't mean that it specifically allocates for srv, cbv and uav
		#define RMLUI_RENDER_BACKEND_FIELD_DESCRIPTORAMOUNT_FOR_SRV_CBV_UAV RMLUI_RENDER_BACKEND_OVERRIDE_FIELD_DESCRIPTORAMOUNT_FOR_SRV_CBV_UAV
	#else
		// system field that is not supposed to be in initialization structure and used as default (input user doesn't affect it at all like we always
		// use it, not just because we handling invalid input from user)
		// notice: this field is shared for all srv and cbv and uav it doesn't mean that it specifically allocates for srv 128, cbv 128 and uav 128,
		// no! it allocates only on descriptor for all of such types and total amount is 128 not 3 * 128 = 384 !!!
		#define RMLUI_RENDER_BACKEND_FIELD_DESCRIPTORAMOUNT_FOR_SRV_CBV_UAV 128
	#endif

	#ifdef RMLUI_RENDER_BACKEND_OVERRIDE_FIELD_MAXNUMPROGRAMS
		#define RMLUI_RENDER_BACKEND_FIELD_MAXNUMPROGRAMS RMLUI_RENDER_BACKEND_OVERRIDE_FIELD_MAXNUMPROGRAMS
	#else
		#define RMLUI_RENDER_BACKEND_FIELD_MAXNUMPROGRAMS 32
	#endif

	#ifdef RMLUI_RENDER_BACKEND_OVERRIDE_FIELD_PREALLOCATED_CONSTANTBUFFERS
		#define RMLUI_RENDER_BACKEND_FIELD_PREALLOCATED_CONSTANTBUFFERS RMLUI_RENDER_BACKEND_OVERRIDE_FIELD_PREALLOCATED_CONSTANTBUFFERS
	#else
		// for getting total size of constant buffers you should multiply kPreAllocatedConstantBuffers * kSwapchainBackBufferCount e.g. 250 * 3 = 750
		#define RMLUI_RENDER_BACKEND_FIELD_PREALLOCATED_CONSTANTBUFFERS 250
	#endif

	#ifdef RMLUI_RENDER_BACKEND_OVERRIDE_FIELD_MSAA_SAMPLE_COUNT
		#define RMLUI_RENDER_BACKEND_FIELD_MSAA_SAMPLE_COUNT RMLUI_RENDER_BACKEND_OVERRIDE_FIELD_MSAA_SAMPLE_COUNT
	#else
		#define RMLUI_RENDER_BACKEND_FIELD_MSAA_SAMPLE_COUNT 2
	#endif	

	#ifdef RMLUI_RENDER_BACKEND_OVERRIDE_FIELD_DESCRIPTOR_HEAP_RTV
		#define RMLUI_RENDER_BACKEND_FIELD_DESCRIPTOR_HEAP_RTV RMLUI_RENDER_BACKEND_OVERRIDE_FIELD_DESCRIPTOR_HEAP_RTV
	#else
		#define RMLUI_RENDER_BACKEND_FIELD_DESCRIPTOR_HEAP_RTV 8
	#endif

	#ifdef RMLUI_RENDER_BACKEND_OVERRIDE_FIELD_DESCRIPTOR_HEAP_DSV
		#define RMLUI_RENDER_BACKEND_FIELD_DESCRIPTOR_HEAP_DSV RMLUI_RENDER_BACKEND_OVERRIDE_FIELD_DESCRIPTOR_HEAP_DSV
	#else
		#define RMLUI_RENDER_BACKEND_FIELD_DESCRIPTOR_HEAP_DSV 8
	#endif

enum class ProgramId;

class RenderLayerStack;
namespace Gfx {
struct ProgramData;
struct FramebufferData;
} // namespace Gfx

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
		GeometryHandleType(void) : m_num_vertices{}, m_num_indecies{}, m_one_element_vertex_size{}, m_one_element_index_size{} {}

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

	private:
		int m_num_vertices;
		int m_num_indecies;
		size_t m_one_element_vertex_size;
		size_t m_one_element_index_size;
		GraphicsAllocationInfo m_info_vertex;
		GraphicsAllocationInfo m_info_index;
		OffsetAllocator::Allocation m_allocation_descriptor_heap;
	};

	class BufferMemoryManager {
	public:
		BufferMemoryManager();
		~BufferMemoryManager();

		void Initialize(ID3D12Device* m_p_device, D3D12MA::Allocator* p_allocator,
			OffsetAllocator::Allocator* p_offset_allocator_for_descriptor_heap_srv_cbv_uav, CD3DX12_CPU_DESCRIPTOR_HANDLE* p_handle,
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
			const Rml::String& debug_name
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
		CD3DX12_CPU_DESCRIPTOR_HANDLE* m_p_start_pointer_of_descriptor_heap_srv_cbv_uav;
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
			ID3D12CommandQueue* p_copy_queue, CD3DX12_CPU_DESCRIPTOR_HANDLE* p_handle, RenderInterface_DX12* p_renderer,
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
		CD3DX12_CPU_DESCRIPTOR_HANDLE* m_p_handle;
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

		// Push a new layer. All references to previously retrieved layers are invalidated.
		Rml::LayerHandle PushLayer();

		// Pop the top layer. All references to previously retrieved layers are invalidated.
		void PopLayer();

		const Gfx::FramebufferData& GetLayer(Rml::LayerHandle layer) const;
		const Gfx::FramebufferData& GetTopLayer() const;
		const Gfx::FramebufferData& Get_SharedDepthStencil();
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
		int m_width, m_height;
		// The number of active layers is manually tracked since we re-use the framebuffers stored in the fb_layers stack.
		int m_layers_size;
		TextureMemoryManager* m_p_manager_texture;
		BufferMemoryManager* m_p_manager_buffer;
		ID3D12Device* m_p_device;
		Gfx::FramebufferData* m_p_depth_stencil;
		Rml::Vector<Gfx::FramebufferData> m_fb_layers;
		Rml::Vector<Gfx::FramebufferData> m_fb_postprocess;
	};

public:
	// RenderInterface_DX12(ID3D12Device* p_user_device, ID3D12CommandQueue* p_user_command_queue,
	//	ID3D12GraphicsCommandList* p_user_graphics_command_list);

	RenderInterface_DX12(void* p_window_handle, ID3D12Device* p_user_device, IDXGISwapChain* p_user_swapchain, bool use_vsync);
	RenderInterface_DX12(void* p_window_handle, bool use_vsync);
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

	Rml::TextureHandle SaveLayerAsTexture(Rml::Vector2i dimensions) override;

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

private:
	void Initialize_Device(void) noexcept;
	void Initialize_Adapter(void) noexcept;
	void Initialize_DebugLayer(void) noexcept;

	void Initialize_Swapchain(int width, int height) noexcept;
	void Initialize_SyncPrimitives(void) noexcept;
	void Initialize_CommandAllocators(void);

	void Initialize_Allocator(void) noexcept;

	void Destroy_Swapchain() noexcept;
	void Destroy_SyncPrimitives(void) noexcept;
	void Destroy_CommandAllocators(void) noexcept;
	void Destroy_CommandList(void) noexcept;
	void Destroy_Allocator(void) noexcept;

	void Flush() noexcept;
	uint64_t Signal() noexcept;
	void WaitForFenceValue(uint64_t fence_value, std::chrono::milliseconds time = std::chrono::milliseconds::max());

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
	void Create_Resource_For_Shaders_ConstantBufferHeap(void);
	void Destroy_Resource_For_Shaders_ConstantBufferHeap(void);
	void Destroy_Resource_For_Shaders(void);

	void Create_Resource_Pipelines();
	void Create_Resource_Pipeline_Color();
	void Create_Resource_Pipeline_Texture();
	void Create_Resource_Pipeline_Gradient();
	void Create_Resource_Pipeline_Creation();
	void Create_Resource_Pipeline_Passthrough();
	void Create_Resource_Pipeline_ColorMatrix();
	void Create_Resource_Pipeline_BlendMask();
	void Create_Resource_Pipeline_Blur();
	void Create_Resource_Pipeline_DropShadow();
	void Create_Resource_Pipeline_Count();

	void Create_Resource_DepthStencil();
	void Destroy_Resource_DepthStencil();

	void Destroy_Resource_Pipelines();

	ID3DBlob* Compile_Shader(const Rml::String& relative_path_to_shader, const char* entry_point, const char* shader_version, UINT flags);

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

	void DrawFullscreenQuad();
	void DrawFullscreenQuad(Rml::Vector2f uv_offset, Rml::Vector2f uv_scaling = Rml::Vector2f(1.f));

private:
	bool m_is_full_initialization;
	bool m_is_shutdown_called;
	bool m_is_use_vsync;
	bool m_is_use_tearing;
	bool m_is_scissor_was_set;
	bool m_is_stencil_enabled;
	bool m_is_stencil_equal;
	int m_width;
	int m_height;
	int m_current_clip_operation;
	ProgramId m_active_program_id;
	Rml::Rectanglei m_scissor;
	uint32_t m_size_descriptor_heap_render_target_view;
	uint32_t m_size_descriptor_heap_shaders;
	uint32_t m_current_back_buffer_index;

	/// @brief depends on compile build type if it is debug it means D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION otherwise it is 0
	UINT m_default_shader_flags;
	std::bitset<RMLUI_RENDER_BACKEND_FIELD_MAXNUMPROGRAMS> m_program_state_transform_dirty;
	ID3D12Device* m_p_device;
	ID3D12CommandQueue* m_p_command_queue;
	ID3D12CommandQueue* m_p_copy_queue;
	IDXGISwapChain4* m_p_swapchain;
	ID3D12GraphicsCommandList* m_p_command_graphics_list;
	ID3D12DescriptorHeap* m_p_descriptor_heap_render_target_view;
	ID3D12DescriptorHeap* m_p_descriptor_heap_render_target_view_for_texture_manager;
	ID3D12DescriptorHeap* m_p_descriptor_heap_depth_stencil_view_for_texture_manager;
	// cbv; srv; uav; all in one
	ID3D12DescriptorHeap* m_p_descriptor_heap_shaders;
	ID3D12DescriptorHeap* m_p_descriptor_heap_depthstencil;
	D3D12MA::Allocation* m_p_depthstencil_resource;
	ID3D12Fence* m_p_backbuffer_fence;
	IDXGIAdapter* m_p_adapter;

	ID3D12PipelineState* m_pipelines[17];
	ID3D12RootSignature* m_root_signatures[17];

	ID3D12CommandAllocator* m_p_copy_allocator;
	ID3D12GraphicsCommandList* m_p_copy_command_list;

	D3D12MA::Allocator* m_p_allocator;
	OffsetAllocator::Allocator* m_p_offset_allocator_for_descriptor_heap_shaders;
	// D3D12MA::Allocation* m_p_constant_buffers[kSwapchainBackBufferCount];

	HWND m_p_window_handle;
	HANDLE m_p_fence_event;
	uint64_t m_fence_value;
	Rml::CompiledGeometryHandle m_precompiled_fullscreen_quad_geometry;

	Rml::Array<ID3D12Resource*, RMLUI_RENDER_BACKEND_FIELD_SWAPCHAIN_BACKBUFFER_COUNT> m_backbuffers_resources;
	Rml::Array<ID3D12CommandAllocator*, RMLUI_RENDER_BACKEND_FIELD_SWAPCHAIN_BACKBUFFER_COUNT> m_backbuffers_allocators;
	Rml::Array<uint64_t, RMLUI_RENDER_BACKEND_FIELD_SWAPCHAIN_BACKBUFFER_COUNT> m_backbuffers_fence_values;
	Rml::Array<size_t, RMLUI_RENDER_BACKEND_FIELD_SWAPCHAIN_BACKBUFFER_COUNT> m_constant_buffer_count_per_frame;
	Rml::Array<size_t, RMLUI_RENDER_BACKEND_FIELD_SWAPCHAIN_BACKBUFFER_COUNT> m_vertex_and_index_buffer_count_per_frame;
	// per object (per draw)
	Rml::Array<Rml::Vector<ConstantBufferType>, 1> m_constantbuffers;
	Rml::Vector<GeometryHandleType*> m_pending_for_deletion_geometry;
	// todo: delete this if final implementation doesn't use this
	Rml::Vector<TextureHandleType*> m_pending_for_deletion_textures;
	// ConstantBufferType m_constantbuffer;

	// this represents user's data from initialization structure about multisampling features
	DXGI_SAMPLE_DESC m_desc_sample;
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_handle_shaders;
	BufferMemoryManager m_manager_buffer;
	TextureMemoryManager m_manager_texture;
	Rml::Matrix4f m_constant_buffer_data_transform;
	Rml::Matrix4f m_constant_buffer_data_projection;
	Rml::Matrix4f m_projection;
	RenderLayerStack m_manager_render_layer;
};

// forward declaration
namespace Backend {
class RmlRenderInitInfo;
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

#endif // RMLUI_PLATFORM_WIN32

#endif
