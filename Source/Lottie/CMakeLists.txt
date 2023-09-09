# Declare library
#
# Although not necessary, header files are also included to improve IntelliSense
# capabilities on IDEs and language servers
add_library(rmlui_lottie INTERFACE)

# Set C++14 as requirement
target_compile_features(rmlui_lottie INTERFACE "cxx_std_14")

# RMLUI_CMAKE_MINIMUM_VERSION_RAISE_NOTICE:
# Using absolute paths to prevent improper interpretation of relative paths
# Relative paths can be used once the minimum CMake version is greater or
# equal than CMake 3.13
target_sources(rmlui_lottie INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/ElementLottie.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/LottiePlugin.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/LottiePlugin.h"
)

# Add public headers as files in the project (it's not necessary but convenient for IDE integration)
target_sources(rmlui_lottie INTERFACE
    "${PROJECT_SOURCE_DIR}/Include/RmlUi/Lottie/ElementLottie.h"
)

# Link library against rlottie
target_link_libraries(rmlui_lottie INTERFACE rlottie::rlottie)

# Notify source files about the usage of the plugin
target_compile_definitions(rmlui_lottie INTERFACE "RMLUI_ENABLE_LOTTIE_PLUGIN")
