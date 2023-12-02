# Declare executable
add_executable(rmlui_sample_bitmapfont WIN32
    src/FontEngineBitmap.cpp
    src/FontEngineBitmap.h
    src/FontEngineInterfaceBitmap.cpp
    src/FontEngineInterfaceBitmap.h
    src/main.cpp
)

# Link shell library
target_link_libraries(rmlui_sample_bitmapfont PRIVATE rmlui_sample_shell)
