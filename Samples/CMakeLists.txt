# Check the font interface configuration
if(RMLUI_FONT_ENGINE STREQUAL "none")
    # If RmlUi is being built with no font interface, then some samples cannot be built
    message(NOTICE
        "Building samples has been enabled, but no font interface has been selected. "
        "Some samples might be disabled."
    )
endif()

# Handle automatic selection of the backend if chosen
if(RMLUI_SAMPLES_BACKEND STREQUAL "auto")
    # include("${PROJECT_SOURCE_DIR}/CMake/BackendsAutoSelection.cmake")

    message(STATUS "Automatic sample backend selection enabled. Chosen backend: ${RMLUI_SAMPLES_BACKEND}")
endif()

set(RMLUI_SAMPLES_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# Declare dependencies for sample backends
include("${PROJECT_SOURCE_DIR}/CMake/BackendsDependencies.cmake")

# Add information about the available backends specified in CMake/Backends.cmake
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

# Look for dependencies for the RmlUi samples shell
include("${PROJECT_SOURCE_DIR}/CMake/SamplesShellDependencies.cmake")

# Add shell library for the samples
add_subdirectory("shell")

# Add basic samples
add_subdirectory("basic")

install(DIRECTORY assets
	DESTINATION samples
)
