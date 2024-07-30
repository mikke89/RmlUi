#[[
	Custom find module for SDL for Emscripten compilation.

	Input variables:
		none
	Output variables:
		SDL2_FOUND
		SDL2_VERSION

	Resulting targets:
		SDL2::SDL2

	More info:
	https://emscripten.org/docs/porting/multimedia_and_graphics/OpenGL-support.html
]]

include(FindPackageHandleStandardArgs)
include(FindPackageMessage)

if(NOT TARGET SDL2::SDL2)
	# If no version was specified, set default
	if(NOT DEFINED SDL2_FIND_VERSION)
		set(SDL2_FIND_VERSION "2")
	endif()

	# Check if requested SDL version is valid
	if((SDL2_FIND_VERSION VERSION_LESS "2") OR (SDL2_FIND_VERSION VERSION_GREATER_EQUAL "3"))
		message(FATAL_ERROR "The requested SDL2 version ${SDL2_FIND_VERSION} is invalid.")
	endif()

	# Emscripten includes SDL support as part of it's SDK, meaning there's no need to find it
	set(SDL2_FOUND TRUE)
	add_library(SDL2::SDL2 INTERFACE IMPORTED)

	# Set found SDL version based on latest Emscripten SDK at the time of writing this file
	# Version set based on latest Emscripten SDK at the time of writing this file
	set(SDL2_VERSION "2.24.2")

	# Version number to pass to compiler
	set(SDL2_EMSCRIPTEN_COMPILER_SELECTED_VERSION "2")

	# Enable compilation and linking against SDL
	target_compile_options(SDL2::SDL2 INTERFACE "-sUSE_SDL=${SDL2_EMSCRIPTEN_COMPILER_SELECTED_VERSION}")
	target_link_libraries(SDL2::SDL2 INTERFACE "-sUSE_SDL=${SDL2_EMSCRIPTEN_COMPILER_SELECTED_VERSION}")

	# Get final compiler and linker flags to print them
	get_target_property(SDL2_COMPILE_FLAGS SDL2::SDL2 "INTERFACE_COMPILE_OPTIONS")
	get_target_property(SDL2_LINK_FLAGS SDL2::SDL2 "INTERFACE_LINK_OPTIONS")

	find_package_message(
		"SDL2"
		"SDL ${SDL2_VERSION} has been found as part of the Emscripten SDK."
		"[${SDL2_COMPILE_FLAGS}][${SDL2_LINK_FLAGS}]"
	)

	# Clean scope
	unset(SDL2_COMPILE_FLAGS)
	unset(SDL2_COMPILE_FLAGS)
	unset(SDL2_EMSCRIPTEN_COMPILER_SELECTED_VERSION)
else()
	# Since the target already exists, we declare it as found
	set(SDL2_FOUND TRUE)
	if(NOT DEFINED SDL2_VERSION)
		set(SDL2_VERSION "UNKNOWN")
	endif()
endif()
