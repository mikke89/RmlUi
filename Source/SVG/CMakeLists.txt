add_library(rmlui_svg INTERFACE)

target_compile_features(rmlui_svg INTERFACE "cxx_std_14")

# RMLUI_CMAKE_MINIMUM_VERSION_RAISE_NOTICE:
# Here we make a separate interface library for this plugin, and later link to that in `rmlui_core`. From CMake 3.13 we
# could instead target `rmlui_core` here directly, since we are not making an actual library for this plugin. Then we
# also don't have to export this plugin as a separate target.
target_sources(rmlui_svg INTERFACE
	"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ElementSVG.cpp>"
	"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/SVGPlugin.cpp>"
	"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/SVGPlugin.h>"
	"$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/Include/RmlUi/SVG/ElementSVG.h>"
)

add_library(RmlUi::SVG ALIAS rmlui_svg)

set_target_properties(rmlui_svg PROPERTIES
	EXPORT_NAME "SVG"
)

target_link_libraries(rmlui_svg INTERFACE lunasvg::lunasvg)

target_compile_definitions(rmlui_svg INTERFACE "RMLUI_SVG_PLUGIN")

install(TARGETS rmlui_svg
	EXPORT RmlUiTargets
	${RMLUI_RUNTIME_DEPENDENCY_SET_ARG}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/SVG"
	DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/RmlUi"
)
