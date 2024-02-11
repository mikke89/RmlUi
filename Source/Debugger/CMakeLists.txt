# Declare RmlUi debugger
# Not explicitly setting library type so that it can be chosen by consumer using BUILD_SHARED_LIBS. Although not
# necessary, header files are also included to improve IntelliSense capabilities on IDEs and language servers.
add_library(rmlui_debugger
    BeaconSource.h
    CommonSource.h
    Debugger.cpp
    DebuggerPlugin.cpp
    DebuggerPlugin.h
    DebuggerSystemInterface.cpp
    DebuggerSystemInterface.h
    ElementContextHook.cpp
    ElementContextHook.h
    ElementInfo.cpp
    ElementInfo.h
    ElementLog.cpp
    ElementLog.h
    FontSource.h
    Geometry.cpp
    Geometry.h
    InfoSource.h
    LogSource.h
    MenuSource.h
)

target_compile_features(rmlui_debugger PUBLIC "cxx_std_14")

target_include_directories(rmlui_debugger PRIVATE "${PROJECT_SOURCE_DIR}/Include")
target_include_directories(rmlui_debugger INTERFACE
	"$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/Include>"
	"$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

# Add public headers as files in the project (it's not necessary but convenient for IDE integration)
# Setting them as PRIVATE so that it's addition doesn't propagate, it won't affect availability since
# the entire include directory has already been declared as public
target_sources(rmlui_debugger PRIVATE
    "${PROJECT_SOURCE_DIR}/Include/RmlUi/Debugger/Debugger.h"
    "${PROJECT_SOURCE_DIR}/Include/RmlUi/Debugger/Header.h"
    "${PROJECT_SOURCE_DIR}/Include/RmlUi/Debugger.h"
)

# Link against RmlUi
target_link_libraries(rmlui_debugger PUBLIC RmlUi::RmlUi)

# Add alias for the library so that a namespaced name can be used in the rest
# of the project to link against it
add_library(RmlUi::Debugger ALIAS rmlui_debugger)

# Avoid problems with MSVC's __dllspec() in static builds
get_target_property(rmlui_debugger_TYPE rmlui_debugger "TYPE")
if(rmlui_debugger_TYPE STREQUAL "SHARED_LIBRARY")
    # If RmlUi debugger is being compiled as a shared library, notify code to export functions using __dllspec()
    # For applications consuming the library, the headers will automatically use dllimport
    # Platform and compiler handling is already done by code via pre-processor macros
    target_compile_definitions(rmlui_debugger PRIVATE "RMLUI_DEBUGGER_EXPORTS")
endif()

# Set additional target properties
set_target_properties(rmlui_debugger PROPERTIES
    # Add export name so that it can be exported with a namespaced name instead
    # of using the name we actually used to declare the target
    EXPORT_NAME "Debugger"

    # Change output name of the final library file
    OUTPUT_NAME "rmlui_debugger"
)

install(TARGETS rmlui_debugger
	EXPORT RmlUiTargets
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Debugger"
	DESTINATION "Include/RmlUi"
)
install(FILES
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Debugger.h"
	DESTINATION "Include/RmlUi"
)
