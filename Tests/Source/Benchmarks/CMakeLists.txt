set(TARGET_NAME "rmlui_benchmarks")

add_executable(${TARGET_NAME}
	DataExpression.cpp
	Element.cpp
	BackgroundBorder.cpp
	ElementDocument.cpp
	Table.cpp
	Selectors.cpp
	main.cpp
	DataBinding.cpp
	Flexbox.cpp
	FontEffect.cpp
)

set_common_target_options(${TARGET_NAME})

target_link_libraries(${TARGET_NAME} PRIVATE
	rmlui_tests_common
	rmlui_core
	doctest::doctest
	nanobench::nanobench
)

doctest_discover_tests(${TARGET_NAME})

if(EMSCRIPTEN)
	# The benchmarks additionally use data from the benchmark sample.
	target_link_libraries(${TARGET_NAME} "--preload-file \"${PROJECT_SOURCE_DIR}/Samples/basic/benchmark/data/@/Samples/basic/benchmark/data/\"")
endif()
