#===================================
# RmlUi tests definitions ==========
#===================================
target_compile_definitions(RmlCore PUBLIC RMLUI_TESTS_ENABLED)


#===================================
# Include dependencies =============
#===================================
function(include_dependency NAME)
	set(DEPENDENCY_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/${NAME})
	set(DEPENDENCY_TARGET ${NAME}::${NAME})

	add_library(${DEPENDENCY_TARGET} IMPORTED INTERFACE)
	set_property(TARGET ${DEPENDENCY_TARGET} PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${DEPENDENCY_INCLUDE_DIR}")
endfunction()

include_dependency("doctest")
# Include doctest's discovery module
include(${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/doctest/cmake/doctest.cmake)

include_dependency("nanobench")
include_dependency("lodepng")
include_dependency("trompeloeil")

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
target_link_libraries(UnitTests RmlCore doctest::doctest trompeloeil::trompeloeil ${sample_LIBRARIES})
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
