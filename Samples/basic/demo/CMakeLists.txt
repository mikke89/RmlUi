set(SAMPLE_NAME "demo")
set(TARGET_NAME "${RMLUI_SAMPLES_PREFIX}${SAMPLE_NAME}")

add_executable(${TARGET_NAME} WIN32
    src/main.cpp
)

#set_target_properties(sample_demo PROPERTIES OUTPUT_NAME demo)

target_link_libraries(${TARGET_NAME} PRIVATE rmlui_sample_shell)

install(TARGETS ${TARGET_NAME}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY data
	DESTINATION Samples/basic/demo
)
