add_executable("${RMLUI_SAMPLES_PREFIX}benchmark" WIN32
    src/main.cpp
)

target_link_libraries("${RMLUI_SAMPLES_PREFIX}benchmark" PRIVATE rmlui_sample_shell)
