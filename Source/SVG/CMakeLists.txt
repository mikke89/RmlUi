add_library(rmlui_svg INTERFACE)

target_compile_features(rmlui_svg INTERFACE "cxx_std_14")

# RMLUI_CMAKE_MINIMUM_VERSION_RAISE_NOTICE:
# Using absolute paths to prevent improper interpretation of relative paths
# Relative paths can be used once the minimum CMake version is greater or
# equal than CMake 3.13
target_sources(rmlui_svg INTERFACE
	"${CMAKE_CURRENT_SOURCE_DIR}/ElementSVG.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/SVGPlugin.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/SVGPlugin.h"
)

target_sources(rmlui_svg INTERFACE
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/SVG/ElementSVG.h"
)

target_link_libraries(rmlui_svg INTERFACE lunasvg::lunasvg)

# Notify source files about the usage of the plugin
target_compile_definitions(rmlui_svg INTERFACE "RMLUI_SVG_PLUGIN")

install(DIRECTORY
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/SVG"
	DESTINATION "Include/RmlUi"
)
