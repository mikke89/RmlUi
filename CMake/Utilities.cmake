#[[
	Various global utility functions for RmlUi.
]]

#[[
	Format the RmlUi version as it should normally be displayed.
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
	Stop execution and print an error message for the dependency.
	Arguments:
		- friendly_name: Friendly name of the dependency
		- package_name: Name of the package to search for
		- target_name: Name of the CMake target the project will link against
]]
function(report_dependency_not_found friendly_name package_name target_name)
	message(FATAL_ERROR
		"${friendly_name} could not be found.\n"
		"Please ensure that ${friendly_name} can be found by CMake, or linked to using \"${target_name}\" as its "
		"target name. The location of the build directory of the dependency can be provided by setting the "
		"\"${package_name}_ROOT\" CMake variable. If you are consuming RmlUi from another CMake project, you can "
		"create an ALIAS target to offer an alternative name for a CMake target."
	)
endfunction()

#[[
	Print a message for the dependency after being found.
	Arguments:
		- package_name: Name of the package to search for
		- target_name: Name of the CMake target the project will link against
		- success_message: Message to show when the target exists (optional)
	Note: The name and signature of this function should match the macro in `RmlUiConfig.cmake.in`.
]]
function(report_dependency_found package_name target_name)
	set(message "")
	if(DEFINED ${package_name}_VERSION AND NOT ${package_name}_VERSION STREQUAL "UNKNOWN")
		set(message " v${${package_name}_VERSION}")
	endif()
	if(ARGC GREATER "2" AND ARGV2)
		set(message "${message} - ${ARGV2}")
	endif()
	message(STATUS "Found ${target_name}${message}")
endfunction()

#[[
	Verify that the target is found and print a message, otherwise stop execution.
	Arguments:
		- friendly_name: Friendly name of the dependency
		- package_name: Name of the package to search for
		- target_name: Name of the CMake target the project will link against
		- success_message [optional]: Message to show when the target exists
	Note: The name and signature of this function should match the macro in `RmlUiConfig.cmake.in`.
]]
function(report_dependency_found_or_error friendly_name package_name target_name)
	if(NOT TARGET ${target_name})
		report_dependency_not_found(${friendly_name} ${package_name} ${target_name})
	endif()
	set(success_message "")
	if(ARGC GREATER "3" AND ARGV3)
		set(success_message "${ARGV3}")
	endif()
	report_dependency_found(${package_name} ${target_name} ${success_message})
endfunction()

#[[
	Returns a list of data directories for the current target.
	Arguments:
		- target: Name of the target
		- out_var: Name of the returned variable
]]
function(get_data_dirs target out_var)
	set(data_dirs "")
	foreach(dir IN ITEMS "data" "lua")
		if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${dir}")
			list(APPEND data_dirs "${dir}")
		endif()
	endforeach()
	set(${out_var} ${data_dirs} PARENT_SCOPE)
endfunction()

#[[
	Set compiler options and features that are common to all RmlUi targets.
	Arguments:
		- target: The name of the target to set
]]
function(set_common_target_options target)
	target_compile_features(${target} PUBLIC cxx_std_14)
	set_target_properties(${target} PROPERTIES C_EXTENSIONS OFF CXX_EXTENSIONS OFF)

	if(RMLUI_COMPILER_OPTIONS)
		if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
			target_compile_options(${target} PRIVATE -Wall -Wextra -pedantic)
		elseif(MSVC)
			target_compile_options(${target} PRIVATE /W4 /w44062 /permissive-)
			target_compile_definitions(${target} PRIVATE _CRT_SECURE_NO_WARNINGS)
			if(CMAKE_GENERATOR MATCHES "Visual Studio")
				target_compile_options(${target} PRIVATE /MP)
			endif()
		endif()
	endif()

	if(RMLUI_WARNINGS_AS_ERRORS)
		if(NOT RMLUI_COMPILER_OPTIONS)
			message(FATAL_ERROR "Option RMLUI_WARNINGS_AS_ERRORS requires RMLUI_COMPILER_OPTIONS=ON.")
		endif()

		if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
			target_compile_options(${target} PRIVATE -Werror)
		elseif(MSVC)
			target_compile_options(${target} PRIVATE /WX)
		else()
			message(FATAL_ERROR "Unknown compiler, cannot enable option RMLUI_WARNINGS_AS_ERRORS.")
		endif()
	endif()

	# Set Emscripten-specific properties and assets for samples and test executables.
	get_target_property(target_type ${target} TYPE)
	if(EMSCRIPTEN AND target_type STREQUAL "EXECUTABLE")
		# Make Emscripten generate the default HTML shell for the sample.
		set_property(TARGET ${target} PROPERTY SUFFIX ".html")

		# Enables Asyncify which we only need since the backend doesn't control the main loop. This enables us to yield
		# to the browser during the backend call to Backend::ProcessEvents(). Asyncify results in larger and slower
		# code, users are instead encouraged to use 'emscripten_set_main_loop()' and family.
		target_link_libraries(${target} PRIVATE "-sASYNCIFY")

		# We don't know the needed memory beforehand, allow it to grow.
		target_link_libraries(${target} PRIVATE "-sALLOW_MEMORY_GROWTH")

		# Add common assets.
		set(common_assets_dir "Samples/assets")
		target_link_libraries(${target} PRIVATE "--preload-file ${PROJECT_SOURCE_DIR}/${common_assets_dir}/@/${common_assets_dir}/")
		file(GLOB asset_files "${PROJECT_SOURCE_DIR}/${common_assets_dir}/*")

		# Add sample-specific assets.
		get_data_dirs(${target} data_dirs)
		foreach(source_relative_dir IN LISTS data_dirs)
			set(abs_dir "${CMAKE_CURRENT_SOURCE_DIR}/${source_relative_dir}")
			file(RELATIVE_PATH root_relative_dir "${PROJECT_SOURCE_DIR}" "${abs_dir}")
			target_link_libraries(${target} PRIVATE "--preload-file ${abs_dir}/@/${root_relative_dir}/")
			file(GLOB sample_data_files "${abs_dir}/*")
			list(APPEND asset_files "${sample_data_files}")
		endforeach()

		# Add a linker dependency to all asset files, so that the linker runs again if any asset is modified.
		set_target_properties(${target} PROPERTIES LINK_DEPENDS "${asset_files}")
	endif()
