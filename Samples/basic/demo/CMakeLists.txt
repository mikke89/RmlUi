add_executable(rmlui_sample_demo WIN32
    src/main.cpp
)

set_target_properties(rmlui_sample_demo PROPERTIES OUTPUT_NAME demo)

target_link_libraries(rmlui_sample_demo PRIVATE rmlui_sample_shell)
