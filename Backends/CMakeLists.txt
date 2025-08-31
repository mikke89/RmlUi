#[[
	Interface libraries for each backend, including their source files and linking requirements.

	Every time a new backend gets added or its target name is modified, please update
	the list of available backends found in OptionsLists.cmake
]]

add_library(rmlui_backend_common_headers INTERFACE)
target_include_directories(rmlui_backend_common_headers INTERFACE "${CMAKE_CURRENT_LIST_DIR}")
target_sources(rmlui_backend_common_headers INTERFACE
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Backend.h"
)

if(RMLUI_BACKEND_SIMULATE_TOUCH)
	set(RMLUI_BACKEND_SIMULATE_TOUCH_SUPPORTED
		"SDL_GL2"
		"SDL_GL3"
		"SDL_VK"
		"SDL_SDLrenderer"
		"SDL_GPU"
	)
	if(NOT RMLUI_BACKEND IN_LIST RMLUI_BACKEND_SIMULATE_TOUCH_SUPPORTED)
		message(WARNING "RMLUI_BACKEND_SIMULATE_TOUCH enabled but not supported on selected backend ${RMLUI_BACKEND}")
	else()
		target_compile_definitions(rmlui_backend_common_headers INTERFACE "RMLUI_BACKEND_SIMULATE_TOUCH")
	endif()
endif()

add_library(rmlui_backend_Win32_GL2 INTERFACE)
target_sources(rmlui_backend_Win32_GL2 INTERFACE
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Platform_Win32.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Renderer_GL2.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Backend_Win32_GL2.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Platform_Win32.h"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Renderer_GL2.h"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Include_Windows.h"
)
target_link_libraries(rmlui_backend_Win32_GL2 INTERFACE rmlui_backend_common_headers OpenGL::GL)

add_library(rmlui_backend_Win32_VK INTERFACE)
target_sources(rmlui_backend_Win32_VK INTERFACE
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Platform_Win32.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Renderer_VK.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Backend_Win32_VK.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Platform_Win32.h"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Renderer_VK.h"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Include_Vulkan.h"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Vulkan/ShadersCompiledSPV.h"
)
target_link_libraries(rmlui_backend_Win32_VK INTERFACE rmlui_backend_common_headers)

add_library(rmlui_backend_X11_GL2 INTERFACE)
target_sources(rmlui_backend_X11_GL2 INTERFACE
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Platform_X11.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Renderer_GL2.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Backend_X11_GL2.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Platform_X11.h"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Renderer_GL2.h"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Include_Xlib.h"
)

# RMLUI_CMAKE_MINIMUM_VERSION_RAISE_NOTICE:
# Once the minimum CMake version is CMake >= 3.14, "${X11_LIBRARIES}" should
# be substituted by "X11:X11" in addition to any of the other imported that might
# be required. More info:
# https://cmake.org/cmake/help/latest/module/FindX11.html
target_link_libraries(rmlui_backend_X11_GL2 INTERFACE rmlui_backend_common_headers OpenGL::GL ${X11_LIBRARIES})

add_library(rmlui_backend_SDL_GL2 INTERFACE)
target_sources(rmlui_backend_SDL_GL2 INTERFACE
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Platform_SDL.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Renderer_GL2.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Backend_SDL_GL2.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Platform_SDL.h"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Renderer_GL2.h"
)
target_link_libraries(rmlui_backend_SDL_GL2 INTERFACE rmlui_backend_common_headers OpenGL::GL SDL::SDL SDL_image::SDL_image)

add_library(rmlui_backend_SDL_GL3 INTERFACE)
target_sources(rmlui_backend_SDL_GL3 INTERFACE
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Platform_SDL.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Renderer_GL3.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Backend_SDL_GL3.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Platform_SDL.h"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Renderer_GL3.h"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Include_GL3.h"
)
target_link_libraries(rmlui_backend_SDL_GL3 INTERFACE rmlui_backend_common_headers SDL::SDL SDL_image::SDL_image)
if(UNIX)
	# The OpenGL 3 renderer implementation uses dlopen/dlclose
	# This is required in some UNIX and UNIX-like operating systems to load shared object files at runtime
	target_link_libraries(rmlui_backend_SDL_GL3 INTERFACE ${CMAKE_DL_LIBS})
endif()
if(EMSCRIPTEN)
	# Only Emscripten requires linking to OpenGL::GL, for other platforms we use 'glad' as an OpenGL loader.
	target_link_libraries(rmlui_backend_SDL_GL3 INTERFACE OpenGL::GL)
endif()

add_library(rmlui_backend_SDL_VK INTERFACE)
target_sources(rmlui_backend_SDL_VK INTERFACE
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Platform_SDL.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Renderer_VK.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Backend_SDL_VK.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Platform_SDL.h"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Renderer_VK.h"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Include_Vulkan.h"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Vulkan/ShadersCompiledSPV.h"
)
target_link_libraries(rmlui_backend_SDL_VK INTERFACE rmlui_backend_common_headers SDL::SDL)
if(UNIX)
	# The Vulkan renderer implementation uses dlopen/dlclose
	# This is required in some UNIX and UNIX-like operating systems to load shared object files at runtime
	target_link_libraries(rmlui_backend_SDL_VK INTERFACE ${CMAKE_DL_LIBS})
endif()

