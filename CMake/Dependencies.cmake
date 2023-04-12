#[[
Function to print a message to the console indicating a dependency hasn't been found
Arguments:
    - friendly_name: Friendly name of the target
    - target_name: Name of the CMake target the project is supposed to link against
]]
function(report_not_found_dependency friendly_name target_name)
    message(FATAL_ERROR     
        "${friendly_name} has not been found by CMake."
        "\nIf you are consuming RmlUi as a subdirectory inside another CMake project, please ensure that "
        "${friendly_name} can be found by CMake or at least being linked using \"${target_name}\" as its "
        "target name. You can create an ALIAS target to offer an alternative name for a CMake target."
    )
endfunction()

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
        if((${FREETYPE_VERSION_STRING} VERSION_GREATER_EQUAL "2.11.0") AND (${CMAKE_CXX_COMPILER_ID} STREQUAL "MSVC"))
            message(WARNING "Using Freetype 2.11.0 or greater with MSVC can cause issues.")
        endif()
    else()
        if(${CMAKE_CXX_COMPILER_ID} STREQUAL "MSVC")
            message(WARNING "Using Freetype 2.11.0 or greater with MSVC can cause issues.")
        endif()
    endif()
endif()

# rlottie
if(RMLUI_ENABLE_LOTTIE_PLUGIN)
    # Declaring rlottie as a soft dependency so that it doesn't error out if the package
    # is declared by other means
    find_package("rlottie")

    # Instead of relying on the rlottie_NOTFOUND variable, we check directly for the target
    if(NOT TARGET rlottie::rlottie)
        report_not_found_dependency("rlottie" rlottie::rlottie)
    endif()
endif()
