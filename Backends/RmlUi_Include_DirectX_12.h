#ifndef RMLUI_BACKENDS_INCLUDE_DIRECTX_12_H
#define RMLUI_BACKENDS_INCLUDE_DIRECTX_12_H

#if defined _MSC_VER
	#pragma warning(push, 0)
#elif defined __clang__
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wall"
	#pragma clang diagnostic ignored "-Wextra"
	#pragma clang diagnostic ignored "-Wnullability-extension"
	#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
	#pragma clang diagnostic ignored "-Wnullability-completeness"
#elif defined __GNUC__
	#pragma GCC system_header
#endif

#include "RmlUi_DirectX/D3D12MemAlloc.h"
#include "RmlUi_DirectX/offsetAllocator.hpp"
#include <bitset>
#include <chrono>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxgi1_6.h>

namespace Rml  
{
inline constexpr const char* DXGIFormatToString(DXGI_FORMAT format)
{
	switch (format)
	{
	case DXGI_FORMAT::DXGI_FORMAT_UNKNOWN: return "DXGI_FORMAT_UNKNOWN";
	case DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_TYPELESS: return "DXGI_FORMAT_R32G32B32A32_TYPELESS";
	case DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT: return "DXGI_FORMAT_R32G32B32A32_FLOAT";
	case DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_UINT: return "DXGI_FORMAT_R32G32B32A32_UINT";
	case DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_SINT: return "DXGI_FORMAT_R32G32B32A32_SINT";
	case DXGI_FORMAT::DXGI_FORMAT_R32G32B32_TYPELESS: return "DXGI_FORMAT_R32G32B32_TYPELESS";
	case DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT: return "DXGI_FORMAT_R32G32B32_FLOAT";
	case DXGI_FORMAT::DXGI_FORMAT_R32G32B32_UINT: return "DXGI_FORMAT_R32G32B32_UINT";
	case DXGI_FORMAT::DXGI_FORMAT_R32G32B32_SINT: return "DXGI_FORMAT_R32G32B32_SINT";
	case DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_TYPELESS: return "DXGI_FORMAT_R16G16B16A16_TYPELESS";
	case DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT: return "DXGI_FORMAT_R16G16B16A16_FLOAT";
	case DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_UNORM: return "DXGI_FORMAT_R16G16B16A16_UNORM";
	case DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_UINT: return "DXGI_FORMAT_R16G16B16A16_UINT";
	case DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_SNORM: return "DXGI_FORMAT_R16G16B16A16_SNORM";
	case DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_SINT: return "DXGI_FORMAT_R16G16B16A16_SINT";
	case DXGI_FORMAT::DXGI_FORMAT_R32G32_TYPELESS: return "DXGI_FORMAT_R32G32_TYPELESS";
	case DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT: return "DXGI_FORMAT_R32G32_FLOAT";
	case DXGI_FORMAT::DXGI_FORMAT_R32G32_UINT: return "DXGI_FORMAT_R32G32_UINT";
	case DXGI_FORMAT::DXGI_FORMAT_R32G32_SINT: return "DXGI_FORMAT_R32G32_SINT";
	case DXGI_FORMAT::DXGI_FORMAT_R32G8X24_TYPELESS: return "DXGI_FORMAT_R32G8X24_TYPELESS";
	case DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT: return "DXGI_FORMAT_D32_FLOAT_S8X24_UINT";
	case DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS: return "DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS";
	case DXGI_FORMAT::DXGI_FORMAT_X32_TYPELESS_G8X24_UINT: return "DXGI_FORMAT_X32_TYPELESS_G8X24_UINT";
	case DXGI_FORMAT::DXGI_FORMAT_R10G10B10A2_TYPELESS: return "DXGI_FORMAT_R10G10B10A2_TYPELESS";
	case DXGI_FORMAT::DXGI_FORMAT_R10G10B10A2_UNORM: return "DXGI_FORMAT_R10G10B10A2_UNORM";
	case DXGI_FORMAT::DXGI_FORMAT_R10G10B10A2_UINT: return "DXGI_FORMAT_R10G10B10A2_UINT";
	case DXGI_FORMAT::DXGI_FORMAT_R11G11B10_FLOAT: return "DXGI_FORMAT_R11G11B10_FLOAT";
	case DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_TYPELESS: return "DXGI_FORMAT_R8G8B8A8_TYPELESS";
	case DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM: return "DXGI_FORMAT_R8G8B8A8_UNORM";
	case DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return "DXGI_FORMAT_R8G8B8A8_UNORM_SRGB";
	case DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UINT: return "DXGI_FORMAT_R8G8B8A8_UINT";
	case DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_SNORM: return "DXGI_FORMAT_R8G8B8A8_SNORM";
	case DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_SINT: return "DXGI_FORMAT_R8G8B8A8_SINT";
	case DXGI_FORMAT::DXGI_FORMAT_R16G16_TYPELESS: return "DXGI_FORMAT_R16G16_TYPELESS";
	case DXGI_FORMAT::DXGI_FORMAT_R16G16_FLOAT: return "DXGI_FORMAT_R16G16_FLOAT";
	case DXGI_FORMAT::DXGI_FORMAT_R16G16_UNORM: return "DXGI_FORMAT_R16G16_UNORM";
	case DXGI_FORMAT::DXGI_FORMAT_R16G16_UINT: return "DXGI_FORMAT_R16G16_UINT";
	case DXGI_FORMAT::DXGI_FORMAT_R16G16_SNORM: return "DXGI_FORMAT_R16G16_SNORM";
	case DXGI_FORMAT::DXGI_FORMAT_R16G16_SINT: return "DXGI_FORMAT_R16G16_SINT";
	case DXGI_FORMAT::DXGI_FORMAT_R32_TYPELESS: return "DXGI_FORMAT_R32_TYPELESS";
	case DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT: return "DXGI_FORMAT_D32_FLOAT";
	case DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT: return "DXGI_FORMAT_R32_FLOAT";
	case DXGI_FORMAT::DXGI_FORMAT_R32_UINT: return "DXGI_FORMAT_R32_UINT";
	case DXGI_FORMAT::DXGI_FORMAT_R32_SINT: return "DXGI_FORMAT_R32_SINT";
	case DXGI_FORMAT::DXGI_FORMAT_R24G8_TYPELESS: return "DXGI_FORMAT_R24G8_TYPELESS";
	case DXGI_FORMAT::DXGI_FORMAT_D24_UNORM_S8_UINT: return "DXGI_FORMAT_D24_UNORM_S8_UINT";
	case DXGI_FORMAT::DXGI_FORMAT_R24_UNORM_X8_TYPELESS: return "DXGI_FORMAT_R24_UNORM_X8_TYPELESS";
	case DXGI_FORMAT::DXGI_FORMAT_X24_TYPELESS_G8_UINT: return "DXGI_FORMAT_X24_TYPELESS_G8_UINT";
	case DXGI_FORMAT::DXGI_FORMAT_R8G8_TYPELESS: return "DXGI_FORMAT_R8G8_TYPELESS";
	case DXGI_FORMAT::DXGI_FORMAT_R8G8_UNORM: return "DXGI_FORMAT_R8G8_UNORM";
	case DXGI_FORMAT::DXGI_FORMAT_R8G8_UINT: return "DXGI_FORMAT_R8G8_UINT";
	case DXGI_FORMAT::DXGI_FORMAT_R8G8_SNORM: return "DXGI_FORMAT_R8G8_SNORM";
	case DXGI_FORMAT::DXGI_FORMAT_R8G8_SINT: return "DXGI_FORMAT_R8G8_SINT";
	case DXGI_FORMAT::DXGI_FORMAT_R16_TYPELESS: return "DXGI_FORMAT_R16_TYPELESS";
	case DXGI_FORMAT::DXGI_FORMAT_R16_FLOAT: return "DXGI_FORMAT_R16_FLOAT";
	case DXGI_FORMAT::DXGI_FORMAT_D16_UNORM: return "DXGI_FORMAT_D16_UNORM";
	case DXGI_FORMAT::DXGI_FORMAT_R16_UNORM: return "DXGI_FORMAT_R16_UNORM";
	case DXGI_FORMAT::DXGI_FORMAT_R16_UINT: return "DXGI_FORMAT_R16_UINT";
	case DXGI_FORMAT::DXGI_FORMAT_R16_SNORM: return "DXGI_FORMAT_R16_SNORM";
	case DXGI_FORMAT::DXGI_FORMAT_R16_SINT: return "DXGI_FORMAT_R16_SINT";
	case DXGI_FORMAT::DXGI_FORMAT_R8_TYPELESS: return "DXGI_FORMAT_R8_TYPELESS";
	case DXGI_FORMAT::DXGI_FORMAT_R8_UNORM: return "DXGI_FORMAT_R8_UNORM";
	case DXGI_FORMAT::DXGI_FORMAT_R8_UINT: return "DXGI_FORMAT_R8_UINT";
	case DXGI_FORMAT::DXGI_FORMAT_R8_SNORM: return "DXGI_FORMAT_R8_SNORM";
	case DXGI_FORMAT::DXGI_FORMAT_R8_SINT: return "DXGI_FORMAT_R8_SINT";
	case DXGI_FORMAT::DXGI_FORMAT_A8_UNORM: return "DXGI_FORMAT_A8_UNORM";
	case DXGI_FORMAT::DXGI_FORMAT_R1_UNORM: return "DXGI_FORMAT_R1_UNORM";
	case DXGI_FORMAT::DXGI_FORMAT_R9G9B9E5_SHAREDEXP: return "DXGI_FORMAT_R9G9B9E5_SHAREDEXP";
	case DXGI_FORMAT::DXGI_FORMAT_R8G8_B8G8_UNORM: return "DXGI_FORMAT_R8G8_B8G8_UNORM";
	case DXGI_FORMAT::DXGI_FORMAT_G8R8_G8B8_UNORM: return "DXGI_FORMAT_G8R8_G8B8_UNORM";
	case DXGI_FORMAT::DXGI_FORMAT_BC1_TYPELESS: return "DXGI_FORMAT_BC1_TYPELESS";
	case DXGI_FORMAT::DXGI_FORMAT_BC1_UNORM: return "DXGI_FORMAT_BC1_UNORM";
	case DXGI_FORMAT::DXGI_FORMAT_BC1_UNORM_SRGB: return "DXGI_FORMAT_BC1_UNORM_SRGB";
	case DXGI_FORMAT::DXGI_FORMAT_BC2_TYPELESS: return "DXGI_FORMAT_BC2_TYPELESS";
	case DXGI_FORMAT::DXGI_FORMAT_BC2_UNORM: return "DXGI_FORMAT_BC2_UNORM";
	case DXGI_FORMAT::DXGI_FORMAT_BC2_UNORM_SRGB: return "DXGI_FORMAT_BC2_UNORM_SRGB";
	case DXGI_FORMAT::DXGI_FORMAT_BC3_TYPELESS: return "DXGI_FORMAT_BC3_TYPELESS";
	case DXGI_FORMAT::DXGI_FORMAT_BC3_UNORM: return "DXGI_FORMAT_BC3_UNORM";
	case DXGI_FORMAT::DXGI_FORMAT_BC3_UNORM_SRGB: return "DXGI_FORMAT_BC3_UNORM_SRGB";
	case DXGI_FORMAT::DXGI_FORMAT_BC4_TYPELESS: return "DXGI_FORMAT_BC4_TYPELESS";
	case DXGI_FORMAT::DXGI_FORMAT_BC4_UNORM: return "DXGI_FORMAT_BC4_UNORM";
	case DXGI_FORMAT::DXGI_FORMAT_BC4_SNORM: return "DXGI_FORMAT_BC4_SNORM";
	case DXGI_FORMAT::DXGI_FORMAT_BC5_TYPELESS: return "DXGI_FORMAT_BC5_TYPELESS";
	case DXGI_FORMAT::DXGI_FORMAT_BC5_UNORM: return "DXGI_FORMAT_BC5_UNORM";
	case DXGI_FORMAT::DXGI_FORMAT_BC5_SNORM: return "DXGI_FORMAT_BC5_SNORM";
	case DXGI_FORMAT::DXGI_FORMAT_B5G6R5_UNORM: return "DXGI_FORMAT_B5G6R5_UNORM";
	case DXGI_FORMAT::DXGI_FORMAT_B5G5R5A1_UNORM: return "DXGI_FORMAT_B5G5R5A1_UNORM";
	case DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM: return "DXGI_FORMAT_B8G8R8A8_UNORM";
	case DXGI_FORMAT::DXGI_FORMAT_B8G8R8X8_UNORM: return "DXGI_FORMAT_B8G8R8X8_UNORM";
	case DXGI_FORMAT::DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM: return "DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM";
	case DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_TYPELESS: return "DXGI_FORMAT_B8G8R8A8_TYPELESS";
	case DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM_SRGB: return "DXGI_FORMAT_B8G8R8A8_UNORM_SRGB";
	case DXGI_FORMAT::DXGI_FORMAT_B8G8R8X8_TYPELESS: return "DXGI_FORMAT_B8G8R8X8_TYPELESS";
	case DXGI_FORMAT::DXGI_FORMAT_B8G8R8X8_UNORM_SRGB: return "DXGI_FORMAT_B8G8R8X8_UNORM_SRGB";
	case DXGI_FORMAT::DXGI_FORMAT_BC6H_TYPELESS: return "DXGI_FORMAT_BC6H_TYPELESS";
	case DXGI_FORMAT::DXGI_FORMAT_BC6H_UF16: return "DXGI_FORMAT_BC6H_UF16";
	case DXGI_FORMAT::DXGI_FORMAT_BC6H_SF16: return "DXGI_FORMAT_BC6H_SF16";
	case DXGI_FORMAT::DXGI_FORMAT_BC7_TYPELESS: return "DXGI_FORMAT_BC7_TYPELESS";
	case DXGI_FORMAT::DXGI_FORMAT_BC7_UNORM: return "DXGI_FORMAT_BC7_UNORM";
	case DXGI_FORMAT::DXGI_FORMAT_BC7_UNORM_SRGB: return "DXGI_FORMAT_BC7_UNORM_SRGB";
	case DXGI_FORMAT::DXGI_FORMAT_AYUV: return "DXGI_FORMAT_AYUV";
	case DXGI_FORMAT::DXGI_FORMAT_Y410: return "DXGI_FORMAT_Y410";
	case DXGI_FORMAT::DXGI_FORMAT_Y416: return "DXGI_FORMAT_Y416";
	case DXGI_FORMAT::DXGI_FORMAT_NV12: return "DXGI_FORMAT_NV12";
	case DXGI_FORMAT::DXGI_FORMAT_P010: return "DXGI_FORMAT_P010";
	case DXGI_FORMAT::DXGI_FORMAT_P016: return "DXGI_FORMAT_P016";
	case DXGI_FORMAT::DXGI_FORMAT_420_OPAQUE: return "DXGI_FORMAT_420_OPAQUE";
	case DXGI_FORMAT::DXGI_FORMAT_YUY2: return "DXGI_FORMAT_YUY2";
	case DXGI_FORMAT::DXGI_FORMAT_Y210: return "DXGI_FORMAT_Y210";
	case DXGI_FORMAT::DXGI_FORMAT_Y216: return "DXGI_FORMAT_Y216";
	case DXGI_FORMAT::DXGI_FORMAT_NV11: return "DXGI_FORMAT_NV11";
	case DXGI_FORMAT::DXGI_FORMAT_AI44: return "DXGI_FORMAT_AI44";
	case DXGI_FORMAT::DXGI_FORMAT_IA44: return "DXGI_FORMAT_IA44";
	case DXGI_FORMAT::DXGI_FORMAT_P8: return "DXGI_FORMAT_P8";
	case DXGI_FORMAT::DXGI_FORMAT_A8P8: return "DXGI_FORMAT_A8P8";
	case DXGI_FORMAT::DXGI_FORMAT_B4G4R4A4_UNORM: return "DXGI_FORMAT_B4G4R4A4_UNORM";
	case DXGI_FORMAT::DXGI_FORMAT_P208: return "DXGI_FORMAT_P208";
	case DXGI_FORMAT::DXGI_FORMAT_V208: return "DXGI_FORMAT_V208";
	case DXGI_FORMAT::DXGI_FORMAT_V408: return "DXGI_FORMAT_V408";
	case DXGI_FORMAT::DXGI_FORMAT_SAMPLER_FEEDBACK_MIN_MIP_OPAQUE: return "DXGI_FORMAT_SAMPLER_FEEDBACK_MIN_MIP_OPAQUE";
	case DXGI_FORMAT::DXGI_FORMAT_SAMPLER_FEEDBACK_MIP_REGION_USED_OPAQUE: return "DXGI_FORMAT_SAMPLER_FEEDBACK_MIP_REGION_USED_OPAQUE";
	case DXGI_FORMAT::DXGI_FORMAT_A4B4G4R4_UNORM: return "DXGI_FORMAT_A4B4G4R4_UNORM";
	case DXGI_FORMAT::DXGI_FORMAT_FORCE_UINT: return "DXGI_FORMAT_FORCE_UINT";
	default: return "UNKNOWN_DXGI_FORMAT";
	}
}
}

