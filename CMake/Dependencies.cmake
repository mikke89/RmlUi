#[[
    Set up of external dependencies required to build RmlUi itself
]]

include("${PROJECT_SOURCE_DIR}/CMake/Utils.cmake")

# Freetype
if(RMLUI_FONT_INTERFACE STREQUAL "freetype")
    # Declaring Freetype as a soft dependency so that it doesn't error out if the package
    # is declared by other means
    find_package("Freetype")

    # Instead of relying on the Freetype_NOTFOUND variable, we check directly for the target
    if(NOT TARGET Freetype::Freetype)
        report_not_found_dependency("Freetype" Freetype::Freetype)
    endif()

    # Warn about problematic versions of the library with MSVC
    if(DEFINED FREETYPE_VERSION_STRING)
        if((FREETYPE_VERSION_STRING VERSION_EQUAL "2.11.0") AND (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC"))
            message(WARNING "Using Freetype 2.11.0 with MSVC can cause issues. This issue is solved in Freetype 2.11.1.")
        endif()
    else()
        if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
            message(WARNING "Using Freetype 2.11.0 or greater with MSVC can cause issues.")
        endif()
    endif()
endif()

# rlottie
if(RMLUI_LOTTIE_PLUGIN)
    # Declaring rlottie as a soft dependency so that it doesn't error out if the package
    # is declared by other means
    find_package("rlottie")

    # Instead of relying on the rlottie_NOTFOUND variable, we check directly for the target
    if(NOT TARGET rlottie::rlottie)
        report_not_found_dependency("rlottie" rlottie::rlottie)
    endif()
endif()
