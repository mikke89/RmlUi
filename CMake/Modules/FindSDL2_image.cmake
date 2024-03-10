#[[
	Find module for SDL_image version 2 that matches the naming convention of the SDL2_imageConfig.cmake file
	provided in official distributions of the SDL2_image library.

	This is necessary on some distributions, including Ubuntu 20.04 and 22.04, because the libsdl2-image-dev package
	doesn't provide the config file. https://packages.ubuntu.com/focal/amd64/libsdl2-image-dev/filelist
	Furthermore, the CMake integrated find module is only compatible with the SDL1_image.

	Defines the imported CMake target:
		SDL2_image::SDL2_image

	In addition, the following CMake variables are defined according to normal conventions:
		SDL2_IMAGE_LIBRARIES
		SDL2_IMAGE_INCLUDE_DIRS
		SDL2_IMAGE_FOUND
]]

# Prefer config mode.
find_package(SDL2_image CONFIG QUIET)
if(TARGET SDL2_image::SDL2_image)
	return()
endif()

# Not found in config mode, proceed as a normal find module.
find_path(SDL2_IMAGE_INCLUDE_DIR SDL_image.h
	HINTS ENV SDL2_IMAGE_DIR
	PATH_SUFFIXES SDL2
)

find_library(SDL2_IMAGE_LIBRARY
	NAMES SDL2_image
	HINTS ENV SDL2_IMAGE_DIR
	PATH_SUFFIXES lib
)

set(SDL2_IMAGE_INCLUDE_DIRS ${SDL2_IMAGE_INCLUDE_DIR})
set(SDL2_IMAGE_LIBRARIES ${SDL2_IMAGE_LIBRARY})

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(SDL2_image
	REQUIRED_VARS SDL2_IMAGE_LIBRARIES SDL2_IMAGE_INCLUDE_DIRS
)

mark_as_advanced(SDL2_IMAGE_LIBRARY SDL2_IMAGE_INCLUDE_DIR)

if(SDL2_IMAGE_FOUND)
	if(NOT TARGET SDL2_image::SDL2_image)
		add_library(SDL2_image::SDL2_image INTERFACE IMPORTED)
		set_target_properties(SDL2_image::SDL2_image PROPERTIES
			INTERFACE_INCLUDE_DIRECTORIES "${SDL2_IMAGE_INCLUDE_DIRS}"
			INTERFACE_LINK_LIBRARIES "${SDL2_IMAGE_LIBRARIES}"
		)
	endif()
endif()
