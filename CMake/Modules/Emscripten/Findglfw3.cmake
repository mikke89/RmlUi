#[[
	Custom find module for GLFW for Emscripten compilation.

	Input variables:
		none
	Output variables:
		glfw3_FOUND
		glfw3_VERSION

	Resulting targets:
		glfw

	More info:
	https://emscripten.org/docs/porting/multimedia_and_graphics/OpenGL-support.html
]]

include(FindPackageHandleStandardArgs)
include(FindPackageMessage)

if(NOT TARGET glfw)
	# If no version was specified, set default
	if(NOT DEFINED glfw3_FIND_VERSION)
		set(glfw3_FIND_VERSION "1")
	endif()

	# Check if requested GLFW version is valid
	if((glfw3_FIND_VERSION VERSION_LESS "3") OR (glfw3_FIND_VERSION VERSION_GREATER_EQUAL "4"))
		message(FATAL_ERROR "The requested GLFW version ${glfw3_FIND_VERSION} is invalid.")
	endif()

	# Emscripten includes GLFW support as part of it's SDK, meaning there's no need to find it
	set(glfw3_FOUND TRUE)
	add_library(glfw INTERFACE IMPORTED)

	# Set found GLFW version based on latest Emscripten SDK at the time of writing this file
	# Version set based on latest Emscripten SDK at the time of writing this file
	set(glfw3_VERSION "3")

	# Version number to pass to compiler
	set(glfw3_EMSCRIPTEN_COMPILER_SELECTED_VERSION ${glfw3_VERSION})

	# Enable compilation and linking against GLFW
	target_compile_options(glfw INTERFACE "-sUSE_GLFW=${glfw3_EMSCRIPTEN_COMPILER_SELECTED_VERSION}")
	target_link_libraries(glfw INTERFACE "-sUSE_GLFW=${glfw3_EMSCRIPTEN_COMPILER_SELECTED_VERSION}")

	# Get final compiler and linker flags to print them
	get_target_property(glfw3_COMPILE_FLAGS glfw "INTERFACE_COMPILE_OPTIONS")
	get_target_property(glfw3_LINK_FLAGS glfw "INTERFACE_LINK_OPTIONS")

	find_package_message(
		"glfw3"
		"GLFW ${glfw3_VERSION} has been found as part of the Emscripten SDK."
		"[${glfw3_COMPILE_FLAGS}][${glfw3_LINK_FLAGS}]"
	)

	# Clean scope
	unset(glfw3_COMPILE_FLAGS)
	unset(glfw3_COMPILE_FLAGS)
	unset(glfw3_EMSCRIPTEN_COMPILER_SELECTED_VERSION)
else()
	# Since the target already exists, we declare it as found
	set(glfw3_FOUND TRUE)
	if(NOT DEFINED glfw3_VERSION)
		set(glfw3_VERSION "UNKNOWN")
	endif()
endif()
