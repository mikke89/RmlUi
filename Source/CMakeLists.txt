# Change the runtime output directory of all target declared in this scope
# This is needed so that in the event of building the project as a shared
# library, all DLLs required by samples will be properly found on Windows.
# The set() call is guarded against pre-definitions in order to respect consumer choice
if(NOT DEFINED CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endif()

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
