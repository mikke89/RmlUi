#[[
    Details about sample backends, their source code files and linking requirements

    Everytime a new backend gets added or its target name is modified, please update
    the list of available backends found in OptionsLists.cmake

    This file is not meant to be used by consumers of the library, only by the RmlUi CMake project
]]

add_library(rmlui_backend_common_headers INTERFACE)
target_sources(rmlui_backend_common_headers INTERFACE
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Backend.h"
)

add_library(rmlui_backend_Win32_GL2 INTERFACE)
target_sources(rmlui_backend_Win32_GL2 INTERFACE
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Platform_Win32.cpp"
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Renderer_GL2.cpp"
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Backend_Win32_GL2.cpp"
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Platform_Win32.h"
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Renderer_GL2.h"
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Include_Windows.h"
)
target_link_libraries(rmlui_backend_Win32_GL2 INTERFACE rmlui_backend_common_headers OpenGL::GL)

add_library(rmlui_backend_Win32_VK INTERFACE)
target_sources(rmlui_backend_Win32_VK INTERFACE    
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Platform_Win32.cpp"
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Renderer_VK.cpp"
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Backend_Win32_VK.cpp"
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Platform_Win32.h"
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Renderer_VK.h"
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Vulkan/ShadersCompiledSPV.h"
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Vulkan/vk_mem_alloc.h"
)
target_link_libraries(rmlui_backend_Win32_VK INTERFACE rmlui_backend_common_headers Vulkan::Vulkan)

add_library(rmlui_backend_X11_GL2 INTERFACE)
target_sources(rmlui_backend_X11_GL2 INTERFACE    
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Platform_X11.cpp"
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Renderer_GL2.cpp"
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Backend_X11_GL2.cpp"
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Platform_X11.h"
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Renderer_GL2.h"
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Include_Xlib.h"
)

# RMLUI_CMAKE_MINIMUM_VERSION_RAISE_NOTICE:
# Once the minimum CMake version is CMake >= 3.14, "${X11_LIBRARIES}" should
# be substituted by "X11:X11" in addition to any of the other imported that might
# be required. More info: 
# https://cmake.org/cmake/help/latest/module/FindX11.html
target_link_libraries(rmlui_backend_X11_GL2 INTERFACE rmlui_backend_common_headers OpenGL::GL ${X11_LIBRARIES})

add_library(rmlui_backend_SDL_GL2 INTERFACE)
target_sources(rmlui_backend_SDL_GL2 INTERFACE    
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Platform_SDL.cpp"
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Renderer_GL2.cpp"
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Backend_SDL_GL2.cpp"
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Platform_SDL.h"
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Renderer_GL2.h"
)
target_link_libraries(rmlui_backend_SDL_GL2 INTERFACE rmlui_backend_common_headers OpenGL::GL SDL2::SDL2 GLEW::GLEW SDL2_image::SDL2_image)

add_library(rmlui_backend_SDL_GL3 INTERFACE)
target_sources(rmlui_backend_SDL_GL3 INTERFACE    
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Platform_SDL.cpp"
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Renderer_GL3.cpp"
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Backend_SDL_GL3.cpp"
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Platform_SDL.h"
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Renderer_GL3.h"
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Include_GL3.h"
)
target_link_libraries(rmlui_backend_SDL_GL3 INTERFACE rmlui_backend_common_headers OpenGL::GL SDL2::SDL2 SDL2_image::SDL2_image)
if(UNIX)
    # The OpenGL 3 renderer implementation uses dlopen/dlclose
    # This is required in some UNIX and UNIX-like operating systems to load shared object files at runtime
    target_link_libraries(rmlui_backend_SDL_GL3 INTERFACE ${CMAKE_DL_LIBS})
endif()

