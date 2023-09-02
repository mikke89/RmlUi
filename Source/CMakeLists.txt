# Change the runtime output directory of all target declared in this scope
# This is needed so that in the event of building the project as a shared
# library, all DLLs required by samples will be properly found in Windows
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Add plugin directories
add_subdirectory("Lottie")

# Add main RmlUi directory
add_subdirectory("Core")

# Add RmlUi debugger directory
add_subdirectory("Debugger")
