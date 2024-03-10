set(RMLUI_SAMPLE_PREFIX "rmlui_sample_")

if(RMLUI_SAMPLES_BACKEND MATCHES "VK$")
    option(RMLUI_VK_DEBUG "Enable debugging mode for Vulkan renderer." OFF)
    mark_as_advanced(RMLUI_VK_DEBUG)
    if(RMLUI_VK_DEBUG)
        target_compile_definitions(rmlui_backend_${RMLUI_SAMPLES_BACKEND} INTERFACE "RMLUI_VK_DEBUG")
    endif()
endif()

if(RMLUI_SHELL)
	include("${PROJECT_SOURCE_DIR}/CMake/SamplesShellDependencies.cmake")
	add_subdirectory("shell")
endif()

if(RMLUI_SAMPLES)
	if(NOT RMLUI_FONT_ENGINE_ENABLED)
		message(NOTICE "Building samples without any font engine selected - most samples will be disabled.")
	endif()

	add_subdirectory("basic")

	if(RMLUI_FONT_ENGINE_ENABLED)
		add_subdirectory("invaders")
		add_subdirectory("tutorial")
	endif()
	if(RMLUI_FONT_ENGINE_ENABLED AND RMLUI_LUA_BINDINGS)
		add_subdirectory("luainvaders")
	endif()

	install(DIRECTORY assets
		DESTINATION "${CMAKE_INSTALL_DATADIR}/samples"
	)
endif()
