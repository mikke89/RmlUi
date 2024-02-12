add_library(rmlui_lottie INTERFACE)

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

target_sources(rmlui_lottie INTERFACE
    "${PROJECT_SOURCE_DIR}/Include/RmlUi/Lottie/ElementLottie.h"
)

target_link_libraries(rmlui_lottie INTERFACE rlottie::rlottie)

# Notify source files about the usage of the plugin
target_compile_definitions(rmlui_lottie INTERFACE "RMLUI_LOTTIE_PLUGIN")

install(DIRECTORY
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Lottie"
	DESTINATION "Include/RmlUi"
)
