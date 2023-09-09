# Placeholder CMake file

# Add public headers as files in the project (it's not necessary but convenient for IDE integration)
target_sources(rmlui_svg INTERFACE
    "${PROJECT_SOURCE_DIR}/Include/RmlUi/SVG/ElementSVG.h"
)
