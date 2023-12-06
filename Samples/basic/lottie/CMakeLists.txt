# Declare executable
add_executable(rmlui_sample_lottie WIN32
    src/main.cpp
)

# Link shell library
target_link_libraries(rmlui_sample_lottie PRIVATE rmlui_sample_shell)
