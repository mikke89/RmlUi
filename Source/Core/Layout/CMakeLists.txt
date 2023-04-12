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
add_library(rmlui_core_layout INTERFACE)

# Set C++14 as requirement
target_compile_features(rmlui_core_layout INTERFACE "cxx_std_14")

# RMLUI_CMAKE_MINIMUM_VERSION_RAISE_NOTICE:
# Using absolute paths to prevent improper interpretation of relative paths
# Relative paths can be used once the minimum CMake version is greater or
# equal than CMake 3.13
target_sources(rmlui_core_layout INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/BlockContainer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/BlockContainer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/BlockFormattingContext.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/BlockFormattingContext.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ContainerBox.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ContainerBox.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FlexFormattingContext.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FlexFormattingContext.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FloatedBoxSpace.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FloatedBoxSpace.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FormattingContext.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FormattingContext.h
    ${CMAKE_CURRENT_SOURCE_DIR}/InlineBox.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/InlineBox.h
    ${CMAKE_CURRENT_SOURCE_DIR}/InlineContainer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/InlineContainer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/InlineLevelBox.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/InlineLevelBox.h
    ${CMAKE_CURRENT_SOURCE_DIR}/InlineTypes.h
    ${CMAKE_CURRENT_SOURCE_DIR}/LayoutBox.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/LayoutBox.h
    ${CMAKE_CURRENT_SOURCE_DIR}/LayoutDetails.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/LayoutDetails.h
    ${CMAKE_CURRENT_SOURCE_DIR}/LayoutEngine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/LayoutEngine.h
    ${CMAKE_CURRENT_SOURCE_DIR}/LayoutPools.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/LayoutPools.h
    ${CMAKE_CURRENT_SOURCE_DIR}/LineBox.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/LineBox.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ReplacedFormattingContext.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ReplacedFormattingContext.h
    ${CMAKE_CURRENT_SOURCE_DIR}/TableFormattingContext.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TableFormattingContext.h
    ${CMAKE_CURRENT_SOURCE_DIR}/TableFormattingDetails.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TableFormattingDetails.h
)
