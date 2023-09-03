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

#[[
Function to print a message to the console indicating a library from a native platform SDK hasn't been found
Arguments:
    - library_name: Name of the library
]]
function(report_not_found_native_library library_name)
    # Set SDK notice string
    if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set(SDK_NOTICE  "In order to ensure it is found, install the Windows SDK and build RmlUi inside a Visual Studio Developer CLI environment.\n"
                        "More info: https://learn.microsoft.com/en-us/visualstudio/ide/reference/command-prompt-powershell"
        )
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        set(SDK_NOTICE  "In order to ensure it is found, install the macOS SDK.\n"
                        "More info: https://developer.apple.com/macos/"
        )
    endif()

    # Print notice
    message(NOTICE 
        "CMake failed to find the ${library_name} library. Depending on the compiler, underlying build system "
        "and environment setup, linkage of the RmlUi samples executables might fail."
        "\n${SDK_NOTICE}"
    )
endfunction()

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
