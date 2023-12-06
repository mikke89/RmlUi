# Add CMake subdirectories of basic samples
add_subdirectory("bitmapfont")

# If a font interface has been selected, add CMake subdirectories of samples that require it
if(NOT (RMLUI_FONT_INTERFACE STREQUAL "none"))
    add_subdirectory("benchmark")
    add_subdirectory("demo")

    # Negotiate inclusion of Lottie sample
    if(RMLUI_LOTTIE_PLUGIN)
        add_subdirectory("lottie")
    else()
        message(NOTICE "Lottie sample disabled due to RMLUI_LOTTIE_PLUGIN=OFF")
    endif()
endif()
