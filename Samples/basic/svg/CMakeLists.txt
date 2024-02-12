add_executable(rmlui_sample_svg WIN32
    src/main.cpp
)

target_link_libraries(rmlui_sample_svg PRIVATE rmlui_sample_shell)

install(TARGETS rmlui_sample_svg
	${RMLUI_RUNTIME_DEPENDENCY_SET_ARG}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY data
	DESTINATION Samples/basic/svg
)
