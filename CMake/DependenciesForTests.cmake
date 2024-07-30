#[[
	Set up dependencies for the RmlUi tests. The dependencies are all built-in header-only libraries.
]]
function(add_builtin_header_only_tests_dependency NAME)
	set(DEPENDENCY_PATH "${PROJECT_SOURCE_DIR}/Tests/Dependencies/${NAME}")
	set(DEPENDENCY_TARGET "${NAME}::${NAME}")
	add_library(${DEPENDENCY_TARGET} IMPORTED INTERFACE)
	set_property(TARGET ${DEPENDENCY_TARGET} PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${DEPENDENCY_PATH}")
endfunction()

add_builtin_header_only_tests_dependency("doctest")
add_builtin_header_only_tests_dependency("nanobench")
add_builtin_header_only_tests_dependency("lodepng")
add_builtin_header_only_tests_dependency("trompeloeil")

# Include doctest's discovery module
include("${PROJECT_SOURCE_DIR}/Tests/Dependencies/doctest/cmake/doctest.cmake")

if(MSVC)
	target_compile_definitions(doctest::doctest INTERFACE DOCTEST_CONFIG_USE_STD_HEADERS)
endif()
if(NOT RMLUI_RTTI_AND_EXCEPTIONS)
	target_compile_definitions(doctest::doctest INTERFACE DOCTEST_CONFIG_NO_EXCEPTIONS_BUT_WITH_ALL_ASSERTS)
endif()
