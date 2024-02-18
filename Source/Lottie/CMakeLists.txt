add_library(rmlui_lottie INTERFACE)

target_compile_features(rmlui_lottie INTERFACE "cxx_std_14")

# RMLUI_CMAKE_MINIMUM_VERSION_RAISE_NOTICE:
# Here we make a separate interface library for this plugin, and later link to that in `rmlui_core`. From CMake 3.13 we
# could instead target `rmlui_core` here directly, since we are not making an actual library for this plugin. Then we
# also don't have to export this plugin as a separate target.
target_sources(rmlui_lottie INTERFACE
	"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ElementLottie.cpp>"
	"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/LottiePlugin.cpp>"
	"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/LottiePlugin.h>"
	"$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/Include/RmlUi/Lottie/ElementLottie.h>"
)

add_library(RmlUi::Lottie ALIAS rmlui_lottie)

set_target_properties(rmlui_lottie PROPERTIES
	EXPORT_NAME "Lottie"
)

target_link_libraries(rmlui_lottie INTERFACE rlottie::rlottie)

target_compile_definitions(rmlui_lottie INTERFACE "RMLUI_LOTTIE_PLUGIN")

install(TARGETS rmlui_lottie
	EXPORT RmlUiTargets
	${RMLUI_RUNTIME_DEPENDENCY_SET_ARG}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Lottie"
	DESTINATION "Include/RmlUi"
)
