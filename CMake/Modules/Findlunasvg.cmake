# Try to find LunaSVG

find_path(LUNASVG_INCLUDE_DIR document.h
          HINTS $ENV{LUNASVG_DIR}
          PATH_SUFFIXES lunasvg lunasvg/include include )

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

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(lunasvg  DEFAULT_MSG
                                  LUNASVG_LIBRARY LUNASVG_INCLUDE_DIR)

mark_as_advanced(LUNASVG_INCLUDE_DIR LUNASVG_LIBRARY_DEBUG LUNASVG_LIBRARY_RELEASE )

set(LUNASVG_LIBRARIES ${LUNASVG_LIBRARY} )
set(LUNASVG_INCLUDE_DIRS ${LUNASVG_INCLUDE_DIR} )
