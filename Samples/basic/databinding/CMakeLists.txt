add_executable("${RMLUI_SAMPLES_PREFIX}databinding" WIN32
    src/main.cpp
)

target_link_libraries("${RMLUI_SAMPLES_PREFIX}databinding" PRIVATE rmlui_sample_shell)
