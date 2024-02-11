add_executable("${RMLUI_SAMPLES_PREFIX}bitmapfont" WIN32
    src/FontEngineBitmap.cpp
    src/FontEngineBitmap.h
    src/FontEngineInterfaceBitmap.cpp
    src/FontEngineInterfaceBitmap.h
    src/main.cpp
)

target_link_libraries("${RMLUI_SAMPLES_PREFIX}bitmapfont" PRIVATE rmlui_sample_shell)
