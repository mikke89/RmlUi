set(TUTORIAL_NAME "drag")
set(TARGET_NAME "${RMLUI_TUTORIAL_PREFIX}${TUTORIAL_NAME}")

add_executable(${TARGET_NAME} WIN32
	src/Inventory.cpp
	src/Inventory.h
	src/main.cpp
)

set_common_target_options(${TARGET_NAME})

target_link_libraries(${TARGET_NAME} PRIVATE rmlui_shell)

install_sample_target(${TARGET_NAME})
