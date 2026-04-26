if(EXISTS "${CMAKE_SOURCE_DIR}/.install")
if(NOT CMAKE_TOOLCHAIN_FILE)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/.install/build/Debug/generators/conan_toolchain.cmake" CACHE FILEPATH "Toolchain file")
    else()
        set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/.install/build/Release/generators/conan_toolchain.cmake" CACHE FILEPATH "Toolchain file")
    endif()
endif()
endif()

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER   "${CCACHE_PROGRAM}" CACHE STRING "" FORCE)
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}" CACHE STRING "" FORCE)
    message(STATUS "ccache found: ${CCACHE_PROGRAM}, using it")
else()
    message(STATUS "ccache not found, not using it")
endif()

if(WIN32)
    set(CMAKE_C_COMPILER clang-cl)
    set(CMAKE_CXX_COMPILER clang-cl)
else()
    set(CMAKE_C_COMPILER clang)
    set(CMAKE_CXX_COMPILER clang++)
endif()



