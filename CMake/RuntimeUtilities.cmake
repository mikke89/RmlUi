#[[
	Utility functions used to set the output of runtime executables and dependencies.
]]

#[[
	Change the output directories of binaries and libraries of all targets declared in this scope. This is particularly
	helpful on Windows when building the project as a shared library, so that all DLLs are located together with any
	built executables. More generally, placing these files in the top-level binary directory makes them easier to
	locate. The set() call is guarded against pre-definitions in order to respect consumer choice.
]]
function(setup_binary_output_directories)
	if(NOT DEFINED CMAKE_RUNTIME_OUTPUT_DIRECTORY)
		set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}" PARENT_SCOPE)
	endif()
	if(NOT DEFINED CMAKE_LIBRARY_OUTPUT_DIRECTORY)
		set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}" PARENT_SCOPE)
	endif()
	if(NOT DEFINED CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
		set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}" PARENT_SCOPE)
	endif()
endfunction()

# RMLUI_CMAKE_MINIMUM_VERSION_RAISE_NOTICE:
# From CMake 3.21 there is no need for the following function, as we can set the RUNTIME_DEPENDENCY_SET on
# install(TARGETS) directly.
#[[
	Output variable to conditionally set up runtime dependency arguments when installing targets.
	This feature is only available from CMake 3.21, on older version this feature will be disabled.
	Output variable:
		- RMLUI_RUNTIME_DEPENDENCY_SET_ARG: Argument to use for the install(TARGETS) command.
]]
function(setup_runtime_dependency_set_arg)
	set(RMLUI_RUNTIME_DEPENDENCY_SET_ARG "" PARENT_SCOPE)
	if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.21")
		set(RMLUI_RUNTIME_DEPENDENCY_SET_ARG RUNTIME_DEPENDENCY_SET rmlui_runtime_dependencies PARENT_SCOPE)
	endif()
endfunction()

#[[
	Install runtime dependencies on supported platforms, when enabled by the user.
]]
function(install_runtime_dependencies)
	if(WIN32 AND RMLUI_RUNTIME_DEPENDENCY_SET_ARG)
		option(RMLUI_INSTALL_RUNTIME_DEPENDENCIES "Include runtime dependencies when installing RmlUi." ON)
		mark_as_advanced(RMLUI_INSTALL_RUNTIME_DEPENDENCIES)
		if(RMLUI_INSTALL_RUNTIME_DEPENDENCIES)
			install(RUNTIME_DEPENDENCY_SET rmlui_runtime_dependencies
				PRE_EXCLUDE_REGEXES "api-ms-" "ext-ms-"
				POST_EXCLUDE_REGEXES ".*system32/.*\\.dll"
			)
		endif()
	endif()
endfunction()
