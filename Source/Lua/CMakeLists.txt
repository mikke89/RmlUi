# Placeholder CMake file

# Add public headers as files in the project (it's not necessary but convenient for IDE integration)
target_sources(rmlui_lua INTERFACE
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Lua/Header.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Lua/IncludeLua.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Lua/Interpreter.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Lua/Lua.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Lua/LuaType.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Lua/LuaType.inl
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Lua/Utilities.h
    ${PROJECT_SOURCE_DIR}/Include/RmlUi/Lua.h
)
