#[[
	Various global CMake utilities for RmlUi, functions only.
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
		- friendly_name: Friendly name of the target
		- target_name: Name of the CMake target the project will link against
]]
function(report_dependency_not_found friendly_name target_name)
	message(FATAL_ERROR
		"${friendly_name} could not be found.\n"
		"Please ensure that ${friendly_name} can be found by CMake, or linked to using \"${target_name}\" as its "
		"target name. If you are consuming RmlUi from another CMake project, you can create an ALIAS target to "
		"offer an alternative name for a CMake target."
	)
endfunction()

#[[
	Verify that the target is found and print a message, otherwise stop execution.
	Arguments:
		- friendly_name: Friendly name of the target
		- target_name: Name of the CMake target the project will link against
		- success_message: Message to show when the target exists (optional)
]]
function(report_dependency_found_or_error friendly_name target_name)
	if(NOT TARGET ${target_name})
		report_dependency_not_found(${friendly_name} ${target_name})
	endif()
	if(ARGC GREATER "2" AND ARGV2)
		set(success_message " - ${ARGV2}")
	endif()
	message(STATUS "Found ${target_name}${success_message}")
endfunction()

#[[
	Print a message to the console indicating a library from a native platform SDK hasn't been found.
	Arguments:
		- library_name: Name of the library
]]
function(report_not_found_native_library library_name)
	if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
		set(SDK_NOTICE "In order to ensure it is found, install the Windows SDK and build RmlUi inside a Visual Studio Developer CLI environment.\n"
			"More info: https://learn.microsoft.com/en-us/visualstudio/ide/reference/command-prompt-powershell"
		)
	elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
		set(SDK_NOTICE "In order to ensure it is found, install the macOS SDK.\n"
			"More info: https://developer.apple.com/macos/"
		)
	endif()

	message(NOTICE
		"CMake failed to find the ${library_name} library. Depending on the compiler, underlying build system "
		"and environment setup, linkage of the RmlUi samples executables might fail."
		"\n${SDK_NOTICE}"
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
	set(install_dirs "src")
	foreach(dir IN ITEMS "data" "lua")
		if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${dir}")
			list(APPEND install_dirs "${dir}")
		endif()
	endforeach()
	file(RELATIVE_PATH sample_path ${PROJECT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
	install(TARGETS ${TARGET_NAME}
		${RMLUI_RUNTIME_DEPENDENCY_SET_ARG}
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	)
	install(DIRECTORY ${install_dirs}
		DESTINATION "${CMAKE_INSTALL_DATADIR}/${sample_path}"
	)
endfunction()

#[[
	Install all license files, including for all installed packages in vcpkg if in use.
]]
function(install_licenses)
	set(bin_licenses_dir "${CMAKE_CURRENT_BINARY_DIR}/Licenses")
	configure_file("${PROJECT_SOURCE_DIR}/LICENSE.txt"
		"${bin_licenses_dir}/LICENSE.txt" COPYONLY
	)
	configure_file("${PROJECT_SOURCE_DIR}/Include/RmlUi/Core/Containers/LICENSE.txt"
		"${bin_licenses_dir}/LICENSE.Core.ThirdParty.txt" COPYONLY
	)
	configure_file("${PROJECT_SOURCE_DIR}/Source/Debugger/LICENSE.txt"
		"${bin_licenses_dir}/LICENSE.Debugger.ThirdParty.txt" COPYONLY
	)

	if(VCPKG_TOOLCHAIN)
		set(vcpkg_share_dir "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share")
		file(GLOB copyright_files "${vcpkg_share_dir}/*/copyright")
		foreach(copyright_file IN LISTS copyright_files)
			get_filename_component(name ${copyright_file} DIRECTORY)
			get_filename_component(name ${name} NAME)
			if(NOT "${name}" MATCHES "^vcpkg-")
				set(copy_destination "${bin_licenses_dir}/Dependencies/${name}.txt")
				configure_file(${copyright_file} ${copy_destination} COPYONLY)
			endif()
		endforeach()
	endif()

	install(DIRECTORY "${bin_licenses_dir}/" DESTINATION "${CMAKE_INSTALL_DATADIR}")
endfunction()

#[[
	Install text file with build info.
]]
function(install_build_info)
	if(NOT RMLUI_ARCHITECTURE OR NOT RMLUI_COMMIT_DATE OR NOT RMLUI_RUN_ID OR NOT RMLUI_SHA)
		message(FATAL_ERROR "Cannot install build info: Missing variables")
	endif()
	generate_rmlui_version_string()
	file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/Build.txt"
		"RmlUi ${RMLUI_VERSION_SHORT} binaries for ${RMLUI_ARCHITECTURE}.\n\n"
		"https://github.com/mikke89/RmlUi\n\n"
		"Built using ${CMAKE_GENERATOR} (${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}) on ${RMLUI_COMMIT_DATE} (run ${RMLUI_RUN_ID}).\n"
		"Commit id: ${RMLUI_SHA}"
	)
	install(FILES "${CMAKE_CURRENT_BINARY_DIR}/Build.txt"
		DESTINATION "${CMAKE_INSTALL_DATADIR}"
	)
endfunction()

#[[
	Install all dependencies found for the current vcpkg target triplet.
]]
function(install_vcpkg_dependencies)
	if(NOT VCPKG_TOOLCHAIN)
		message(FATAL_ERROR "Cannot install vcpkg dependencies: vcpkg not in use")
	endif()
	set(vcpkg_triplet_dir "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}")
	set(common_patterns
		PATTERN "${VCPKG_TARGET_TRIPLET}/tools" EXCLUDE
		PATTERN "pkgconfig" EXCLUDE
		PATTERN "vcpkg*" EXCLUDE
		PATTERN "*.pdb" EXCLUDE
	)
	message(STATUS "Installing vcpkg dependencies from: ${vcpkg_triplet_dir}")
	install(DIRECTORY "${vcpkg_triplet_dir}/"
		DESTINATION "${RMLUI_INSTALL_DEPENDENCIES_DIR}"
		CONFIGURATIONS "Release"
		${common_patterns}
		PATTERN "debug" EXCLUDE
		PATTERN "*debug.cmake" EXCLUDE
	)
	install(DIRECTORY "${vcpkg_triplet_dir}/"
		DESTINATION "${RMLUI_INSTALL_DEPENDENCIES_DIR}"
		CONFIGURATIONS "Debug"
		${common_patterns}
		PATTERN "${VCPKG_TARGET_TRIPLET}/bin" EXCLUDE
		PATTERN "${VCPKG_TARGET_TRIPLET}/lib" EXCLUDE
		PATTERN "*release.cmake" EXCLUDE
	)
endfunction()
