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
	ElementDataModels.cpp
	ElementDataModels.h
	ElementDebugDocument.cpp
	ElementDebugDocument.h
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

set_common_target_options(rmlui_debugger)

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

target_link_libraries(rmlui_debugger PUBLIC rmlui_core)

add_library(RmlUi::Debugger ALIAS rmlui_debugger)

# Set up definitions to export functions and classes as appropriate
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
)

install(TARGETS rmlui_debugger
	EXPORT RmlUiTargets
	${RMLUI_RUNTIME_DEPENDENCY_SET_ARG}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install_target_pdb(rmlui_debugger)
