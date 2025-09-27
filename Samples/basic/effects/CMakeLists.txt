# add new backend just using |XX like |DX11 will be not ignored DX11 backend and effects will be added to samples
if(NOT RMLUI_BACKEND MATCHES ".*(GL3|DX12).*")
	message(STATUS "Effects sample disabled - only available on GL3 backends")
	return()
endif()

set(SAMPLE_NAME "effects")
set(TARGET_NAME "${RMLUI_SAMPLE_PREFIX}${SAMPLE_NAME}")

add_executable(${TARGET_NAME} WIN32
	src/main.cpp
)

set_common_target_options(${TARGET_NAME})

target_link_libraries(${TARGET_NAME} PRIVATE rmlui_shell)

install_sample_target(${TARGET_NAME})
