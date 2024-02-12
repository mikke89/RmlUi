#[[
    Set up of external dependencies required to build RmlUi itself

    All packages are configured as soft dependencies (i.e. not REQUIRED), so that a consuming project can declare them
    by other means, without an error being emitted here. For the same reason, instead of relying on variables like
    *_NOTOUND variables, we check directly for the existence of the target.
]]

include("${PROJECT_SOURCE_DIR}/CMake/Utils.cmake")

if(RMLUI_FONT_INTERFACE STREQUAL "freetype")
    find_package("Freetype")

    if(NOT TARGET Freetype::Freetype)
        report_not_found_dependency("Freetype" Freetype::Freetype)
    endif()

    if(DEFINED FREETYPE_VERSION_STRING)
        if(FREETYPE_VERSION_STRING VERSION_EQUAL "2.11.0" AND CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
            message(WARNING "Using Freetype 2.11.0 with MSVC can cause issues, please upgrade to Freetype 2.11.1 or newer.")
        endif()
    endif()
endif()

if(RMLUI_LOTTIE_PLUGIN)
    find_package("rlottie")

    if(NOT TARGET rlottie::rlottie)
        report_not_found_dependency("rlottie" rlottie::rlottie)
    endif()
endif()

if(RMLUI_SVG_PLUGIN)
    find_package("lunasvg")

    if(NOT TARGET lunasvg::lunasvg)
        report_not_found_dependency("lunasvg" lunasvg::lunasvg)
    endif()
endif()
