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
set(RMLUI_FONT_INTERFACE "freetype" CACHE STRING "Font engine to build RmlUi with. If left empty, RmlUi won't be built with any of the included font interfaces.")
option(RMLUI_ENABLE_LOTTIE_PLUGIN "Enable plugin for Lottie animations. Requires the rlottie library." OFF)
option(RMLUI_ENABLE_SVG_PLUGIN "Enable plugin for SVG images. Requires the lunasvg library." OFF)
option(RMLUI_BUILD_SAMPLES "Build samples of the library." OFF)
set(RMLUI_SAMPLES_BACKEND "GLFW_GL3" CACHE STRING "Backend to use when building the RmlUi samples. Choose one from ./CMake/Backends.cmake.")
option(BUILD_SHARED_LIBS "CMake standard option. Choose whether to build RmlUi and RmlUi's debugger as shared libraries." OFF)
option(RMLUI_USING_VCPKG "If true, the CMake project will adapt its behavior to accept third-party dependencies from vcpkg." OFF)

# Add custom CMake modules path for external dependencies
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake/Modules")
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Dependencies")

# Set up external dependencies
include("CMake/Dependencies.cmake")

# Add CMake subdirectories
add_subdirectory("Source")

# Add samples to the build project if enabled
if(RMLUI_BUILD_SAMPLES)
    add_subdirectory("Samples")
endif()
