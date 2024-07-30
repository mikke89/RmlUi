#[[
	Custom find module for OpenGL ES for Emscripten compilation.

	Input variables:
		OpenGL_ENABLE_EMULATION

	Output variables:
		OpenGL_FOUND
		OpenGL_VERSION

	Resulting targets:
		OpenGL::GL

	More info:
	https://emscripten.org/docs/porting/multimedia_and_graphics/OpenGL-support.html
]]

include(FindPackageHandleStandardArgs)
include(FindPackageMessage)

if(NOT TARGET OpenGL::GL)
	# If no version was specified, set default
	if(NOT DEFINED OpenGL_FIND_VERSION)
		set(OpenGL_FIND_VERSION "3")
	endif()
	if(NOT DEFINED OpenGL_ENABLE_EMULATION)
		set(OpenGL_ENABLE_EMULATION ON)
	endif()

	# Check if requested OpenGL version is valid
	if((OpenGL_FIND_VERSION VERSION_LESS "1") OR (OpenGL_FIND_VERSION VERSION_GREATER_EQUAL "4"))
		message(FATAL_ERROR "The requested OpenGL version ${OpenGL_FIND_VERSION} is invalid.")
	endif()

	# Emscripten includes OpenGL ES support as part of it's SDK, meaning there's no need to find it
	set(OpenGL_FOUND TRUE)
	add_library(OpenGL::GL INTERFACE IMPORTED)

	# Set found OpenGL version
	if((OpenGL_FIND_VERSION VERSION_GREATER_EQUAL "1") AND (OpenGL_FIND_VERSION VERSION_LESS "2"))
		set(OpenGL_VERSION "1")
	elseif((OpenGL_FIND_VERSION VERSION_GREATER_EQUAL "2") AND (OpenGL_FIND_VERSION VERSION_LESS "3"))
		set(OpenGL_VERSION "2")
	elseif((OpenGL_FIND_VERSION VERSION_GREATER_EQUAL "3") AND (OpenGL_FIND_VERSION VERSION_LESS "4"))
		set(OpenGL_VERSION "3")
	endif()

	#[[
		The Emscripten SDK already makes OpenGL available by default without any additional configuration.
		However, additional settings might be needed in some cases.
	]]

	# Handle OpenGL 1 edge case
	# More info: https://emscripten.org/docs/porting/multimedia_and_graphics/OpenGL-support.html#emulation-of-older-desktop-opengl-api-features
	if(OpenGL_VERSION VERSION_EQUAL "1")
		target_link_libraries(OpenGL::GL INTERFACE "-sLEGACY_GL_EMULATION")
	endif()

	# Handle OpenGL ES software emulation
	# More info: https://emscripten.org/docs/porting/multimedia_and_graphics/OpenGL-support.html#opengl-es-2-0-3-0-emulation
	if(OpenGL_ENABLE_EMULATION AND OpenGL_VERSION VERSION_GREATER_EQUAL "2")
		target_link_libraries(OpenGL::GL INTERFACE "-sFULL_ES${OpenGL_VERSION}")
	endif()

	# Get final compiler and linker flags to print them
	get_target_property(OpenGL_COMPILE_FLAGS OpenGL::GL "INTERFACE_COMPILE_OPTIONS")
	get_target_property(OpenGL_LINK_FLAGS OpenGL::GL "INTERFACE_LINK_OPTIONS")

	find_package_message(
		"OpenGL"
		"OpenGL ${OpenGL_VERSION} has been found as part of the Emscripten SDK."
		"[${OpenGL_COMPILE_FLAGS}][${OpenGL_LINK_FLAGS}]"
	)

	# Clean scope
	unset(OpenGL_COMPILE_FLAGS)
	unset(OpenGL_LINK_FLAGS)
else()
	# Since the target already exists, we declare it as found
	set(OpenGL_FOUND TRUE)
	if(NOT DEFINED OpenGL_VERSION)
		set(OpenGL_VERSION "UNKNOWN")
	endif()
endif()
