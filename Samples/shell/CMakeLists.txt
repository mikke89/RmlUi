#[[
    Note: rmlui_sample_shell is NOT a sample. It's a utility static library with code common to most if not all RmlUi samples.
]]

add_library(rmlui_sample_shell STATIC
    src/PlatformExtensions.cpp
    src/RendererExtensions.cpp
    src/Shell.cpp
    src/ShellFileInterface.cpp

    include/PlatformExtensions.h
    include/RendererExtensions.h
    include/Shell.h
    include/ShellFileInterface.h
)

target_link_libraries(rmlui_sample_shell PRIVATE rmlui_backend_${RMLUI_SAMPLES_BACKEND})

target_include_directories(rmlui_sample_shell PUBLIC "include" "${PROJECT_SOURCE_DIR}/Backends")

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_link_libraries(rmlui_sample_shell PRIVATE Windows::Shell::LightweightUtility)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    target_link_libraries(rmlui_sample_shell PRIVATE macOS::Cocoa)
endif()

target_link_libraries(rmlui_sample_shell PUBLIC RmlUi::Core RmlUi::Debugger)
