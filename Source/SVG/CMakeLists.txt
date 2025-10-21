# This plugin targets rmlui_core directly and is incorporated into the core library when enabled.

target_sources(rmlui_core PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/DecoratorSVG.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/DecoratorSVG.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/ElementSVG.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/SVGCache.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/SVGCache.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/SVGPlugin.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/SVGPlugin.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/XMLNodeHandlerSVG.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/XMLNodeHandlerSVG.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/SVG/ElementSVG.h"
)

target_compile_definitions(rmlui_core PRIVATE "RMLUI_SVG_PLUGIN")
