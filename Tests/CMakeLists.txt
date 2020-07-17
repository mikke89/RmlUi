#===================================
# Include doctest ==================
#===================================
set(DOCTEST_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/doctest)

add_library(doctest::doctest IMPORTED INTERFACE)
set_property(TARGET doctest::doctest PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${DOCTEST_INCLUDE_DIR}")

#===================================
# Create binary for tests ==========
#===================================
file(GLOB RmlTest_SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp)
add_executable(Tests ${RmlTest_SRC_FILES})
target_link_libraries(Tests doctest::doctest RmlCore)

set_target_properties(Tests PROPERTIES CXX_STANDARD 14)

# Enable compiler warnings
if (NOT TEST_INSTALLED_VERSION)
  if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(RmlCore PUBLIC -Wall -pedantic -Wextra -Werror)
  elseif(MSVC)
    target_compile_options(RmlCore PUBLIC /W4 /WX /permissive-)
    target_compile_definitions(Tests PUBLIC DOCTEST_CONFIG_USE_STD_HEADERS)
  endif()
endif()

# Add tests using doctest's discovery module
include(${DOCTEST_INCLUDE_DIR}/cmake/doctest.cmake)
doctest_discover_tests(Tests)
