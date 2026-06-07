# add new backend just using |XX like |DX11 will be not ignored DX11 backend and effects will be added to samples
if(NOT RMLUI_BACKEND MATCHES ".*(DX12).*")
	message(STATUS "Integration sample disabled - only available on DX12 backends")
	return()
endif()

set(SAMPLE_NAME "integration_as_postprocess")
set(TARGET_NAME "${RMLUI_SAMPLE_PREFIX}${SAMPLE_NAME}")

string(TOLOWER "${RMLUI_BACKEND}" BACKEND_NAMED_LOWERED)

if ("${BACKEND_NAMED_LOWERED}" STREQUAL "win32_dx12")
	set(to_include src/main_dx12_win32.cpp)
elseif ("${BACKEND_NAMED_LOWERED}" STREQUAL "sdl_dx12")
	set(to_include src/main_dx12_sdl.cpp)
elseif ("${BACKEND_NAMED_LOWERED}" STREQUAL "glfw_dx12")
	set(to_include src/main_dx12_glfw.cpp)
else()
	message(FATAL_ERROR "unsupported backend: ${BACKEND_NAMED_LOWERED} report to developers https://github.com/mikke89/RmlUi/issues")
endif()

add_executable(${TARGET_NAME} WIN32
	${to_include}
)

set_common_target_options(${TARGET_NAME})

target_link_libraries(${TARGET_NAME} PRIVATE rmlui_shell)

install_sample_target(${TARGET_NAME})