add_library(rmlui_backend_SDL_SDLrenderer INTERFACE)
target_sources(rmlui_backend_SDL_SDLrenderer INTERFACE
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Renderer_SDL.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Platform_SDL.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Backend_SDL_SDLrenderer.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Platform_SDL.h"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Renderer_SDL.h"
)
target_link_libraries(rmlui_backend_SDL_SDLrenderer INTERFACE rmlui_backend_common_headers SDL::SDL SDL_image::SDL_image)

add_library(rmlui_backend_SDL_GPU INTERFACE)
target_sources(rmlui_backend_SDL_GPU INTERFACE
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Renderer_SDL_GPU.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Platform_SDL.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Backend_SDL_GPU.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Platform_SDL.h"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Renderer_SDL_GPU.h"
)
target_link_libraries(rmlui_backend_SDL_GPU INTERFACE rmlui_backend_common_headers SDL::SDL SDL_image::SDL_image)

add_library(rmlui_backend_SFML_GL2 INTERFACE)
target_sources(rmlui_backend_SFML_GL2 INTERFACE
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Platform_SFML.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Renderer_GL2.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Backend_SFML_GL2.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Platform_SFML.h"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Renderer_GL2.h"
)
target_link_libraries(rmlui_backend_SFML_GL2 INTERFACE
	rmlui_backend_common_headers OpenGL::GL SFML::Graphics SFML::Window SFML::System
)

add_library(rmlui_backend_GLFW_GL2 INTERFACE)
target_sources(rmlui_backend_GLFW_GL2 INTERFACE
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Platform_GLFW.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Renderer_GL2.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Backend_GLFW_GL2.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Platform_GLFW.h"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Renderer_GL2.h"
)
target_link_libraries(rmlui_backend_GLFW_GL2 INTERFACE rmlui_backend_common_headers OpenGL::GL glfw)

add_library(rmlui_backend_GLFW_GL3 INTERFACE)
target_sources(rmlui_backend_GLFW_GL3 INTERFACE
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Platform_GLFW.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Renderer_GL3.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Backend_GLFW_GL3.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Platform_GLFW.h"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Renderer_GL3.h"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Include_GL3.h"
)
target_link_libraries(rmlui_backend_GLFW_GL3 INTERFACE rmlui_backend_common_headers glfw)
if(UNIX)
	# The OpenGL 3 renderer implementation uses dlopen/dlclose
	# This is required in some UNIX and UNIX-like operating systems to load shared object files at runtime
	target_link_libraries(rmlui_backend_GLFW_GL3 INTERFACE ${CMAKE_DL_LIBS})
endif()

add_library(rmlui_backend_GLFW_VK INTERFACE)
target_sources(rmlui_backend_GLFW_VK INTERFACE
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Platform_GLFW.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Renderer_VK.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Backend_GLFW_VK.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Platform_GLFW.h"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Renderer_VK.h"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Include_Vulkan.h"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Vulkan/ShadersCompiledSPV.h"
)
target_link_libraries(rmlui_backend_GLFW_VK INTERFACE rmlui_backend_common_headers glfw)
if(UNIX)
	# The Vulkan renderer implementation uses dlopen/dlclose
	# This is required in some UNIX and UNIX-like operating systems to load shared object files at runtime
	target_link_libraries(rmlui_backend_GLFW_VK INTERFACE ${CMAKE_DL_LIBS})
endif()

add_library(rmlui_backend_BackwardCompatible_GLFW_GL2 INTERFACE)
target_sources(rmlui_backend_BackwardCompatible_GLFW_GL2 INTERFACE
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Platform_GLFW.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Platform_GLFW.h"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_BackwardCompatible/RmlUi_Renderer_BackwardCompatible_GL2.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_BackwardCompatible/RmlUi_Renderer_BackwardCompatible_GL2.h"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_BackwardCompatible/RmlUi_Backend_BackwardCompatible_GLFW_GL2.cpp"
)
target_link_libraries(rmlui_backend_BackwardCompatible_GLFW_GL2 INTERFACE rmlui_backend_common_headers OpenGL::GL glfw)

add_library(rmlui_backend_BackwardCompatible_GLFW_GL3 INTERFACE)
target_sources(rmlui_backend_BackwardCompatible_GLFW_GL3 INTERFACE
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Platform_GLFW.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_Platform_GLFW.h"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_BackwardCompatible/RmlUi_Renderer_BackwardCompatible_GL3.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_BackwardCompatible/RmlUi_Renderer_BackwardCompatible_GL3.h"
	"${CMAKE_CURRENT_LIST_DIR}/RmlUi_BackwardCompatible/RmlUi_Backend_BackwardCompatible_GLFW_GL3.cpp"
)
target_link_libraries(rmlui_backend_BackwardCompatible_GLFW_GL3 INTERFACE rmlui_backend_common_headers glfw)
if(UNIX)
	# The OpenGL 3 renderer implementation uses dlopen/dlclose
	# This is required in some UNIX and UNIX-like operating systems to load shared object files at runtime
	target_link_libraries(rmlui_backend_BackwardCompatible_GLFW_GL3 INTERFACE ${CMAKE_DL_LIBS})
endif()

if(RMLUI_IS_ROOT_PROJECT)
	install(DIRECTORY "./"
		DESTINATION "${CMAKE_INSTALL_DATADIR}/Backends"
	)
endif()
