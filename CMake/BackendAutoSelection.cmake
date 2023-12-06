function(backends_auto_selection_error)
	message(FATAL_ERROR
		"RmlUi ${RMLUI_BACKEND} backend for ${CMAKE_SYSTEM_NAME} is not available. "
		"Please select a backend that is available on the system. "
		"Possible options for RMLUI_BACKEND are: ${RMLUI_BACKEND_OPTIONS}"
	)
endfunction()

if(RMLUI_BACKEND STREQUAL "auto")
	if(EMSCRIPTEN)
		set(RMLUI_BACKEND SDL_GL3)
	elseif(WIN32)
		set(RMLUI_BACKEND GLFW_GL3)
	elseif(APPLE)
		set(RMLUI_BACKEND SDL_SDLrenderer)
	elseif(UNIX)
		set(RMLUI_BACKEND GLFW_GL3)
	else()
		backends_auto_selection_error()
	endif()
elseif(RMLUI_BACKEND STREQUAL "native")
	if(EMSCRIPTEN)
		set(RMLUI_BACKEND SDL_GL3)
	elseif(WIN32)
		set(RMLUI_BACKEND WIN32_GL2)
	elseif(UNIX AND NOT APPLE)
		set(RMLUI_BACKEND X11_GL2)
	else()
		backends_auto_selection_error()
	endif()
endif()

message(STATUS "Using RmlUi backend ${RMLUI_BACKEND} for samples")
