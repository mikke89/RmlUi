#===================================
# Build script for libRocket =======
#===================================

cmake_minimum_required(VERSION 2.6)

project(libRocket C CXX)

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
option(BUILD_SAMPLES "Build samples" OFF)

if(NOT BUILD_SHARED_LIBS)
    add_definitions(-DSTATIC_LIB)
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
    add_definitions(-Wno-conversion-null)
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
# Link libraries ===================
#===================================

target_link_libraries(RocketCore ${CORE_LINK_LIBS})

if(BUILD_PYTHON_BINDINGS)
    target_link_libraries(_rocketcore RocketCore ${PY_BINDINGS_LINK_LIBS})
    target_link_libraries(_rocketcontrols RocketControls ${PY_BINDINGS_LINK_LIBS})
endif()


#===================================
# Build samples ====================
#===================================

# Build and link the samples
macro(bl_sample NAME)
    add_executable(${NAME} ${${NAME}_SRC_FILES}
                           ${${NAME}_HDR_FILES}
    )

    target_link_libraries(${NAME} shell)
endmacro()

if(BUILD_SAMPLES)
    include(SampleFileList)

    find_package(OpenGL REQUIRED)

    if(OPENGL_FOUND)
        include_directories(${OPENGL_INCLUDE_DIR})
    endif()

    set(SAMPLES_DIR opt/Rocket/Samples)

    # The samples use the shell library
    include_directories(${PROJECT_SOURCE_DIR}/Samples/shell/include)

    # Deal with platform specific sources for sample shell
    if(WIN32)
        list(APPEND samples basic_directx)
        list(APPEND shell_SRC_FILES
            ${PROJECT_SOURCE_DIR}/Samples/shell/src/win32/ShellWin32.cpp
            ${PROJECT_SOURCE_DIR}/Samples/shell/src/win32/InputWin32.cpp
        )
        list(APPEND shell_HDR_FILES
            ${PROJECT_SOURCE_DIR}/Samples/shell/include/win32/InputWin32.h
        )
    elseif(APPLE)
        list(APPEND shell_SRC_FILES
            ${PROJECT_SOURCE_DIR}/Samples/shell/src/macosx/ShellMacOSX.cpp
            ${PROJECT_SOURCE_DIR}/Samples/shell/src/macosx/InputMacOSX.cpp
        )
        list(APPEND shell_HDR_FILES
            ${PROJECT_SOURCE_DIR}/Samples/shell/include/macosx/InputMacOSX.h
        )
    else()
        list(APPEND shell_SRC_FILES
            ${PROJECT_SOURCE_DIR}/Samples/shell/src/x11/ShellX11.cpp
            ${PROJECT_SOURCE_DIR}/Samples/shell/src/x11/InputX11.cpp
        )
        list(APPEND shell_HDR_FILES
            ${PROJECT_SOURCE_DIR}/Samples/shell/include/x11/InputX11.h
        )
    endif()

    # Build and install sample shell library
    add_library(shell ${shell_SRC_FILES}
                      ${shell_HDR_FILES}
    )

    target_link_libraries(shell RocketCore RocketControls RocketDebugger ${OPENGL_LIBRARIES})

    install(TARGETS shell
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib
    )

    list(APPEND samples treeview customlog drag loaddocument)

    # Build and install the basic samples
    foreach(sample ${samples})
        bl_sample(${sample})

        # The samples always set this as their current working directory
        install(DIRECTORY DESTINATION ${SAMPLES_DIR}/basic/${sample})

        install(TARGETS ${sample}
                RUNTIME DESTINATION ${SAMPLES_DIR}
        )
    endforeach()

    # Build and install invaders sample
    bl_sample(invaders)
    install(DIRECTORY DESTINATION ${SAMPLES_DIR}/invaders)
    install(TARGETS invaders
            RUNTIME DESTINATION ${SAMPLES_DIR}/invaders
    )

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

if(BUILD_SAMPLES)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/Samples/basic/treeview/data
            DESTINATION ${SAMPLES_DIR}/basic/treeview
    )
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/Samples/basic/drag/data
            DESTINATION ${SAMPLES_DIR}/basic/drag
    )
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/Samples/invaders/data
            DESTINATION ${SAMPLES_DIR}/invaders
    )
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/Samples/assets
            DESTINATION ${SAMPLES_DIR}
    )
endif()
