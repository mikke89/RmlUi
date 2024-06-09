set(TARGET_NAME "rmlui_visual_tests")

add_executable(${TARGET_NAME} WIN32
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

set_common_target_options(${TARGET_NAME})

target_link_libraries(${TARGET_NAME} PRIVATE
	rmlui_tests_common
	rmlui_core
	rmlui_shell
	lodepng::lodepng
)
