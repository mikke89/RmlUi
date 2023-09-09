#[[
    Set up of external dependencies required by the shell utility library used by the samples
]]

include("${PROJECT_SOURCE_DIR}/CMake/Utils.cmake")

# RMLUI_CMAKE_MINIMUM_VERSION_RAISE_NOTICE:
# CMake >= 3.18 introduces the REQUIRED option for find_library() calls.
# Guaranteeing the presence of the platform SDK by making these calls to find_library()
# REQUIRED should be investigated.
# More info: https://cmake.org/cmake/help/latest/command/find_library.html

# Link against required libraries from Windows
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    # Required to use the functions from the shlwapi.h header
    find_library(Shlwapi NAMES "Shlwapi" "Shlwapi.lib" "Shlwapi.dll")
    if(NOT Shlwapi)
        report_not_found_native_library("Shlwapi")

        # Ignore the fact that the Shlwapi wasn't found and try to link against it anyway
        set(Shlwapi "Shlwapi")
    endif()

    # Set up wrapper target
    add_library(Windows::Shell::LightweightUtility INTERFACE IMPORTED)
    target_link_libraries(Windows::Shell::LightweightUtility INTERFACE ${Shlwapi})

# Link against required libraries from macOS
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    # Required to use the functions from the Cocoa framework
    find_library(Cocoa NAMES "Cocoa" "Cocoa.framework")
    if(NOT Cocoa)
        report_not_found_native_library("Cocoa")

        # Ignore the fact that the Cocoa wasn't found and try to link against it anyway
        set(Cocoa "Cocoa")
    endif()

    # Set up wrapper target
    add_library(macOS::Cocoa INTERFACE IMPORTED)
    target_link_libraries(macOS::Cocoa INTERFACE ${Cocoa})
endif()
