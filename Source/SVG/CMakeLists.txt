# Here we make two separate interface libraries for this plugin, and later link to them in `rmlui_core`. One private and
# one public, so that dependencies are propagated to users during export, but not source files and other build details.

# RMLUI_CMAKE_MINIMUM_VERSION_RAISE_NOTICE:
# From CMake 3.13 we could instead target `rmlui_core` here directly, since we are not making a real library for this
# plugin. This way we can remove the interface libraries defined here, which should considerably simplify how we install
# targets with dependencies being automatically handled when installing `rmlui_core`. We would just need to install
# headers here.
add_library(rmlui_svg INTERFACE)
add_library(rmlui_svg_private INTERFACE)

target_compile_features(rmlui_svg_private INTERFACE "cxx_std_14")

target_sources(rmlui_svg_private INTERFACE
	"${CMAKE_CURRENT_SOURCE_DIR}/ElementSVG.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/SVGPlugin.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/SVGPlugin.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/SVG/ElementSVG.h"
)

target_compile_definitions(rmlui_svg_private INTERFACE "RMLUI_SVG_PLUGIN")

set_target_properties(rmlui_svg PROPERTIES
	EXPORT_NAME "SVG"
)

target_link_libraries(rmlui_svg INTERFACE lunasvg::lunasvg)

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
