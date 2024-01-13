# Try to find HarfBuzz include and library directories.
#
# The following CMake variables are required:
# HARFBUZZ_DIR      - directories to search for the HarfBuzz source directory.
# HARFBUZZ_LIB_DIRS - directories to search for the compiled HarfBuzz library.
#
# After a successful discovery, this will set the following for inclusion where needed:
# HARFBUZZ_INCLUDE_DIR - directory containing the HarfBuzz header files.
# HARFBUZZ_LIBRARY_DIR - directory containing the compiled HarfBuzz library.

FIND_PATH(HARFBUZZ_INCLUDE_DIR
	NAMES hb.h
	HINTS ${HARFBUZZ_DIR})

FIND_LIBRARY(HARFBUZZ_LIBRARY_DIR
	NAMES harfbuzz
	HINTS ${HARFBUZZ_LIB_DIRS})

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(HarfBuzz DEFAULT_MSG HARFBUZZ_INCLUDE_DIR HARFBUZZ_LIBRARY_DIR)
