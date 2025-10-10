#ifndef RMLUI_BACKENDS_INCLUDE_VULKAN_H
	#define RMLUI_BACKENDS_INCLUDE_VULKAN_H

	#if defined RMLUI_PLATFORM_UNIX
		#define VK_USE_PLATFORM_XCB_KHR 1
	#endif
	#define VMA_STATIC_VULKAN_FUNCTIONS 0
	#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#endif

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

#include "RmlUi_Vulkan/vulkan.h"
// Always include "vulkan.h" first, this comment prevents clang-format from reordering the includes.
#include "RmlUi_Vulkan/vk_mem_alloc.h"

#if defined _MSC_VER
	#pragma warning(pop)
#elif defined __clang__
	#pragma clang diagnostic pop
#endif

#ifdef RMLUI_RENDER_BACKEND_OVERRIDE_FIELD_MSAA_SAMPLE_COUNT
	#define RMLUI_RENDER_BACKEND_FIELD_MSAA_SAMPLE_COUNT RMLUI_RENDER_BACKEND_OVERRIDE_FIELD_MSAA_SAMPLE_COUNT
#else
	#define RMLUI_RENDER_BACKEND_FIELD_MSAA_SAMPLE_COUNT 2
#endif

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