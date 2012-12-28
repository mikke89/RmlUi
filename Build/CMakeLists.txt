#===================================
# Build script for libRocket =======
#===================================

# We use the new OSX_ARCHITECTURES property
# and GNUInstallDirs module
cmake_minimum_required(VERSION 2.8.5)

if(COMMAND cmake_policy)
  cmake_policy(SET CMP0015 NEW)
endif(COMMAND cmake_policy)

project(libRocket C CXX)

# paths
include(GNUInstallDirs)

set(LIBROCKET_VERSION_MAJOR 1)
set(LIBROCKET_VERSION_MINOR 2)
set(LIBROCKET_VERSION_PATCH 1)
set(PROJECT_VERSION ${LIBROCKET_VERSION_MAJOR}.${LIBROCKET_VERSION_MINOR}.${LIBROCKET_VERSION_PATCH})

# Search in the 'cmake' directory for additional CMake modules.
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

set(CMAKE_DEBUG_POSTFIX  "_d")

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
    set(CMAKE_BUILD_TYPE Release CACHE STRING
        "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
        FORCE)
endif()

option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(BUILD_PYTHON_BINDINGS "Build python bindings" OFF)
option(BUILD_SAMPLES "Build samples" OFF)

if(NOT BUILD_SHARED_LIBS)
    add_definitions(-DSTATIC_LIB)
endif()

#on windows, check for VC10 and fix the multiple compile target issue.
IF(WIN32)
  if(MSVC)
    if(${MSVC_VERSION} STREQUAL 1600 OR ${MSVC_VERSION} STRGREATER 1600)
      message("Visual Studio 2010 (${MSVC_VERSION}) build fix at play (/FORCE:MULTIPLE)")
      set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /FORCE:MULTIPLE")
    endif()
  endif() 
ENDIF(WIN32)


#===================================
# Find dependencies ================
#===================================

# FreeType
find_package(Freetype REQUIRED)	
if(FREETYPE_FOUND)
		include_directories(${FREETYPE_INCLUDE_DIRS})
		link_directories(${FREETYPE_LINK_DIRS})
		list(APPEND CORE_LINK_LIBS ${FREETYPE_LIBRARY})
endif()
mark_as_advanced(FREETYPE_INCLUDE_DIRS FREETYPE_LIBRARY FREETYPE_LINK_DIRECTORIES)

# Boost and Python
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
    
    if(APPLE)
    	set_target_properties(${NAME} PROPERTIES
    					OSX_ARCHITECTURES "i386;x86_64;"
    	)
    endif()

    install(TARGETS ${NAME}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
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
target_link_libraries(RocketControls RocketCore)
target_link_libraries(RocketDebugger RocketCore)

if(BUILD_PYTHON_BINDINGS)
    target_link_libraries(_rocketcore RocketCore ${PY_BINDINGS_LINK_LIBS})
    target_link_libraries(_rocketcontrols RocketControls ${PY_BINDINGS_LINK_LIBS})
endif()


#===================================
# Build samples ====================
#===================================

# Build and link the samples
macro(bl_sample NAME)
	if (WIN32)
    	add_executable(${NAME} WIN32 ${${NAME}_SRC_FILES} ${${NAME}_HDR_FILES} )
	elseif(APPLE)
		add_executable(${NAME} MACOSX_BUNDLE ${${NAME}_SRC_FILES} ${${NAME}_HDR_FILES} )
    else()
    	add_executable(${NAME} ${${NAME}_SRC_FILES} ${${NAME}_HDR_FILES} )
    endif()
    
    if (APPLE)
    	# We only support i386 for the samples as it still uses Carbon
    	set_target_properties(${NAME} PROPERTIES OSX_ARCHITECTURES "i386;" )
    endif()

    target_link_libraries(${NAME} ${ARGN})
endmacro()

if(BUILD_SAMPLES)
    include(SampleFileList)

    set(samples treeview customlog drag loaddocument)
    set(tutorials template datagrid datagrid_tree tutorial_drag)
    
    set(sample_LIBRARIES
    	shell 
    	RocketCore 
    	RocketControls
    	RocketDebugger
    )

	# Find OpenGL 
	find_package(OpenGL REQUIRED)
    	   
    if(OPENGL_FOUND)
	include_directories(${OPENGL_INCLUDE_DIR})
	list(APPEND sample_LIBRARIES ${OPENGL_LIBRARIES})
    endif()
    
    # Set up required system libraries
    if(WIN32)
		list(APPEND samples directx)
    	list(APPEND sample_LIBRARIES d3d9 d3dx9)
    elseif(APPLE)
		include(FindCarbon)
    	find_package(Carbon REQUIRED)
    	
    	if (Carbon_FOUND)
    		include_directories(${Carbon_INCLUDE_DIR})
        	list(APPEND sample_LIBRARIES ${Carbon_LIBRARIES})
    	endif()
    else()
    	find_package(X11 REQUIRED)
        if (X11_FOUND)
        	list(APPEND sample_LIBRARIES ${X11_LIBRARIES})
        endif()
    endif()
   
    set(SAMPLES_DIR opt/Rocket/Samples CACHE PATH "path to samples dir")

    # The samples and tutorials use the shell library
    include_directories(${PROJECT_SOURCE_DIR}/Samples/shell/include)

    # Build and install sample shell library
    add_library(shell STATIC ${shell_SRC_FILES} ${shell_HDR_FILES})
    if (APPLE)
    	# We only support i386 for the samples as it still uses Carbon
    	set_target_properties(shell PROPERTIES OSX_ARCHITECTURES "i386;")
    endif()

    install(TARGETS shell
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )

    # Build and install the basic samples
    foreach(sample ${samples})
        bl_sample(${sample} ${sample_LIBRARIES})

        # The samples always set this as their current working directory
        install(DIRECTORY DESTINATION ${SAMPLES_DIR}/basic/${sample})
        install(TARGETS ${sample} 
        	RUNTIME DESTINATION ${SAMPLES_DIR}/${sample}
        	BUNDLE DESTINATION ${SAMPLES_DIR})
    endforeach()

    # Build and install the tutorials
    foreach(tutorial ${tutorials})
        bl_sample(${tutorial} ${sample_LIBRARIES})

        # The tutorials always set this as their current working directory
        install(DIRECTORY DESTINATION ${SAMPLES_DIR}/tutorial/${tutorial})
        install(TARGETS ${tutorial}
                RUNTIME DESTINATION ${SAMPLES_DIR}/${tutorial}
                BUNDLE DESTINATION ${SAMPLES_DIR})
    endforeach()

    # Build and install invaders sample
    bl_sample(invaders ${sample_LIBRARIES})
    install(DIRECTORY DESTINATION ${SAMPLES_DIR}/invaders)
    install(TARGETS invaders 
    	RUNTIME DESTINATION ${SAMPLES_DIR}/invaders
    	BUNDLE DESTINATION ${SAMPLES_DIR})
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
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/Samples/tutorial/template/data
            DESTINATION ${SAMPLES_DIR}/tutorial/template
    )
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/Samples/tutorial/datagrid/data
            DESTINATION ${SAMPLES_DIR}/tutorial/datagrid
    )
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/Samples/tutorial/datagrid_tree/data
            DESTINATION ${SAMPLES_DIR}/tutorial/datagrid_tree
    )
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/Samples/tutorial/tutorial_drag/data
            DESTINATION ${SAMPLES_DIR}/tutorial/tutorial_drag
    )
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
