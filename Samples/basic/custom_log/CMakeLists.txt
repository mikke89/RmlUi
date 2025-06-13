set(SAMPLE_NAME "custom_log")
set(TARGET_NAME "${RMLUI_SAMPLE_PREFIX}${SAMPLE_NAME}")

add_executable(${TARGET_NAME} WIN32
	src/main.cpp
	src/SystemInterface.cpp
	src/SystemInterface.h
)

set_common_target_options(${TARGET_NAME})

target_link_libraries(${TARGET_NAME} PRIVATE rmlui_shell)

install_sample_target(${TARGET_NAME})
