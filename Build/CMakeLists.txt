#===================================
# Build script for libRocket =======
#===================================

cmake_minimum_required(VERSION 2.6)

project(libRocket CXX)

set(LIBROCKET_VERSION_MAJOR 1)
set(LIBROCKET_VERSION_MINOR 2)
set(LIBROCKET_VERSION_PATCH 0)
set(PROJECT_VERSION ${LIBROCKET_VERSION_MAJOR}.${LIBROCKET_VERSION_MINOR}.${LIBROCKET_VERSION_PATCH})

# Search in the 'cmake' directory for additional CMake modules.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)


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
option(BUILD_PYTHON_BINDINGS "Build python bindings" OFF)

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

if(BUILD_PYTHON_BINDINGS)
    find_package(PythonInterp REQUIRED)
    find_package(PythonLibs REQUIRED)
    execute_process(
        COMMAND ${PYTHON_EXECUTABLE} -c "from distutils import sysconfig; print(sysconfig.get_python_lib(1,0,prefix=''))"
        OUTPUT_VARIABLE PYTHON_INSTDIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(PYTHONLIBS_FOUND)
        include_directories(${PYTHON_INCLUDE_DIR})
    endif()

    #set(Boost_USE_STATIC_LIBS OFF)
    #set(Boost_USE_MULTITHREADED ON)
    find_package(Boost 1.40.0 COMPONENTS python REQUIRED)
    if(Boost_FOUND)
        include_directories(${Boost_INCLUDE_DIR})
        list(APPEND PY_BINDINGS_LINK_LIBS ${Boost_LIBRARIES})
    endif()

endif()

#===================================
# Setup paths ======================
#===================================

set(PROJECT_SOURCE_DIR ${PROJECT_SOURCE_DIR}/..)

include_directories(
    ${PROJECT_SOURCE_DIR}/Include
)

# Include list of source files
include(FileList)

#===================================
# Build libraries ==================
#===================================

set(LIBRARIES Core Controls Debugger)

foreach(library ${LIBRARIES})
    set(NAME Rocket${library})

    add_library(${NAME} ${${library}_SRC_FILES}
                        ${${library}_HDR_FILES}
                        ${${library}_PUB_HDR_FILES}
    )

    set_target_properties(${NAME} PROPERTIES
                           VERSION ${PROJECT_VERSION}
                           SOVERSION ${LIBROCKET_VERSION_MAJOR}
    )

    install(TARGETS ${NAME}
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib
    )
endforeach(library)

# Build python bindings

if(BUILD_PYTHON_BINDINGS)
    set(LIBRARIES core controls)

    foreach(library ${LIBRARIES})
        set(NAME _rocket${library})

        add_library(${NAME} ${Py${library}_SRC_FILES}
                            ${Py${library}_HDR_FILES}
                            ${Py${library}_PUB_HDR_FILES}
        )

        set_target_properties(${NAME} PROPERTIES PREFIX "")

        install(TARGETS ${NAME}
                LIBRARY DESTINATION ${PYTHON_INSTDIR}
        )
    endforeach(library)
endif()


#===================================
# Link libraries====================
#===================================

target_link_libraries(RocketCore ${CORE_LINK_LIBS})

if(BUILD_PYTHON_BINDINGS)
    target_link_libraries(_rocketcore RocketCore ${PY_BINDINGS_LINK_LIBS})
    target_link_libraries(_rocketcontrols RocketControls ${PY_BINDINGS_LINK_LIBS})
endif()


#===================================
# Installation =====================
#===================================

if(BUILD_PYTHON_BINDINGS)
    install(FILES ${PROJECT_SOURCE_DIR}/bin/rocket.py
            DESTINATION ${PYTHON_INSTDIR}
    )
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/Include/Rocket
            DESTINATION include
    )
else()
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/Include/Rocket
            DESTINATION include
            PATTERN "Python" EXCLUDE
    )
endif()
