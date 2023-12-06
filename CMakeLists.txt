# Using CMake 3.10.2 as minimum to support all platforms of interest
# https://github.com/mikke89/RmlUi/issues/198#issuecomment-1246957062
cmake_minimum_required(VERSION "3.10.2")

# Define CMake project
project("RmlUi"
    VERSION "6.0"
    DESCRIPTION "C++ user interface package based on the HTML and CSS standards"
    LANGUAGES "C" "CXX"
)

# Disable compiler-specific extensions
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_EXTENSIONS OFF)

# Declare project-specific options
# "RMLUI_" prefix is included in order to take advantage of the fact that the
# CMake GUI can group variables based on their prefix to make more clear
# which options are specific to this project
set(RMLUI_FONT_INTERFACE "freetype" CACHE STRING "Font interface to build RmlUi with. If set to \"none\", RmlUi won't be built with any of the included font interfaces and some samples might be disabled.")
option(RMLUI_LOTTIE_PLUGIN "Enable plugin for Lottie animations. Requires the rlottie library." OFF)
option(RMLUI_SVG_PLUGIN "Enable plugin for SVG images. Requires the lunasvg library." OFF)
option(RMLUI_SAMPLES "Build samples of the library." OFF)
set(RMLUI_SAMPLES_BACKEND "GLFW_GL3" CACHE STRING "Backend to use when building the RmlUi samples. Choose one from ./CMake/Backends.cmake.")
option(BUILD_SHARED_LIBS "CMake standard option. Choose whether to build RmlUi and RmlUi's debugger as shared libraries." ON)

# Add lists of available options for those that support it
include("${PROJECT_SOURCE_DIR}/CMake/OptionsLists.cmake")

# Set hints for non-boolean custom CMake options
set_property(CACHE RMLUI_SAMPLES_BACKEND PROPERTY STRINGS ${RMLUI_SAMPLES_AVAILABLE_BACKENDS})
set_property(CACHE RMLUI_FONT_INTERFACE PROPERTY STRINGS ${RMLUI_AVAILABLE_FONT_INTERFACES})

# Check if the specified sample backend is available
list(FIND RMLUI_SAMPLES_AVAILABLE_BACKENDS ${RMLUI_SAMPLES_BACKEND} rmlui_backend_index)
if(rmlui_backend_index EQUAL "-1")
    # If the backend identifier isn't found in the list, return error
    message(FATAL_ERROR "The RmlUi backend ID specified in RMLUI_SAMPLES_BACKEND is not valid.")
endif()
unset(rmlui_backend_index)

# Check if the specified font interface is available
list(FIND RMLUI_AVAILABLE_FONT_INTERFACES ${RMLUI_FONT_INTERFACE} rmlui_fontinterface_index)
if(rmlui_fontinterface_index EQUAL "-1")
    # If the backend identifier isn't found in the list, return error
    message(FATAL_ERROR "The RmlUi font interface ID specified in RMLUI_FONT_INTERFACE is not valid.")
endif()
unset(rmlui_fontinterface_index)

# Add custom CMake modules path for external dependencies
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake/Modules")
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Dependencies")

# Set up external dependencies
include("CMake/Dependencies.cmake")

# Add CMake subdirectories
add_subdirectory("Source")

# Add samples to the build project if enabled
if(RMLUI_SAMPLES)
    add_subdirectory("Samples")
endif()
