# Quoting the source file paths isn't necessary because add_library() doesn't read
# CMake variables, just strings
add_library(rmlui_samples_shell STATIC
    src/PlatformExtensions.cpp
    src/RendererExtensions.cpp
    src/Shell.cpp
    src/ShellFileInterface.cpp
)

# Add backend
target_link_libraries(rmlui_samples_shell PRIVATE rmlui_backend_${RMLUI_SAMPLES_BACKEND})

# Add include directories
target_include_directories(rmlui_samples_shell PUBLIC "include" "${PROJECT_SOURCE_DIR}/Backends")

# Link against required libraries from Windows
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    # Required to use the functions from the shlwapi.h header
    target_link_libraries(rmlui_samples_shell PRIVATE Windows::Shell::LightweightUtility)
# Link against required libraries from macOS
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    # Required to use the functions from the Cocoa framework
    target_link_libraries(rmlui_samples_shell PRIVATE macOS::Cocoa)
endif()

# Link against RmlUi and its debugger
target_link_libraries(rmlui_samples_shell PUBLIC RmlUi::RmlUi RmlUi::Debugger)
