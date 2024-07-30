# This plugin targets rmlui_core directly and is incorporated into the core library when enabled.

target_sources(rmlui_core PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/ElementLottie.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/LottiePlugin.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/LottiePlugin.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Lottie/ElementLottie.h"
)

target_compile_definitions(rmlui_core PRIVATE "RMLUI_LOTTIE_PLUGIN")