endfunction()

#[[
	Create installation rule for MSVC program database (PDB) files.
	Arguments:
		- target: The name of the target
]]
function(install_target_pdb target)
	if(MSVC)
		if(BUILD_SHARED_LIBS)
			# The following only works for linker-generated PDBs, not compiler-generated PDBs produced in static builds.
			install(FILES "$<TARGET_PDB_FILE:${target}>"
				DESTINATION "${CMAKE_INSTALL_LIBDIR}"
				OPTIONAL
			)
		else()
			get_property(output_name TARGET ${target} PROPERTY OUTPUT_NAME)
			install(FILES "$<TARGET_FILE_DIR:${target}>/${output_name}.pdb"
				DESTINATION "${CMAKE_INSTALL_LIBDIR}"
				OPTIONAL
			)
		endif()
	endif()
endfunction()

#[[
	Create installation rules for sample targets.
	Arguments:
		- target: The name of the target
]]
function(install_sample_target target)
	get_data_dirs(${target} data_dirs)
	file(RELATIVE_PATH sample_path ${PROJECT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
	install(TARGETS ${TARGET_NAME}
		${RMLUI_RUNTIME_DEPENDENCY_SET_ARG}
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	)
	set(install_dirs "src" ${data_dirs})
	install(DIRECTORY ${install_dirs}
		DESTINATION "${CMAKE_INSTALL_DATADIR}/${sample_path}"
	)
endfunction()

#[[
	Enable or disable a given configuration type for multi-configuration generators.
	Arguments:
		- name: The name of the new configuration
		- clone_config: The name of the configuration to clone compile flags from
		- enable: Enable or disable configuration
]]
function(enable_configuration_type name clone_config enable)
	if(CMAKE_CONFIGURATION_TYPES)
		string(TOUPPER "${name}" name_upper)
		string(TOUPPER "${clone_config}" clone_config_upper)
		if(enable)
			list(APPEND CMAKE_CONFIGURATION_TYPES "${name}")
			list(REMOVE_DUPLICATES CMAKE_CONFIGURATION_TYPES)
			set("CMAKE_MAP_IMPORTED_CONFIG_${name_upper}" "${name};${clone_config}" CACHE INTERNAL "" FORCE)
			set("CMAKE_C_FLAGS_${name_upper}" "${CMAKE_C_FLAGS_${clone_config_upper}}" CACHE INTERNAL "" FORCE)
			set("CMAKE_CXX_FLAGS_${name_upper}" "${CMAKE_CXX_FLAGS_${clone_config_upper}}" CACHE INTERNAL "" FORCE)
			set("CMAKE_EXE_LINKER_FLAGS_${name_upper}" "${CMAKE_EXE_LINKER_FLAGS_${clone_config_upper}}" CACHE INTERNAL "" FORCE)
			set("CMAKE_SHARED_LINKER_FLAGS_${name_upper}" "${CMAKE_SHARED_LINKER_FLAGS_${clone_config_upper}}" CACHE INTERNAL "" FORCE)
		else()
			list(REMOVE_ITEM CMAKE_CONFIGURATION_TYPES "${name}")
		endif()
		set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING "List of configurations to enable" FORCE)
	endif()
endfunction()
