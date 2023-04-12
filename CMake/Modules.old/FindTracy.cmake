# - Try to find Tracy
# Once done, this will define
#
#  TRACY_FOUND - system has Tracy
#  TRACY_INCLUDE_DIR - the Tracy include directory

set(TRACY_DIR "" CACHE PATH "Parent directory of Tracy library")

find_path(TRACY_INCLUDE_DIR Tracy.hpp PATHS ${TRACY_DIR} $ENV{TRACY_DIR} PATH_SUFFIXES tracy)

set(TRACY_FOUND "NO")
if(TRACY_INCLUDE_DIR)
	message(STATUS "Found Tracy ${TRACY_INCLUDE_DIR}...")
	mark_as_advanced(TRACY_DIR TRACY_INCLUDE_DIR)
	set(TRACY_FOUND "YES")
elseif(Tracy_FIND_REQUIRED)
	 message(FATAL_ERROR "Required library Tracy not found! Install the library and try again. If the library is already installed, set the missing variables manually in cmake.")
else()
	message(STATUS "Library Tracy not found...")
endif()
