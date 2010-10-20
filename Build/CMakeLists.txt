cmake_minimum_required(VERSION 2.6)

project(libRocket)

# Set up build options
option(STATIC_LIB "Build the libRocket using static libraries." OFF)
set(FREETYPE_INCLUDE "/usr/include/freetype2" CACHE STRING "Path to freetype")

# Configure shared/static libraries
if(STATIC_LIB)
	set(LIBRARY_TYPE STATIC)
	add_definitions(-DSTATIC_LIB)
else(STATIC_LIB)
	set(LIBRARY_TYPE SHARED)
endif(STATIC_LIB)

# Set up paths
set(INCLUDE "../Include;${FREETYPE_INCLUDE}")
set(SOURCE "../Source")
set(LIBRARIES Core Controls Debugger)

include_directories(${INCLUDE})

# Build each library
foreach(library ${LIBRARIES})
	set(LIB_SOURCE_PATH "${SOURCE}/${library}")
	set(NAME "Rocket${library}")
	
	aux_source_directory(${LIB_SOURCE_PATH} LIB_SOURCES)
	add_library(${NAME} ${LIBRARY_TYPE} ${LIB_SOURCES})
	set(LIB_SOURCES)
endforeach(library)
