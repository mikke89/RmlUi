set(SAMPLE_NAME "demo")
set(TARGET_NAME "${RMLUI_SAMPLES_PREFIX}${SAMPLE_NAME}")

add_executable(${TARGET_NAME} WIN32
    src/main.cpp
)

target_link_libraries(${TARGET_NAME} PRIVATE rmlui_sample_shell)

install(TARGETS ${TARGET_NAME}
	${RMLUI_RUNTIME_DEPENDENCY_SET_ARG}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY data
	DESTINATION "samples/basic/demo"
)