#ifdef RMLUI_DEBUG
	#define RMLUI_DX_ASSERTMSG(statement, msg) RMLUI_ASSERTMSG(SUCCEEDED(statement), msg)

	// Uncomment the following line to enable additional DirectX debugging.
	#define RMLUI_DX_DEBUG
#else
	#define RMLUI_DX_ASSERTMSG(statement, msg) static_cast<void>(statement)
#endif

#if defined _MSC_VER
	#pragma warning(pop)
#elif defined __clang__
	#pragma clang diagnostic pop
#endif

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
	#define RMLUI_RENDER_BACKEND_FIELD_VIDEOMEMORY_FOR_BUFFER_ALLOCATION 1048576 * 2 // (2 * 1024 * 1024 = bytes or 2 Megabytes)
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
	// for getting total size of constant buffers you should multiply kPreAllocatedConstantBuffers * kSwapchainBackBufferCount e.g. 512 *
	// RMLUI_RENDER_BACKEND_FIELD_SWAPCHAIN_BACKBUFFER_COUNT = 1536 (if RMLUI_RENDER_BACKEND_FIELD_SWAPCHAIN_BACKBUFFER_COUNT=3) but keep in mind that
	// allocation per constant buffer is 512 bytes (due to fact of the max presented CB in shaders) so 1536 * 512 will be 786'432 bytes and knowing
	// that default buffer budget is just 2Mb (2'097'152) you will have 1'310'720 bytes (~37% of budget took by CBs) but some information will go to
	// vertices and some information will go to indices and then you will have some free space (maybe small depends on your input) for handling other
	// constant buffers that weren't sufficient for your current frame aka reallocation happens at some point it is fine just because the memory
	// management of this backend handles a reallocation situations and you will just get a new allocated buffer and all stuff goes to that buffer but
	// I give you this reasoning for being a developer that cares about optimizations and cares about pipeline development for end user and for that
	// reason I gave you understanding that generally constant buffers even 512 bytes but uses so much in frame will take a big chunk from your VB/IB
	// budget just because GAPI needs to handle many frames from swapchain, just because we expect indeed hard scene for rendering, but for average
	// mid or low pages that's enough for sure (in terms of preallocated memory for VB,IB,CB)
	#define RMLUI_RENDER_BACKEND_FIELD_PREALLOCATED_CONSTANTBUFFERS 512