add_library(rmlui_backend_SDL_VK INTERFACE)
target_sources(rmlui_backend_SDL_VK INTERFACE    
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Platform_SDL.cpp"
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Renderer_VK.cpp"
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Backend_SDL_VK.cpp"
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Platform_SDL.h"
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Renderer_VK.h"
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Vulkan/ShadersCompiledSPV.h"
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Vulkan/vk_mem_alloc.h"
)
target_link_libraries(rmlui_backend_SDL_VK INTERFACE rmlui_backend_common_headers Vulkan::Vulkan SDL2::SDL2)
if(UNIX)
    # The Vulkan renderer implementation uses dlopen/dlclose
    # This is required in some UNIX and UNIX-like operating systems to load shared object files at runtime
    target_link_libraries(rmlui_backend_SDL_VK INTERFACE ${CMAKE_DL_LIBS})
endif()

add_library(rmlui_backend_SDL_SDLrenderer INTERFACE)
target_sources(rmlui_backend_SDL_SDLrenderer INTERFACE    
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Renderer_SDL.cpp"
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Platform_SDL.cpp"
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Backend_SDL_SDLrenderer.cpp"
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Platform_SDL.h"
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Renderer_SDL.h"
)
target_link_libraries(rmlui_backend_SDL_SDLrenderer INTERFACE rmlui_backend_common_headers SDL2::SDL2 SDL2_image::SDL2_image)

add_library(rmlui_backend_SFML_GL2 INTERFACE)
target_sources(rmlui_backend_SFML_GL2 INTERFACE    
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Platform_SFML.cpp"
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Renderer_GL2.cpp"
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Backend_SFML_GL2.cpp"
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Platform_SFML.h"
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Renderer_GL2.h"
)
target_link_libraries(rmlui_backend_SFML_GL2 INTERFACE 
    rmlui_backend_common_headers OpenGL::GL rmlui_SFML_old_wrapper SFML::Graphics SFML::Window SFML::System
)
if(WIN32)
    target_link_libraries(rmlui_backend_SFML_GL2 INTERFACE SFML::Main)
endif()

add_library(rmlui_backend_GLFW_GL2 INTERFACE)
target_sources(rmlui_backend_GLFW_GL2 INTERFACE    
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Platform_GLFW.cpp"
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Renderer_GL2.cpp"
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Backend_GLFW_GL2.cpp"
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Platform_GLFW.h"
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Renderer_GL2.h"
)
target_link_libraries(rmlui_backend_GLFW_GL2 INTERFACE rmlui_backend_common_headers OpenGL::GL glfw)

add_library(rmlui_backend_GLFW_GL3 INTERFACE)
target_sources(rmlui_backend_GLFW_GL3 INTERFACE    
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Platform_GLFW.cpp"
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Renderer_GL3.cpp"
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Backend_GLFW_GL3.cpp"
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Platform_GLFW.h"
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Renderer_GL3.h"
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Include_GL3.h"
)
target_link_libraries(rmlui_backend_GLFW_GL3 INTERFACE rmlui_backend_common_headers OpenGL::GL glfw)
if(UNIX)
    # The OpenGL 3 renderer implementation uses dlopen/dlclose
    # This is required in some UNIX and UNIX-like operating systems to load shared object files at runtime
    target_link_libraries(rmlui_backend_GLFW_GL3 INTERFACE ${CMAKE_DL_LIBS})
endif()

add_library(rmlui_backend_GLFW_VK INTERFACE)
target_sources(rmlui_backend_GLFW_VK INTERFACE    
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Platform_GLFW.cpp"
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Renderer_VK.cpp"
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Backend_GLFW_VK.cpp"
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Platform_GLFW.h"
    "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Renderer_VK.h"
)
target_link_libraries(rmlui_backend_GLFW_VK INTERFACE rmlui_backend_common_headers Vulkan::Vulkan glfw)
if(UNIX)
    # The Vulkan renderer implementation uses dlopen/dlclose
    # This is required in some UNIX and UNIX-like operating systems to load shared object files at runtime
    target_link_libraries(rmlui_backend_GLFW_VK INTERFACE ${CMAKE_DL_LIBS})
endif()
