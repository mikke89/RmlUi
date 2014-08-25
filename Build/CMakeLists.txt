#===================================
# Build script for libRocket =======
#===================================

if(APPLE)
	if(IOS_PLATFORM)
		set(CMAKE_TOOLCHAIN_FILE cmake/Platform/iOS.cmake)
	endif(IOS_PLATFORM)
endif(APPLE)

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
set(LIBROCKET_VERSION_MINOR 3)
set(LIBROCKET_VERSION_PATCH 0)
set(LIBROCKET_VERSION_TWEAK 0)
set(PROJECT_VERSION ${LIBROCKET_VERSION_MAJOR}.${LIBROCKET_VERSION_MINOR}.${LIBROCKET_VERSION_PATCH}.${LIBROCKET_VERSION_TWEAK})

# Search in the 'cmake' directory for additional CMake modules.
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

# Old versions of CMake need some updated Modules, but we don't want
# to override newer versions of CMake which have working versions
if(CMAKE_MAJOR_VERSION LESS 3)
	list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/v2fixes)
endif()

#===================================
# Environment tests ================
#===================================

include(TestForANSIForScope)
include(TestForANSIStreamHeaders)
include(TestForSTDNamespace)

#===================================
# Provide hints as to where depends=
# might be found                   =
#===================================

if(NOT DEFINED ENV{FREETYPE_DIR})
	set(ENV{FREETYPE_DIR} "${PROJECT_SOURCE_DIR}/../Dependencies")
endif()

if(NOT DEFINED ENV{Boost_DIR})
	set(ENV{Boost_DIR} "${PROJECT_SOURCE_DIR}/../Dependencies")
endif()

if(NOT DEFINED ENV{LUA_DIR})
	set(ENV{LUA_DIR} "${PROJECT_SOURCE_DIR}/../Dependencies")
endif()

if(NOT DEFINED ENV{SDLDIR})
	set(ENV{SDLDIR} "${PROJECT_SOURCE_DIR}/../Dependencies")
endif()

if(NOT DEFINED ENV{SDLIMAGEDIR})
	set(ENV{SDLIMAGEDIR} "${PROJECT_SOURCE_DIR}/../Dependencies")
endif()

if(NOT DEFINED ENV{SFML_ROOT})
	set(ENV{SFML_ROOT} "${PROJECT_SOURCE_DIR}/../Dependencies")
endif()

#===================================
# Build options ====================
#===================================

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING
        "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
        FORCE)
endif()

if(NOT IOS)
	option(BUILD_SHARED_LIBS "Build shared libraries" ON)
endif(NOT IOS)

option(BUILD_PYTHON_BINDINGS "Build python bindings" OFF)
option(BUILD_LUA_BINDINGS "Build Lua bindings" OFF)
option(BUILD_SAMPLES "Build samples" OFF)
if(WIN32)
	option(SKIP_DIRECTX_SAMPLES "Skip build of all DirectX related samples.  Only applies if BUILD_SAMPLES is ON" OFF)
	option(SKIP_DIRECTX9_SAMPLE "Skip build of DirectX 9 related sample.  Only applies if BUILD_SAMPLES is ON and SKIP_DIRECTX_SAMPLES is OFF" OFF)
	option(SKIP_DIRECTX10_SAMPLE "Skip build of DirectX 10 related sample.  Only applies if BUILD_SAMPLES is ON and SKIP_DIRECTX_SAMPLES is OFF" OFF)
endif()

if(IOS)
	if(BUILD_SHARED_LIBS)
		message(FATAL_ERROR "BUILD_SHARED_LIBS must be OFF for iOS builds.  iOS does not support shared libraries.")
	endif(BUILD_SHARED_LIBS)
endif(IOS)

if(IOS)
	if(BUILD_SHARED_LIBS)
		message(FATAL_ERROR "BUILD_SHARED_LIBS must be OFF for iOS builds.  iOS does not support shared libraries.")
	endif(BUILD_SHARED_LIBS)
