# Try to find HarfBuzz include and library directories.
#
# The following CMake variables are required:
# HARFBUZZ_DIR      - directories to search for the HarfBuzz source directory.
# HARFBUZZ_LIB_DIRS - directories to search for the compiled HarfBuzz library.
#
# After a successful discovery, this will set the following for inclusion where needed:
# HARFBUZZ_INCLUDE_DIR - directory containing the HarfBuzz header files.
# HARFBUZZ_LIBRARY     - the compiled HarfBuzz library.

# Look for the library in config mode first.
find_package(harfbuzz CONFIG)
if (TARGET harfbuzz::harfbuzz)
	message(STATUS "Found HarfBuzz in config mode: version ${HARFBUZZ_VERSION}.")
	set(HARFBUZZ_LIBRARY "harfbuzz::harfbuzz")
	return()
else()
	message(STATUS "Looking for HarfBuzz in module mode instead.")
endif()

find_path(HARFBUZZ_INCLUDE_DIR
	NAMES hb.h
	HINTS ${HARFBUZZ_DIR})

find_library(HARFBUZZ_LIBRARY
	NAMES harfbuzz
	HINTS ${HARFBUZZ_DIR} ${HARFBUZZ_LIB_DIRS})

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(HarfBuzz DEFAULT_MSG HARFBUZZ_LIBRARY HARFBUZZ_INCLUDE_DIR)

if (HARFBUZZ_FOUND)
	message(STATUS "Found HarfBuzz library at " ${HARFBUZZ_LIBRARY})
endif()
