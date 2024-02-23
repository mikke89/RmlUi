if(NOT RMLUI_FONT_ENGINE_ENABLED)
	message(NOTICE "Building samples without any font engine selected: Some samples will be disabled.")
endif()
if(RMLUI_SAMPLES_BACKEND STREQUAL "auto")
    # include("${PROJECT_SOURCE_DIR}/CMake/BackendsAutoSelection.cmake")
    message(FATAL_ERROR "[auto backend not yet implemented] Automatic sample backend selection enabled. Chosen backend: ${RMLUI_SAMPLES_BACKEND}")
endif()

include("${PROJECT_SOURCE_DIR}/CMake/BackendsDependencies.cmake")
include("${PROJECT_SOURCE_DIR}/CMake/Backends.cmake")

# Negotiate Vulkan debugging for Vulkan-based backends
if(RMLUI_SAMPLES_BACKEND MATCHES "VK$")
    option(RMLUI_VK_DEBUG "Enable debugging mode for Vulkan renderer." OFF)
    mark_as_advanced(RMLUI_VK_DEBUG)
    if(RMLUI_VK_DEBUG)
        # Add compile definition for Vulkan debugging
        target_compile_definitions(rmlui_backend_${RMLUI_SAMPLES_BACKEND} INTERFACE "RMLUI_VK_DEBUG")
    endif()
endif()

include("${PROJECT_SOURCE_DIR}/CMake/SamplesShellDependencies.cmake")

set(RMLUI_SAMPLE_PREFIX "rmlui_sample_")

add_subdirectory("shell")
add_subdirectory("basic")

if(RMLUI_FONT_ENGINE_ENABLED)
	add_subdirectory("invaders")
	add_subdirectory("tutorial")
endif()
if(RMLUI_FONT_ENGINE_ENABLED AND RMLUI_LUA_BINDINGS)
	add_subdirectory("luainvaders")
endif()

install(DIRECTORY assets
	DESTINATION samples
)
