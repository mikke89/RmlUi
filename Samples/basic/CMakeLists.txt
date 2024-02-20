set(RMLUI_SAMPLES_PREFIX "rmlui_")

# The following sample does not require any default font engine.
add_subdirectory("bitmapfont")

# Only enable the remaining samples if a default font engine is selected.
if(NOT RMLUI_FONT_ENGINE STREQUAL "none")
    add_subdirectory("benchmark")
    add_subdirectory("demo")

    if(RMLUI_LOTTIE_PLUGIN)
        add_subdirectory("lottie")
    else()
        message(STATUS "Lottie sample disabled due to RMLUI_LOTTIE_PLUGIN=OFF")
    endif()
	if(RMLUI_SVG_PLUGIN)
		add_subdirectory("svg")
	else()
		message(STATUS "SVG sample disabled due to RMLUI_SVG_PLUGIN=OFF")
	endif()
endif()