#endif

#ifdef RMLUI_RENDER_BACKEND_OVERRIDE_FIELD_MSAA_SAMPLE_COUNT
	#define RMLUI_RENDER_BACKEND_FIELD_MSAA_SAMPLE_COUNT RMLUI_RENDER_BACKEND_OVERRIDE_FIELD_MSAA_SAMPLE_COUNT
#else
	#define RMLUI_RENDER_BACKEND_FIELD_MSAA_SAMPLE_COUNT 2
#endif

#ifdef RMLUI_RENDER_BACKEND_OVERRIDE_FIELD_DESCRIPTOR_HEAP_RTV
	#define RMLUI_RENDER_BACKEND_FIELD_DESCRIPTOR_HEAP_RTV RMLUI_RENDER_BACKEND_OVERRIDE_FIELD_DESCRIPTOR_HEAP_RTV
#else
	#define RMLUI_RENDER_BACKEND_FIELD_DESCRIPTOR_HEAP_RTV 12
#endif

#ifdef RMLUI_RENDER_BACKEND_OVERRIDE_FIELD_DESCRIPTOR_HEAP_DSV
	#define RMLUI_RENDER_BACKEND_FIELD_DESCRIPTOR_HEAP_DSV RMLUI_RENDER_BACKEND_OVERRIDE_FIELD_DESCRIPTOR_HEAP_DSV
