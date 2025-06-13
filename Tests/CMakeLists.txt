# Enable some extra testing code in the core library.
target_compile_definitions(rmlui_core PUBLIC RMLUI_TESTS_ENABLED)

include("${PROJECT_SOURCE_DIR}/CMake/DependenciesForTests.cmake")

if(EMSCRIPTEN)
	# Add data for tests to emscripten targets
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -sLLD_REPORT_UNDEFINED")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --preload-file \"${CMAKE_CURRENT_SOURCE_DIR}/Data/@/Tests/Data/\"")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --preload-file \"${PROJECT_SOURCE_DIR}/Samples/basic/demo/data/@/basic/demo/data/\"")
endif()

add_subdirectory("Source")
