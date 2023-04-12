# Declare executable
add_executable(rmlui_samples_benchmark WIN32
    src/main.cpp
)

# Link shell library
target_link_libraries(rmlui_samples_benchmark PRIVATE rmlui_samples_shell)
