add_library(rmlui_lua
	Colourb.cpp
	Colourb.h
	Colourf.cpp
	Colourf.h
	Context.cpp
	Context.h
	ContextDocumentsProxy.cpp
	ContextDocumentsProxy.h
	Document.cpp
	Document.h
	Element.cpp
	Element.h
	ElementAttributesProxy.cpp
	ElementAttributesProxy.h
	ElementChildNodesProxy.cpp
	ElementChildNodesProxy.h
	ElementInstancer.cpp
	ElementInstancer.h
	ElementStyleProxy.cpp
	ElementStyleProxy.h
	ElementText.cpp
	ElementText.h
	Event.cpp
	Event.h
	EventParametersProxy.cpp
	EventParametersProxy.h
	GlobalLuaFunctions.cpp
	GlobalLuaFunctions.h
	Interpreter.cpp
	Log.cpp
	Log.h
	Lua.cpp
	LuaDataModel.cpp
	LuaDataModel.h
	LuaDocument.cpp
	LuaDocument.h
	LuaDocumentElementInstancer.cpp
	LuaDocumentElementInstancer.h
	LuaElementInstancer.cpp
	LuaElementInstancer.h
	LuaEventListener.cpp
	LuaEventListener.h
	LuaEventListenerInstancer.cpp
	LuaEventListenerInstancer.h
	LuaPlugin.cpp
	LuaPlugin.h
	LuaType.cpp
	Pairs.h
	RmlUi.cpp
	RmlUi.h
	RmlUiContextsProxy.cpp
	RmlUiContextsProxy.h
	Utilities.cpp
	Vector2f.cpp
	Vector2f.h
	Vector2i.cpp
	Vector2i.h
)

target_sources(rmlui_lua PRIVATE
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Lua/Header.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Lua/IncludeLua.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Lua/Interpreter.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Lua/Lua.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Lua/LuaType.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Lua/LuaType.inl"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Lua/Utilities.h"
	"${PROJECT_SOURCE_DIR}/Include/RmlUi/Lua.h"
)

set_common_target_options(rmlui_lua)

target_include_directories(rmlui_lua PRIVATE "${PROJECT_SOURCE_DIR}/Include")
target_include_directories(rmlui_lua INTERFACE
	"$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/Include>"
	"$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

add_library(RmlUi::Lua ALIAS rmlui_lua)

target_link_libraries(rmlui_lua PUBLIC rmlui_core)

# Use public linking, as some of our public headers require Lua. However, in principle the Lua bindings can be used
# without the Lua header files, in particular `RmlUi/Lua/Lua.h` does not require Lua.
target_link_libraries(rmlui_lua PUBLIC RmlUi::External::Lua)

# Set up the necessary compile definition if Lua is compiled as C++.
if(RMLUI_LUA_BINDINGS_LIBRARY STREQUAL "lua_as_cxx")
	target_compile_definitions(rmlui_lua PUBLIC "RMLUI_LUA_AS_CXX")
endif()

# Set up definitions to export functions and classes as appropriate
get_target_property(rmlui_lua_TYPE rmlui_lua "TYPE")
if(rmlui_lua_TYPE STREQUAL "SHARED_LIBRARY")
	# If RmlUi lua is being compiled as a shared library, notify code to export functions using __dllspec()
	# For applications consuming the library, the headers will automatically use dllimport
	# Platform and compiler handling is already done by code via pre-processor macros
	target_compile_definitions(rmlui_lua PRIVATE "RMLUI_LUA_EXPORTS")
endif()

# Set additional target properties
set_target_properties(rmlui_lua PROPERTIES
	# Add export name so that it can be exported with a namespaced name instead
	# of using the name we actually used to declare the target
	EXPORT_NAME "Lua"
)

add_subdirectory("Elements")

install(TARGETS rmlui_lua
	EXPORT RmlUiTargets
	${RMLUI_RUNTIME_DEPENDENCY_SET_ARG}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install_target_pdb(rmlui_lua)