endif(IOS)

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
if(CMAKE_MAJOR_VERSION LESS 3)
	# Freetype changed the layout of its header files, we  need to use 
	# the FindFreetype module from cmake v3 at least, included here
	find_package(Freetype-v2fix REQUIRED)	
else()
	find_package(Freetype REQUIRED)	
endif()

if(FREETYPE_FOUND)
		include_directories(${FREETYPE_INCLUDE_DIRS})
		link_directories(${FREETYPE_LINK_DIRS})
		list(APPEND CORE_LINK_LIBS ${FREETYPE_LIBRARY})
endif()
mark_as_advanced(FREETYPE_INCLUDE_DIRS FREETYPE_LIBRARY FREETYPE_LINK_DIRECTORIES)

# Boost and Python
if(BUILD_PYTHON_BINDINGS)
    find_package(PythonInterp 2 REQUIRED)
    find_package(PythonLibs 2 REQUIRED)
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
        list(APPEND PY_BINDINGS_LINK_LIBS ${PYTHON_LIBRARY} ${Boost_LIBRARIES})
    endif()

endif()

#Lua
if(BUILD_LUA_BINDINGS)
    if(CMAKE_MAJOR_VERSION LESS 3)
        find_package(Lua-v2fix)
    else()
        find_package(Lua)
    endif()
    if(LUA_FOUND)
        include_directories(${LUA_INCLUDE_DIR})
        list(APPEND LUA_BINDINGS_LINK_LIBS ${LUA_LIBRARIES})
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
                        ${MASTER_${library}_PUB_HDR_FILES}
    )

    set_target_properties(${NAME} PROPERTIES
                           VERSION ${PROJECT_VERSION}
                           SOVERSION ${LIBROCKET_VERSION_MAJOR}
    )
    
    if(APPLE)
	if(NOT IOS)
    		set_target_properties(${NAME} PROPERTIES
    						OSX_ARCHITECTURES "i386;x86_64;"
    		)
	endif(NOT IOS)
    endif(APPLE)

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

        add_library(${NAME} MODULE ${Py${library}_SRC_FILES}
                            ${Py${library}_HDR_FILES}
                            ${Py${library}_PUB_HDR_FILES}
        )

    if(APPLE)
	if(NOT IOS)
    		set_target_properties(${NAME} PROPERTIES
    						OSX_ARCHITECTURES "$(ARCHS_STANDARD_32_64_BIT)"
    		)
	endif(NOT IOS)
    endif(APPLE)

        set_target_properties(${NAME} PROPERTIES PREFIX "")

        install(TARGETS ${NAME}
                LIBRARY DESTINATION ${PYTHON_INSTDIR}
        )
    endforeach(library)
endif()

