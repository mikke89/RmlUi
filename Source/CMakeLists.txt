# Change the runtime output directory of all target declared in this scope
# This is needed so that in the event of building the project as a shared
# library, all DLLs required by samples will be properly found in Windows
# The set() call is guarded against pre-definitions in order to respect consumer choice
if(NOT DEFINED CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endif()

# Add plugin directories
if(RMLUI_ENABLE_LOTTIE_PLUGIN)
    add_subdirectory("Lottie")
endif()

# Add main RmlUi directory
add_subdirectory("Core")

# Add RmlUi debugger directory
add_subdirectory("Debugger")
