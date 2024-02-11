add_executable("${RMLUI_SAMPLES_PREFIX}customlog" WIN32
    src/main.cpp
    src/SystemInterface.cpp
    src/SystemInterface.h
)

target_link_libraries("${RMLUI_SAMPLES_PREFIX}customlog" PRIVATE rmlui_sample_shell)