# Build Lua bindings
if(BUILD_LUA_BINDINGS)
    set(LIBRARIES Core Controls)

    foreach(library ${LIBRARIES})
        set(NAME Rocket${library}Lua)

        add_library(${NAME} ${Lua${library}_SRC_FILES}
                            ${Lua${library}_HDR_FILES}
                            ${Lua${library}_PUB_HDR_FILES}
        )

        set_target_properties(${NAME} PROPERTIES
                           VERSION ${PROJECT_VERSION}
                           SOVERSION ${LIBROCKET_VERSION_MAJOR}
        )
        
    if(APPLE)
	if(NOT IOS)
    		set_target_properties(${NAME} PROPERTIES
    						OSX_ARCHITECTURES "$(ARCHS_STANDARD_32_64_BIT)"
    		)
	endif(NOT IOS)
    endif(APPLE)

        install(TARGETS ${NAME}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
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

if(BUILD_LUA_BINDINGS)
    target_link_libraries(RocketCoreLua RocketCore ${LUA_BINDINGS_LINK_LIBS})
    target_link_libraries(RocketControlsLua RocketControls RocketCoreLua ${LUA_BINDINGS_LINK_LIBS})
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
		if(SKIP_DIRECTX_SAMPLES)
			message("-- Skipping all DirectX samples")
		else()
			message("-- Determing if DirectX samples can be built")
			include(FindDirectX)
			find_package(DirectX)
			if(DirectX_FOUND)
				set(DIRECTX_SAMPLE_LIST)
				set(DIRECTX_SKIPPED_SAMPLE_LIST)

				# We should be able to build DirectX 9 sample
				message("-- Determing if DirectX samples can be built - Yes")

				if(SKIP_DIRECTX9_SAMPLE)
					message("-- Skipping build of DirectX 9 sample: User disabled")
					list(APPEND DIRECTX_SKIPPED_SAMPLE_LIST "DirectX9 ")
				else()
					if(DirectX_LIBRARY)
						if(DirectX_D3DX9_LIBRARY)
							list(APPEND DIRECTX_SAMPLE_LIST "DirectX9 ")
						else()
							set(SKIP_DIRECTX9_SAMPLE ON)
							message("-- Skipping build of DirectX 9 sample: DirectX_D3DX9_LIBRARY not found")
							list(APPEND DIRECTX_SKIPPED_SAMPLE_LIST "DirectX9 ")
						endif()
					else()
						set(SKIP_DIRECTX9_SAMPLE ON)
						message("-- Skipping build of DirectX 9 sample: DirectX_LIBRARY not found")
						list(APPEND DIRECTX_SKIPPED_SAMPLE_LIST "DirectX9 ")
					endif()
				endif()

				if(SKIP_DIRECTX10_SAMPLE)
					message("-- Skipping build of DirectX 10 sample: User disabled")
					list(APPEND DIRECTX_SKIPPED_SAMPLE_LIST "DirectX10 ")
				else()
					if(DirectX_D3D10_FOUND)
						list(APPEND DIRECTX_SAMPLE_LIST "DirectX10 ")
					else()
						set(SKIP_DIRECTX10_SAMPLE ON)
						message("-- Skipping build of DirectX 10 sample: Missing DirectX_D3D10_INCLUDE_DIR, DirectX_D3D10_LIBRARY or DirectX_D3DX10_LIBRARY")
						list(APPEND DIRECTX_SKIPPED_SAMPLE_LIST "DirectX10 ")
					endif()
				endif()


				if(DIRECTX_SAMPLE_LIST)
					message("-- Enabled DirectX samples: " ${DIRECTX_SAMPLE_LIST})
				endif()
				if(DIRECTX_SKIPPED_SAMPLE_LIST)
					message("-- Disabled DirectX samples: " ${DIRECTX_SKIPPED_SAMPLE_LIST})
				endif()
			else()
				message("-- Determing if DirectX samples can be built - No")
				set(SKIP_DIRECTX_SAMPLES ON)
				set(SKIP_DIRECTX9_SAMPLE ON)
			endif()
		endif()
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
		# shell/src/x11/InputX11.cpp:InitialiseX11Keymap uses Xkb if
		# possible instead of XGetKeyboardMapping for performance
		if(X11_Xkb_FOUND)
			FIND_PACKAGE_MESSAGE(X11 "Found X11 KBlib: ${X11_X11_LIB}" "[${X11_X11_LIB}][${X11_XkbINCLUDE_DIR}]")
			add_definitions(-DHAS_X11XKBLIB)
		endif()
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

    # Build and install the basic samples
    foreach(sample ${samples})
        bl_sample(${sample} ${sample_LIBRARIES})

        # The samples always set this as their current working directory
        install(DIRECTORY DESTINATION ${SAMPLES_DIR}/basic/${sample})
        install(TARGETS ${sample} 
        	RUNTIME DESTINATION ${SAMPLES_DIR}/${sample}
        	BUNDLE DESTINATION ${SAMPLES_DIR})
    endforeach()

	if(WIN32)
		if(NOT SKIP_DIRECTX9_SAMPLE)
			include_directories(${DirectX_INCLUDE_DIR})

			bl_sample(directx ${sample_LIBRARIES}  ${DirectX_LIBRARY} ${DirectX_D3DX9_LIBRARY})

			# The samples always set this as their current working directory
			install(DIRECTORY DESTINATION ${SAMPLES_DIR}/basic/directx)
			install(TARGETS directx 
				RUNTIME DESTINATION ${SAMPLES_DIR}/directx
				BUNDLE DESTINATION ${SAMPLES_DIR})
		endif()

		if(NOT SKIP_DIRECTX10_SAMPLE)
			include_directories(${DirectX_INCLUDE_DIR} ${DirectX_D3D10_INCLUDE_DIRS})

			bl_sample(directx10 ${sample_LIBRARIES}  ${DirectX_D3D10_LIBRARIES})

			# The samples always set this as their current working directory
			install(DIRECTORY DESTINATION ${SAMPLES_DIR}/basic/directx10)
			install(TARGETS directx10 
				RUNTIME DESTINATION ${SAMPLES_DIR}/directx10
				BUNDLE DESTINATION ${SAMPLES_DIR})
		endif()
	endif()

    message("-- Can SDL2 sample be built")
    find_package(SDL)
    if(SDL_FOUND)
	find_package(SDL_image)
	if(SDL_IMAGE_FOUND)
		find_package(GLEW)
		if(GLEW_FOUND)
        		message("-- Can SDL2 sample be built - yes")
			include_directories(${SDL_INCLUDE_DIR} ${GLEW_INCLUDE_DIR})

			bl_sample(sdl2 ${sample_LIBRARIES}  ${SDL_LIBRARY} ${SDL_IMAGE_LIBRARY} ${GLEW_LIBRARY})
			# The samples always set this as their current working directory
			install(DIRECTORY DESTINATION ${SAMPLES_DIR}/basic/sdl2)
			install(TARGETS sdl2
				RUNTIME DESTINATION ${SAMPLES_DIR}/sdl2
				BUNDLE DESTINATION ${SAMPLES_DIR})
		else()
		        message("-- Can SDL2 sample be built - GLEW not found")
		endif()
	else()
	        message("-- Can SDL2 sample be built - SDL2_image not found")
	endif()
    else()
        message("-- Can SDL2 sample be built - SDL2 not found")
    endif()


	message("-- Can SFML 1.x sample be built")
	find_package(SFML 1 COMPONENTS graphics window system)
	if(NOT SFML_FOUND)
		message("-- Can SFML 1.x sample be built - no")
	elseif(SFML_VERSION_MAJOR GREATER 1)
		message("-- Can SFML 1.x sample be built - no: Version 2 detected")
	else()
		message("-- Can SFML 1.x sample be built - yes")

		include_directories(${SFML_INCLUDE_DIR})

		bl_sample(sfml ${sample_LIBRARIES} ${SFML_LIBRARIES})
			# The samples always set this as their current working directory
			install(DIRECTORY DESTINATION ${SAMPLES_DIR}/basic/sfml)
			install(TARGETS sfml
				RUNTIME DESTINATION ${SAMPLES_DIR}/sfml
				BUNDLE DESTINATION ${SAMPLES_DIR})
	endif()

	message("-- Can SFML 2.x sample be built")
	find_package(SFML 2 COMPONENTS graphics window system)
	if(NOT SFML_FOUND)
		message("-- Can SFML 2.x sample be built - no")
	else()
		find_package(GLEW)
                if(GLEW_FOUND)
			message("-- Can SFML 2.x sample be built - yes: with GLEW")
			include_directories(${SFML_INCLUDE_DIR} ${GLEW_INCLUDE_DIR})
			add_definitions( -DENABLE_GLEW )
			bl_sample(sfml2 ${sample_LIBRARIES} ${SFML_LIBRARIES} ${GLEW_LIBRARY})
		else()
			message("-- Can SFML 2.x sample be built - yes: without GLEW")
			include_directories(${SFML_INCLUDE_DIR})
			bl_sample(sfml2 ${sample_LIBRARIES} ${SFML_LIBRARIES})
		endif()

			# The samples always set this as their current working directory
			install(DIRECTORY DESTINATION ${SAMPLES_DIR}/basic/sfml2)
			install(TARGETS sfml2
				RUNTIME DESTINATION ${SAMPLES_DIR}/sfml2
				BUNDLE DESTINATION ${SAMPLES_DIR})
	endif()

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

	if(BUILD_PYTHON_BINDINGS)
	    # Build and install pyinvaders sample
	    bl_sample(pyinvaders ${sample_LIBRARIES} ${PYTHON_LIBRARIES} ${PY_BINDINGS_LINK_LIBS})
	    install(DIRECTORY DESTINATION ${SAMPLES_DIR}/pyinvaders)
	    install(TARGETS pyinvaders
	    	RUNTIME DESTINATION ${SAMPLES_DIR}/pyinvaders
	    	BUNDLE DESTINATION ${SAMPLES_DIR})
	endif()
    
    if(BUILD_LUA_BINDINGS)
        bl_sample(luainvaders RocketCoreLua RocketControlsLua ${sample_LIBRARIES} ${LUA_BINDINGS_LINK_LIBS})
        install(DIRECTORY DESTINATION ${SAMPLES_DIR}/luainvaders)
        install(TARGETS luainvaders 
        	RUNTIME DESTINATION ${SAMPLES_DIR}/luainvaders
        	BUNDLE DESTINATION ${SAMPLES_DIR})
    endif()
endif()


#===================================
# Installation =====================
#===================================

if(BUILD_LUA_BINDINGS AND BUILD_PYTHON_BINDINGS)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/Include/Rocket
            DESTINATION include
    )
