function(backends_auto_selection_error)
	message(FATAL_ERROR
		"RmlUi ${RMLUI_SAMPLES_BACKEND} backend for ${CMAKE_SYSTEM_NAME} is not available. "
		"Please select a backend that is available on the system. "
		"Valid options for RMLUI_SAMPLES_BACKEND are: ${RMLUI_AVAILABLE_SAMPLES_BACKENDS}"
	)
endfunction()

if(RMLUI_SAMPLES_BACKEND STREQUAL "auto")
    if(EMSCRIPTEN)
        set(RMLUI_SAMPLES_BACKEND SDL_GL3)
    elseif(WIN32)
		set(RMLUI_SAMPLES_BACKEND GLFW_GL3)
    elseif(APPLE)
        set(RMLUI_SAMPLES_BACKEND SDL_SDLrenderer)
	elseif(UNIX)
		set(RMLUI_SAMPLES_BACKEND X11_GL2)
	else()
		backends_auto_selection_error()
	endif()
elseif(RMLUI_SAMPLES_BACKEND STREQUAL "native")
	if(EMSCRIPTEN)
		set(RMLUI_SAMPLES_BACKEND SDL_GL3)
	elseif(WIN32)
		set(RMLUI_SAMPLES_BACKEND WIN32_GL2)
	elseif(UNIX AND NOT APPLE)
        set(RMLUI_SAMPLES_BACKEND X11_GL2)
	else()
		backends_auto_selection_error()
    endif()
endif()

message(STATUS "Using RmlUi backend ${RMLUI_SAMPLES_BACKEND} for samples")
