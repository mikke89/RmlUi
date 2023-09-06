# Declare executable
add_executable(rmlui_samples_bitmapfont WIN32
    src/FontEngineBitmap.cpp
    src/FontEngineBitmap.h
    src/FontEngineInterfaceBitmap.cpp
    src/FontEngineInterfaceBitmap.h
    src/main.cpp
)

# Link shell library
target_link_libraries(rmlui_samples_bitmapfont PRIVATE rmlui_samples_shell)
