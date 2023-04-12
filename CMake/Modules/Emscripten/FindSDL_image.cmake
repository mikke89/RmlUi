#[[
    Custom find module for SDL_image for Emscripten compilation.

    Input variables:
        none
    Output variables:
        SDL_image_FOUND
        SDL_image_VERSION

    Resulting targets:
        SDL::Image

    More info:
    https://emscripten.org/docs/porting/multimedia_and_graphics/OpenGL-support.html
#]]

include(FindPackageHandleStandardArgs)
include(FindPackageMessage)

# If no version was specified, set default
if(NOT DEFINED SDL_image_FIND_VERSION)
    set(SDL_image_FIND_VERSION "1")
endif()

# Check if requested SDL version is valid
if((SDL_image_FIND_VERSION VERSION_LESS "1") OR (SDL_image_FIND_VERSION VERSION_GREATER_EQUAL "3"))
    message(FATAL_ERROR "The requested SDL_image version ${SDL_image_FIND_VERSION} is invalid.")
endif()

message(WARNING 
    "SDL_image with Emscripten can only be used if SDL usage is also enabled. "
    "The version number of both libraries must match as well."
)

# Emscripten includes SDL support as part of it's SDK, meaning there's no need to find it
set(SDL_image_FOUND TRUE)
add_library(sdl_image INTERFACE)
add_library(SDL::Image ALIAS sdl_image)

# Set found SDL_image version based on latest Emscripten SDK at the time of writing this file
if((SDL_image_FIND_VERSION VERSION_GREATER_EQUAL "1") AND (SDL_image_FIND_VERSION VERSION_LESS "2"))
    # Version set based on the information on $EMSDK/upstream/emscripten/src/settings.js
    set(SDL_image_VERSION "1.3")

    # Version number to pass to compiler
    set(SDL_image_EMSCRIPTEN_COMPILER_SELECTED_VERSION "1")

elseif((SDL_image_FIND_VERSION VERSION_GREATER_EQUAL "2") AND (SDL_image_FIND_VERSION VERSION_LESS "3"))
    # Version set based on latest Emscripten SDK at the time of writing this file
    set(SDL_image_VERSION "2.24.2")

    # Version number to pass to compiler
    set(SDL_image_EMSCRIPTEN_COMPILER_SELECTED_VERSION "2")
endif()

# Enable compilation and linking against SDL
target_compile_options(sdl_image INTERFACE "-sUSE_SDL_IMAGE=${SDL_image_EMSCRIPTEN_COMPILER_SELECTED_VERSION}")
target_link_libraries(sdl_image INTERFACE "-sUSE_SDL_IMAGE=${SDL_image_EMSCRIPTEN_COMPILER_SELECTED_VERSION}")

# Get final compiler and linker flags to print them
get_target_property(SDL_image_COMPILE_FLAGS sdl_image "INTERFACE_COMPILE_OPTIONS")
get_target_property(SDL_image_LINK_FLAGS sdl_image "INTERFACE_LINK_OPTIONS")

find_package_message(
    "SDL_image"
    "SDL_image ${SDL_image_VERSION} has been found as part of the Emscripten SDK."
    "[${SDL_image_COMPILE_FLAGS}][${SDL_image_LINK_FLAGS}]"
)

# Clean scope
unset(SDL_image_COMPILE_FLAGS)
unset(SDL_image_COMPILE_FLAGS)
unset(SDL_image_EMSCRIPTEN_COMPILER_SELECTED_VERSION)
