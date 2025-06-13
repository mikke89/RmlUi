#[[
	Note: rmlui_shell is NOT a sample. It's a utility static library with code common to the RmlUi samples.
]]

add_library(rmlui_shell STATIC
	src/PlatformExtensions.cpp
	src/RendererExtensions.cpp
	src/Shell.cpp
	src/ShellFileInterface.cpp

	include/PlatformExtensions.h
	include/RendererExtensions.h
	include/Shell.h
	include/ShellFileInterface.h
)

set_common_target_options(rmlui_shell)

target_link_libraries(rmlui_shell PRIVATE rmlui_backend_${RMLUI_BACKEND})

target_include_directories(rmlui_shell PUBLIC "include" "${PROJECT_SOURCE_DIR}/Backends")

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	target_link_libraries(rmlui_shell PRIVATE Windows::Shell::LightweightUtility)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	target_link_libraries(rmlui_shell PRIVATE macOS::Cocoa)
endif()

target_link_libraries(rmlui_shell PUBLIC rmlui_core rmlui_debugger)

if(RMLUI_BACKEND MATCHES "GL2$")
	target_compile_definitions(rmlui_shell PRIVATE "RMLUI_RENDERER_GL2")
endif()
if(RMLUI_BACKEND MATCHES "GL3$")
	target_compile_definitions(rmlui_shell PRIVATE "RMLUI_RENDERER_GL3")
endif()
