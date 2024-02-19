add_executable(rmlui_sample_lottie WIN32
    src/main.cpp
)

target_link_libraries(rmlui_sample_lottie PRIVATE rmlui_sample_shell)

install(TARGETS rmlui_sample_lottie
	${RMLUI_RUNTIME_DEPENDENCY_SET_ARG}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY data
	DESTINATION samples/basic/lottie
)
