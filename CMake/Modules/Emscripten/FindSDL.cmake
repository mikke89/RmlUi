#[[
    Custom find module for SDL for Emscripten compilation.

    Input variables:
        none
    Output variables:
        SDL_FOUND
        SDL_VERSION

    Resulting targets:
        SDL::SDL

    More info:
    https://emscripten.org/docs/porting/multimedia_and_graphics/OpenGL-support.html
#]]

include(FindPackageHandleStandardArgs)
include(FindPackageMessage)

# If no version was specified, set default
if(NOT DEFINED SDL_FIND_VERSION)
    set(SDL_FIND_VERSION "1")
endif()

# Check if requested SDL version is valid
if((SDL_FIND_VERSION VERSION_LESS "1") OR (SDL_FIND_VERSION VERSION_GREATER_EQUAL "3"))
    message(FATAL_ERROR "The requested SDL version ${SDL_FIND_VERSION} is invalid.")
endif()

# Emscripten includes SDL support as part of it's SDK, meaning there's no need to find it
set(SDL_FOUND TRUE)
add_library(SDL::SDL INTERFACE IMPORTED)

# Set found SDL version based on latest Emscripten SDK at the time of writing this file
if((SDL_FIND_VERSION VERSION_GREATER_EQUAL "1") AND (SDL_FIND_VERSION VERSION_LESS "2"))
    # Version set based on the information on $EMSDK/upstream/emscripten/src/settings.js
    set(SDL_VERSION "1.3")

    # Version number to pass to compiler
    set(SDL_EMSCRIPTEN_COMPILER_SELECTED_VERSION "1")

elseif((SDL_FIND_VERSION VERSION_GREATER_EQUAL "2") AND (SDL_FIND_VERSION VERSION_LESS "3"))
    # Version set based on latest Emscripten SDK at the time of writing this file
    set(SDL_VERSION "2.24.2")

    # Version number to pass to compiler
    set(SDL_EMSCRIPTEN_COMPILER_SELECTED_VERSION "2")
endif()

# Enable compilation and linking against SDL
target_compile_options(SDL::SDL INTERFACE "-sUSE_SDL=${SDL_EMSCRIPTEN_COMPILER_SELECTED_VERSION}")
target_link_libraries(SDL::SDL INTERFACE "-sUSE_SDL=${SDL_EMSCRIPTEN_COMPILER_SELECTED_VERSION}")

# Get final compiler and linker flags to print them
get_target_property(SDL_COMPILE_FLAGS SDL::SDL "INTERFACE_COMPILE_OPTIONS")
get_target_property(SDL_LINK_FLAGS SDL::SDL "INTERFACE_LINK_OPTIONS")

find_package_message(
    "SDL"
    "SDL ${SDL_VERSION} has been found as part of the Emscripten SDK."
    "[${SDL_COMPILE_FLAGS}][${SDL_LINK_FLAGS}]"
)

# Clean scope
unset(SDL_COMPILE_FLAGS)
unset(SDL_COMPILE_FLAGS)
unset(SDL_EMSCRIPTEN_COMPILER_SELECTED_VERSION)
