add_executable(rmlui_benchmarks
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

target_link_libraries(rmlui_benchmarks PRIVATE
	rmlui_tests_common
	rmlui_core
	doctest::doctest
	nanobench::nanobench
)

if(EMSCRIPTEN)
	# The benchmarks additionally use data from the benchmark sample.
	target_link_libraries(rmlui_benchmarks "--preload-file \"${PROJECT_SOURCE_DIR}/Samples/basic/benchmark/data/@/Samples/basic/benchmark/data/\"")
endif()
