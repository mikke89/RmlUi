set(SAMPLE_NAME "bitmapfont")
set(TARGET_NAME "${RMLUI_SAMPLE_PREFIX}${SAMPLE_NAME}")

add_executable(${TARGET_NAME} WIN32
	src/FontEngineBitmap.cpp
	src/FontEngineBitmap.h
	src/FontEngineInterfaceBitmap.cpp
	src/FontEngineInterfaceBitmap.h
	src/main.cpp
)

target_link_libraries(${TARGET_NAME} PRIVATE rmlui_shell)

install(TARGETS ${TARGET_NAME}
	${RMLUI_RUNTIME_DEPENDENCY_SET_ARG}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY data
	DESTINATION "samples/basic/${SAMPLE_NAME}"
)
