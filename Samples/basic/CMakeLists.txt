# Add CMake subdirectories of samples
add_subdirectory("benchmark")
add_subdirectory("demo")

# Negotiate inclusion of Lottie sample
if(RMLUI_ENABLE_LOTTIE_PLUGIN)
    add_subdirectory("lottie")
else()
    message(NOTICE "Lottie sample disabled due to RMLUI_ENABLE_LOTTIE_PLUGIN=OFF")
endif()
