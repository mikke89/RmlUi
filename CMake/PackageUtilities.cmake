#[[
	Utility functions used for packaging RmlUi.
]]

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
	Install a text file with build info.
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
