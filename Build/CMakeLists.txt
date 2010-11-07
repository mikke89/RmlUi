#===================================
# Build script for libRocket =======
#===================================

cmake_minimum_required(VERSION 2.6)

project(libRocket CXX)

set(LIBROCKET_VERSION_MAJOR 1)
set(LIBROCKET_VERSION_MINOR 2)
set(LIBROCKET_VERSION_PATCH 0)
set(PROJECT_VERSION ${LIBROCKET_VERSION_MAJOR}.${LIBROCKET_VERSION_MINOR}.${LIBROCKET_VERSION_PATCH})


#===================================
# Environment tests ================
#===================================

include(TestForANSIForScope)
include(TestForANSIStreamHeaders)
include(TestForSTDNamespace)


#===================================
# Build options ====================
#===================================

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING
        "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
        FORCE)
endif()

option(BUILD_SHARED_LIBS "Build shared libraries" ON)

if(NOT BUILD_SHARED_LIBS)
    add_definitions(-DSTATIC_LIB)
endif()


#===================================
# Find dependencies ================
#===================================

find_package(Freetype REQUIRED)

if(FREETYPE_FOUND)
    include_directories(${FREETYPE_INCLUDE_DIRS})
    list(APPEND CORE_LINK_LIBS ${FREETYPE_LIBRARY})
endif()


#===================================
# Setup paths ======================
#===================================

include_directories(
    ${PROJECT_SOURCE_DIR}/../Include
)

set(SOURCE "../Source")


#===================================
# Build libraries ==================
#===================================

set(LIBRARIES Core Controls Debugger)

foreach(library ${LIBRARIES})
	set(LIB_SOURCE_PATH "${SOURCE}/${library}")
	set(NAME "Rocket${library}")

	aux_source_directory(${LIB_SOURCE_PATH} LIB_SOURCES)
	add_library(${NAME} ${LIB_SOURCES})
    set_target_properties(${NAME} PROPERTIES
                           VERSION ${PROJECT_VERSION}
                           SOVERSION ${LIBROCKET_VERSION_MAJOR}
    )
	set(LIB_SOURCES)

    install(TARGETS ${NAME}
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib
    )
endforeach(library)


#===================================
# Link libraries====================
#===================================

target_link_libraries(RocketCore ${CORE_LINK_LIBS})


#===================================
# Installation =====================
#===================================

install(DIRECTORY ${PROJECT_SOURCE_DIR}/../Include/Rocket
        DESTINATION include
)
