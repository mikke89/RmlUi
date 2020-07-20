#===================================
# Include dependencies =============
#===================================
set(DOCTEST_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/doctest)

add_library(doctest::doctest IMPORTED INTERFACE)
set_property(TARGET doctest::doctest PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${DOCTEST_INCLUDE_DIR}")

set(NANOBENCH_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/nanobench)

add_library(nanobench::nanobench IMPORTED INTERFACE)
set_property(TARGET nanobench::nanobench PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${NANOBENCH_INCLUDE_DIR}")

#===================================
# Create binary for tests ==========
#===================================
file(GLOB RmlTest_SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp)
add_executable(Tests ${RmlTest_SRC_FILES})
target_link_libraries(Tests RmlCore RmlDebugger doctest::doctest nanobench::nanobench)

set_target_properties(Tests PROPERTIES CXX_STANDARD 14)

# Enable compiler warnings
add_common_target_options(Tests)

if(MSVC)
	target_compile_definitions(Tests PUBLIC DOCTEST_CONFIG_USE_STD_HEADERS)
endif()

# Add tests using doctest's discovery module
include(${DOCTEST_INCLUDE_DIR}/cmake/doctest.cmake)
doctest_discover_tests(Tests)
