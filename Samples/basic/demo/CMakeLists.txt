# Declare executable
add_executable(rmlui_sample_demo WIN32
    src/main.cpp
)

# Link shell library
target_link_libraries(rmlui_sample_demo PRIVATE rmlui_sample_shell)
