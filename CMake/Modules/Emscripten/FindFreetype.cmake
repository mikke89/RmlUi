#[[
    Custom find module for Freetype for Emscripten compilation.

    Input variables:
        none
    Output variables:
        Freetype_FOUND
        FREETYPE_VERSION_STRING

    Resulting targets:
        Freetype::Freetype

    More info:
    https://emscripten.org/docs/porting/multimedia_and_graphics/OpenGL-support.html
#]]

include(FindPackageHandleStandardArgs)
include(FindPackageMessage)

# If no version was specified, set default
if(NOT DEFINED Freetype_FIND_VERSION)
    set(Freetype_FIND_VERSION "2")
endif()

# Check if requested Freetype version is valid
# Emscripten SDK only provides Freetype 2.x
if((Freetype_FIND_VERSION VERSION_LESS "2") OR (Freetype_FIND_VERSION VERSION_GREATER_EQUAL "3"))
    message(FATAL_ERROR "The requested Freetype version ${Freetype_FIND_VERSION} is invalid.")
endif()

# Emscripten includes Freetype support as part of it's SDK, meaning there's no need to find it
set(Freetype_FOUND TRUE)
add_library(freetype INTERFACE)
add_library(Freetype::Freetype ALIAS freetype)

# Set found Freetype version
# Version set based on latest Emscripten SDK at the time of writing this file
set(FREETYPE_VERSION_STRING "2.6")

# Enable compilation and linking against Freetype
target_compile_options(freetype INTERFACE "-sUSE_FREETYPE=1")
target_link_libraries(freetype INTERFACE "-sUSE_FREETYPE=1")

# Get final compiler and linker flags to print them
get_target_property(Freetype_COMPILE_FLAGS freetype "INTERFACE_COMPILE_OPTIONS")
get_target_property(Freetype_LINK_FLAGS freetype "INTERFACE_LINK_OPTIONS")

find_package_message(
    "Freetype"
    "Freetype ${FREETYPE_VERSION_STRING} has been found as part of the Emscripten SDK."
    "[${Freetype_COMPILE_FLAGS}][${Freetype_LINK_FLAGS}]"
)

# Clean scope
unset(Freetype_COMPILE_FLAGS)
unset(Freetype_LINK_FLAGS)