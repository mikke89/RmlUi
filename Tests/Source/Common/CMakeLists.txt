add_library(rmlui_tests_common STATIC
	Mocks.h
	TestsInterface.cpp
	TestsInterface.h
	TestsShell.cpp
	TestsShell.h
	TypesToString.h
)

set_common_target_options(rmlui_tests_common)

target_include_directories(rmlui_tests_common INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(rmlui_tests_common PUBLIC
	rmlui_core
	rmlui_shell
	doctest::doctest
	trompeloeil::trompeloeil
)
