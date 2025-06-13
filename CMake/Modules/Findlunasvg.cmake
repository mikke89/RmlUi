#[[
	Custom find module for lunasvg, will use existing target if available.

	Input variables:
		LUNASVG_INCLUDE_DIR
		LUNASVG_LIBRARY_DEBUG
		LUNASVG_LIBRARY_RELEASE

	Output variables:
		lunasvg_FOUND
		lunasvg_VERSION

	Resulting targets:
		lunasvg::lunasvg
]]

include(FindPackageHandleStandardArgs)

if(NOT TARGET lunasvg::lunasvg)
	# Look for lunasvg 3.0+ official config
	find_package("lunasvg" CONFIG QUIET)
endif()

if(NOT TARGET lunasvg::lunasvg)
	# Look for vcpkg port
	find_package("unofficial-lunasvg" CONFIG QUIET)
	if(TARGET unofficial::lunasvg::lunasvg)
		add_library(lunasvg::lunasvg ALIAS unofficial::lunasvg::lunasvg)
	endif()
endif()

if(TARGET lunasvg::lunasvg)
	# Since the target already exists, we declare it as found
	set(lunasvg_FOUND TRUE)
	if(NOT DEFINED lunasvg_VERSION)
		set(lunasvg_VERSION "UNKNOWN")
	endif()
elseif(TARGET lunasvg)
	# This is for when lunasvg is added via add_subdirectory
	add_library(lunasvg::lunasvg ALIAS lunasvg)
	set(lunasvg_FOUND TRUE)
	if(NOT DEFINED lunasvg_VERSION)
		set(lunasvg_VERSION "UNKNOWN")
	endif()
else()
	# Search for the library using input variables
	find_path(LUNASVG_INCLUDE_DIR lunasvg.h
		HINTS $ENV{LUNASVG_DIR}
		PATH_SUFFIXES lunasvg lunasvg/include include)

	find_library(LUNASVG_LIBRARY_DEBUG NAMES lunasvg liblunasvg
		HINTS $ENV{LUNASVG_DIR} $ENV{LUNASVG_DIR}/build
		PATH_SUFFIXES debug Debug)

	find_library(LUNASVG_LIBRARY_RELEASE NAMES lunasvg liblunasvg
		HINTS $ENV{LUNASVG_DIR} $ENV{LUNASVG_DIR}/build
		PATH_SUFFIXES release Release)

	set(LUNASVG_LIBRARY
		debug ${LUNASVG_LIBRARY_DEBUG}
		optimized ${LUNASVG_LIBRARY_RELEASE}
	)

	find_package_handle_standard_args(lunasvg DEFAULT_MSG
		LUNASVG_LIBRARY LUNASVG_INCLUDE_DIR)

	mark_as_advanced(LUNASVG_INCLUDE_DIR LUNASVG_LIBRARY_DEBUG LUNASVG_LIBRARY_RELEASE)

	set(LUNASVG_LIBRARIES ${LUNASVG_LIBRARY})
	set(LUNASVG_INCLUDE_DIRS ${LUNASVG_INCLUDE_DIR})

	if(lunasvg_FOUND)
		add_library(lunasvg::lunasvg INTERFACE IMPORTED)
		set(lunasvg_VERSION "UNKNOWN")

		target_include_directories(lunasvg::lunasvg INTERFACE ${LUNASVG_INCLUDE_DIRS})
		target_link_libraries(lunasvg::lunasvg INTERFACE ${LUNASVG_LIBRARIES})
	endif()

	if(NOT DEFINED lunasvg_VERSION)
		set(lunasvg_VERSION "UNKNOWN")
	endif()
endif()
