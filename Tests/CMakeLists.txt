#===================================
# RmlUi tests definitions ==========
#===================================
target_compile_definitions(RmlCore PUBLIC RMLUI_TESTS_ENABLED)


#===================================
# Include dependencies =============
#===================================
set(DOCTEST_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/doctest)
add_library(doctest::doctest IMPORTED INTERFACE)
set_property(TARGET doctest::doctest PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${DOCTEST_INCLUDE_DIR}")

# Include doctest's discovery module
include(${DOCTEST_INCLUDE_DIR}/cmake/doctest.cmake)

set(NANOBENCH_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/nanobench)
add_library(nanobench::nanobench IMPORTED INTERFACE)
set_property(TARGET nanobench::nanobench PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${NANOBENCH_INCLUDE_DIR}")

set(LODEPNG_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/lodepng)
add_library(lodepng::lodepng IMPORTED INTERFACE)
set_property(TARGET lodepng::lodepng PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${LODEPNG_INCLUDE_DIR}")

#===================================
# Common source files ==============
#===================================

file(GLOB TestsCommon_HDR_FILES ${CMAKE_CURRENT_SOURCE_DIR}/Source/Common/*.h )
file(GLOB TestsCommon_SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/Source/Common/*.cpp )

#===================================
# Unit Tests =======================
#===================================

file(GLOB UnitTests_HDR_FILES ${CMAKE_CURRENT_SOURCE_DIR}/Source/UnitTests/*.h )
file(GLOB UnitTests_SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/Source/UnitTests/*.cpp )

add_executable(UnitTests ${UnitTests_HDR_FILES} ${UnitTests_SRC_FILES})
target_link_libraries(UnitTests RmlCore doctest::doctest ${sample_LIBRARIES})
add_common_target_options(UnitTests)

if(MSVC)
	target_compile_definitions(UnitTests PUBLIC DOCTEST_CONFIG_USE_STD_HEADERS)
endif()

doctest_discover_tests(UnitTests)



#===================================
# Visual Tests =====================
#===================================

file(GLOB VisualTests_HDR_FILES ${CMAKE_CURRENT_SOURCE_DIR}/Source/VisualTests/*.h )
file(GLOB VisualTests_SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/Source/VisualTests/*.cpp )

if(WIN32)
	add_executable(VisualTests WIN32 ${VisualTests_HDR_FILES} ${VisualTests_SRC_FILES})
else()
	add_executable(VisualTests ${VisualTests_HDR_FILES} ${VisualTests_SRC_FILES})
endif()

target_link_libraries(VisualTests RmlCore RmlDebugger lodepng::lodepng ${sample_LIBRARIES})
add_common_target_options(VisualTests)

if(VISUAL_TESTS_RML_DIRECTORIES)
	target_compile_definitions(VisualTests PRIVATE RMLUI_VISUAL_TESTS_RML_DIRECTORIES="${VISUAL_TESTS_RML_DIRECTORIES}")
endif()
if(VISUAL_TESTS_COMPARE_DIRECTORY)
	target_compile_definitions(VisualTests PRIVATE RMLUI_VISUAL_TESTS_COMPARE_DIRECTORY="${VISUAL_TESTS_COMPARE_DIRECTORY}")
endif()
if(VISUAL_TESTS_CAPTURE_DIRECTORY)
	target_compile_definitions(VisualTests PRIVATE RMLUI_VISUAL_TESTS_CAPTURE_DIRECTORY="${VISUAL_TESTS_CAPTURE_DIRECTORY}")
endif()

if(MSVC)
	target_compile_definitions(VisualTests PUBLIC DOCTEST_CONFIG_USE_STD_HEADERS)
endif()



#===================================
# Benchmarks =======================
#===================================

file(GLOB Benchmarks_HDR_FILES ${CMAKE_CURRENT_SOURCE_DIR}/Source/Benchmarks/*.h )
file(GLOB Benchmarks_SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/Source/Benchmarks/*.cpp )

add_executable(Benchmarks ${Benchmarks_HDR_FILES} ${Benchmarks_SRC_FILES} ${TestsCommon_HDR_FILES} ${TestsCommon_SRC_FILES})
target_link_libraries(Benchmarks RmlCore RmlDebugger doctest::doctest nanobench::nanobench ${sample_LIBRARIES})
add_common_target_options(Benchmarks)

if(MSVC)
	target_compile_definitions(Benchmarks PUBLIC DOCTEST_CONFIG_USE_STD_HEADERS)
endif()
