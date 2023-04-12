# Declare library
#
# Although not necessary, header files are also included to improve IntelliSense
# capabilities on IDEs and language servers
#
# Tip: On UNIX systems you can run this on the current source folder to get the
# entire list of files in the folder copied on the files.txt file:
#
#       ls *.cpp *.h > files.txt
#
# After running it, you can copy the text in that file and paste it here as a list
# of source files
#
# Quoting the source file paths isn't necessary because list() doesn't read
# CMake variables, just strings
add_library(rmlui_core_fontenginedefault INTERFACE)

# Set C++14 as requirement
target_compile_features(rmlui_core_fontenginedefault INTERFACE "cxx_std_14")

# RMLUI_CMAKE_MINIMUM_VERSION_RAISE_NOTICE:
# Using absolute paths to prevent improper interpretation of relative paths
# Relative paths can be used once the minimum CMake version is greater or
# equal than CMake 3.13
target_sources(rmlui_core_fontenginedefault INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/FontEngineInterfaceDefault.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FontEngineInterfaceDefault.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FontFace.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FontFace.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FontFaceHandleDefault.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FontFaceHandleDefault.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FontFaceLayer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FontFaceLayer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FontFamily.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FontFamily.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FontProvider.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FontProvider.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FontTypes.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FreeTypeInterface.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FreeTypeInterface.h
)

# Link library against Freetype
target_link_libraries(rmlui_core_fontenginedefault INTERFACE Freetype::Freetype)