#else
	#define RMLUI_RENDER_BACKEND_FIELD_DESCRIPTOR_HEAP_DSV 8
#endif

// specifes general (for all depth stencil textures that might be allocated by backend) depth value on clear operation (::ClearDepthStencilView)
#ifdef RMLUI_RENDER_BACKEND_OVERRIDE_FIELD_CLEAR_VALUE_DEPTHSTENCIL_DEPTH_VALUE
	#define RMLUI_RENDER_BACKEND_FIELD_CLEAR_VALUE_DEPTHSTENCIL_DEPTH_VALUE RMLUI_RENDER_BACKEND_OVERRIDE_FIELD_DESCRIPTOR_HEAP_DSV
#else
	#define RMLUI_RENDER_BACKEND_FIELD_CLEAR_VALUE_DEPTHSTENCIL_DEPTH_VALUE 1.0f
#endif

// specifies general (for all depth stencil textures that might be allocated by backend) stencil value on clear operation
// (::ClearDepthStencilView)
#ifdef RMLUI_RENDER_BACKEND_OVERRIDE_FIELD_CLEAR_VALUE_DEPTHSTENCIL_STENCIL_VALUE
	#define RMLUI_RENDER_BACKEND_FIELD_CLEAR_VALUE_DEPTHSTENCIL_STENCIL_VALUE \
		RMLUI_RENDER_BACKEND_OVERRIDE_FIELD_CLEAR_VALUE_DEPTHSTENCIL_STENCIL_VALUE
