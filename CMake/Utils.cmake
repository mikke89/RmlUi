#[[
    Various CMake utilities
]]

#[[
    Format the RmlUi version as it should normally be displayed
    Output:
        RMLUI_VERSION_SHORT: The RmlUi version as a string
]]
function(generate_rmlui_version_string)
	if(NOT RMLUI_VERSION_RELEASE)
		set(RMLUI_VERSION_SUFFIX "-dev")
	endif()
	if(PROJECT_VERSION_PATCH GREATER 0)
		set(RMLUI_VERSION_PATCH ".${PROJECT_VERSION_PATCH}")
	endif()
	set(RMLUI_VERSION_SHORT
		"${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}${RMLUI_VERSION_PATCH}${RMLUI_VERSION_SUFFIX}"
		PARENT_SCOPE
	)
endfunction()

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
