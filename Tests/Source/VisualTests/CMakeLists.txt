add_executable(rmlui_visual_tests WIN32
	XmlNodeHandlers.h
	XmlNodeHandlers.cpp
	TestViewer.h
	TestViewer.cpp
	TestSuite.h
	TestConfig.h
	TestConfig.cpp
	TestNavigator.h
	main.cpp
	CaptureScreen.h
	TestNavigator.cpp
	CaptureScreen.cpp
)

target_link_libraries(rmlui_visual_tests PRIVATE
	rmlui_tests_common
	rmlui_core
	rmlui_shell
	lodepng::lodepng
)

if(VISUAL_TESTS_RML_DIRECTORIES)
	target_compile_definitions(rmlui_visual_tests PRIVATE RMLUI_VISUAL_TESTS_RML_DIRECTORIES="${VISUAL_TESTS_RML_DIRECTORIES}")
endif()
if(VISUAL_TESTS_COMPARE_DIRECTORY)
	target_compile_definitions(rmlui_visual_tests PRIVATE RMLUI_VISUAL_TESTS_COMPARE_DIRECTORY="${VISUAL_TESTS_COMPARE_DIRECTORY}")
endif()
if(VISUAL_TESTS_CAPTURE_DIRECTORY)
	target_compile_definitions(rmlui_visual_tests PRIVATE RMLUI_VISUAL_TESTS_CAPTURE_DIRECTORY="${VISUAL_TESTS_CAPTURE_DIRECTORY}")
endif()
