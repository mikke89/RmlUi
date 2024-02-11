add_executable("${RMLUI_SAMPLES_PREFIX}animation" WIN32
    src/main.cpp
)

target_link_libraries("${RMLUI_SAMPLES_PREFIX}animation" PRIVATE rmlui_sample_shell)
