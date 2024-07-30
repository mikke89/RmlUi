# Try to find HarfBuzz include and library directories.
#
# The following CMake variables are required:
#   HARFBUZZ_DIR      - directories to search for the HarfBuzz source directory.
#   HARFBUZZ_LIB_DIRS - directories to search for the compiled HarfBuzz library.
#
# After a successful discovery, this will set the following:
#   HARFBUZZ_INCLUDE_DIR - directory containing the HarfBuzz header files.
#   HARFBUZZ_LIBRARY     - the compiled HarfBuzz library.
# In addition, the following IMPORTED target is created:
#   harfbuzz::harfbuzz

# Look for the library in config mode first.
find_package(harfbuzz CONFIG QUIET)
if(TARGET harfbuzz::harfbuzz)
	message(STATUS "Found HarfBuzz in config mode.")
	set(HARFBUZZ_LIBRARY "harfbuzz::harfbuzz")
	return()
else()
	message(STATUS "Looking for HarfBuzz in module mode.")
endif()

find_path(HARFBUZZ_INCLUDE_DIR
	NAMES hb.h
	HINTS ${HARFBUZZ_DIR}
	PATH_SUFFIXES harfbuzz)

find_library(HARFBUZZ_LIBRARY
	NAMES harfbuzz
	HINTS ${HARFBUZZ_DIR} ${HARFBUZZ_LIB_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(HarfBuzz DEFAULT_MSG HARFBUZZ_LIBRARY HARFBUZZ_INCLUDE_DIR)

if(HARFBUZZ_FOUND AND NOT TARGET harfbuzz::harfbuzz)
	add_library(harfbuzz::harfbuzz INTERFACE IMPORTED)
	set_target_properties(harfbuzz::harfbuzz PROPERTIES
		INTERFACE_INCLUDE_DIRECTORIES "${HARFBUZZ_INCLUDE_DIR}"
		INTERFACE_LINK_LIBRARIES "${HARFBUZZ_LIBRARY}")
endif()
