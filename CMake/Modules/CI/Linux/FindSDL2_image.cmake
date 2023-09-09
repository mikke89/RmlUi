#[[
    Find module for SDL_image version 2 that matches the naming convention of the SDL2_imageConfig.cmake file
    provided in official distributions of the SDL2_image library

    This is necessary on Ubuntu 20.04 because the libsdl2-image-dev package doesn't provide the config file.
    https://packages.ubuntu.com/focal/amd64/libsdl2-image-dev/filelist
]]

set(SDL2_image_FOUND TRUE)

add_library(SDL2_image::SDL2_image INTERFACE IMPORTED)
target_link_libraries(SDL2_image::SDL2_image INTERFACE "SDL2_image")
