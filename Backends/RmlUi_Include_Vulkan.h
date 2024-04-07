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
