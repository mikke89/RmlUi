set(SAMPLE_NAME "sandbox")
set(TARGET_NAME "${RMLUI_SAMPLE_PREFIX}${SAMPLE_NAME}")

add_executable(${TARGET_NAME} WIN32
	src/main.cpp
	src/SandboxWindow.cpp
	src/SandboxWindow.h
)

set_common_target_options(${TARGET_NAME})

target_link_libraries(${TARGET_NAME} PRIVATE rmlui_shell)

install_sample_target(${TARGET_NAME})

if(MSVC)
	# Set UTF-8 on MSVC to properly encode emoji characters.
	target_compile_options(${TARGET_NAME} PRIVATE /utf-8)
endif()
