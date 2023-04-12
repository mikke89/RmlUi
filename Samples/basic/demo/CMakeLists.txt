# Declare executable
add_executable(rmlui_samples_demo WIN32
    src/main.cpp
)

# Link shell library
target_link_libraries(rmlui_samples_demo PRIVATE rmlui_samples_shell)
