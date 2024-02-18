if(RMLUI_LOTTIE_PLUGIN)
    add_subdirectory("Lottie")
endif()
if(RMLUI_SVG_PLUGIN)
	add_subdirectory("SVG")
endif()

add_subdirectory("Core")

add_subdirectory("Debugger")

if(RMLUI_LUA_BINDINGS)
	add_subdirectory("Lua")
endif()