else()
	if(NOT BUILD_LUA_BINDINGS AND NOT BUILD_PYTHON_BINDINGS)
	    install(DIRECTORY ${PROJECT_SOURCE_DIR}/Include/Rocket
	            DESTINATION include
	            PATTERN "Python" EXCLUDE
	            PATTERN "Lua" EXCLUDE
	    )
	else()
		if(BUILD_PYTHON_BINDINGS)
			install(FILES ${PROJECT_SOURCE_DIR}/bin/rocket.py
				DESTINATION ${PYTHON_INSTDIR}
			)
			install(DIRECTORY ${PROJECT_SOURCE_DIR}/Include/Rocket
				DESTINATION include
				PATTERN "Lua" EXCLUDE
			)
		else()
			if(BUILD_LUA_BINDINGS)
				install(DIRECTORY ${PROJECT_SOURCE_DIR}/Include/Rocket
					DESTINATION include
					PATTERN "Python" EXCLUDE
				)
			else()
				message(FATAL_ERROR "ASSERT: Unexpected option combination, this is a logical impossibility.")
			endif()
		endif()
	endif()
endif()

if(BUILD_SAMPLES)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/Samples/assets
            DESTINATION ${SAMPLES_DIR}
    )

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

    if(BUILD_PYTHON_BINDINGS)
        install(DIRECTORY ${PROJECT_SOURCE_DIR}/Samples/pyinvaders/data
                DESTINATION ${SAMPLES_DIR}/pyinvaders
        )
    endif()

    if(BUILD_LUA_BINDINGS)
        install(DIRECTORY ${PROJECT_SOURCE_DIR}/Samples/luainvaders/data 
                DESTINATION ${SAMPLES_DIR}/luainvaders
        )
        install(DIRECTORY ${PROJECT_SOURCE_DIR}/Samples/luainvaders/lua 
                DESTINATION ${SAMPLES_DIR}/luainvaders
        )
    endif()
endif()