#else
	#define RMLUI_RENDER_BACKEND_FIELD_CLEAR_VALUE_DEPTHSTENCIL_STENCIL_VALUE 0
#endif

#ifdef RMLUI_RENDER_BACKEND_OVERRIDE_FIELD_CLEAR_VALUE_RENDERTARGET_COLOR_VALUE
	#define RMLUI_RENDER_BACKEND_FIELD_CLEAR_VALUE_RENDERTARGET_COLOR_VAlUE RMLUI_RENDER_BACKEND_OVERRIDE_FIELD_CLEAR_VALUE_RENDERTARGET_COLOR_VALUE
#else
	#define RMLUI_RENDER_BACKEND_FIELD_CLEAR_VALUE_RENDERTARGET_COLOR_VAlUE 0.0f, 0.0f, 0.0f, 1.0f
#endif

#ifdef RMLUI_RENDER_BACKEND_OVERRIDE_FIELD_STAGING_BUFFER_SIZE
	#define RMLUI_RENDER_BACKEND_FIELD_STAGING_BUFFER_SIZE RMLUI_RENDER_BACKEND_OVERRIDE_FIELD_STAGING_BUFFER_SIZE
#else
	#define RMLUI_RENDER_BACKEND_FIELD_STAGING_BUFFER_SIZE 1024 * 1024 * 8
#endif

#ifdef RMLUI_RENDER_BACKEND_OVERRIDE_FIELD_STAGING_BUFFER_CACHE_ENABLED
	#define RMLUI_RENDER_BACKEND_FIELD_STAGING_BUFFER_CACHE_ENABLED RMLUI_RENDER_BACKEND_OVERRIDE_FIELD_STAGING_BUFFER_CACHE_ENABLED
#else
	#define RMLUI_RENDER_BACKEND_FIELD_STAGING_BUFFER_CACHE_ENABLED 1
#endif

#endif