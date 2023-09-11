# Check the font interface configuration
if(RMLUI_FONT_INTERFACE STREQUAL "none")
    # If RmlUi is being built with no font interface, then some samples cannot be built
    message(NOTICE 
        "Building samples has been enabled, but no font interface has been selected. "
        "Some samples might be disabled."
    )
endif()

# Declare dependencies for sample backend
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

# Change the runtime output directory of all target declared in this scope
# This is needed so that the shell library can find the samples asset files
# when running them from a CMake binary directory
# The set() call is guarded against pre-definitions in order to respect consumer choice
if(NOT DEFINED CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endif()

# Add shell library for the samples
add_subdirectory("shell")

# Add samples's subdirectories
add_subdirectory("basic")
