#[[
	Set up external dependencies required by the built-in backends.

	All dependencies are searched as soft dependencies so that they won't error out if the library is declared by other
	means. This file is not meant to be used by consumers of the library, only by the RmlUi CMake project.
]]

# --- Window/input APIs ---

# SDL 2 and 3 common setup
if(RMLUI_BACKEND MATCHES "^SDL")
	set(RMLUI_SDL_VERSION_MAJOR "" CACHE STRING "Major version of SDL to search for, or empty for automatic search.")
	mark_as_advanced(RMLUI_SDL_VERSION_MAJOR)

	# List of SDL backends that require SDL_image to work with samples
	set(RMLUI_SDL_BACKENDS_WITH_SDLIMAGE "SDL_GL2" "SDL_GL3" "SDL_SDLrenderer" "SDL_GPU")

	# Determine if the selected SDL backend requires SDL_image
	if(RMLUI_BACKEND IN_LIST RMLUI_SDL_BACKENDS_WITH_SDLIMAGE)
		set(RMLUI_SDLIMAGE_REQUIRED TRUE)
	else()
		set(RMLUI_SDLIMAGE_REQUIRED FALSE)
	endif()
	unset(RMLUI_SDL_BACKENDS_WITH_SDLIMAGE)
endif()

# SDL 3
if(RMLUI_BACKEND MATCHES "^SDL" AND (RMLUI_SDL_VERSION_MAJOR EQUAL "3" OR RMLUI_SDL_VERSION_MAJOR STREQUAL ""))
	find_package("SDL3" QUIET)

	if(NOT TARGET SDL3::SDL3 AND RMLUI_SDL_VERSION_MAJOR EQUAL "3")
		report_dependency_not_found("SDL3" "SDL3" SDL3::SDL3)
	endif()

	if(TARGET SDL3::SDL3)
		#[[
			The official target includes the major version number in the target name. However, since we want to link to
			SDL independent of its major version, we create a new version-independent target to link against.
		]]
		report_dependency_found("SDL3" SDL3::SDL3)
		add_library(SDL::SDL INTERFACE IMPORTED)
		target_link_libraries(SDL::SDL INTERFACE SDL3::SDL3)
		target_compile_definitions(SDL::SDL INTERFACE RMLUI_SDL_VERSION_MAJOR=3)

		if(RMLUI_SDLIMAGE_REQUIRED)
			find_package("SDL3_image")
			report_dependency_found_or_error("SDL3_image" "SDL3_image" SDL3_image::SDL3_image)
			add_library(SDL_image::SDL_image INTERFACE IMPORTED)
			target_link_libraries(SDL_image::SDL_image INTERFACE SDL3_image::SDL3_image)
		endif()
	endif()
endif()

# SDL 2
if(RMLUI_BACKEND MATCHES "^SDL" AND NOT TARGET SDL::SDL AND (RMLUI_SDL_VERSION_MAJOR EQUAL "2" OR RMLUI_SDL_VERSION_MAJOR STREQUAL ""))
	# Although the official CMake find module is called FindSDL.cmake, the official config module provided by the SDL
	# package for its version 2 is called SDL2Config.cmake. Following this trend, the official SDL config files change
	# their name according to their major version number
	find_package("SDL2")

	#[[
		Current code operates using a hybrid mode by detecting either the variable or the target due to the possibility
		of package managers such as Conan and vcpkg of setting up SDL in their own way but always following the target
		naming conventions of the official SDL config files.
	]]

	if(NOT TARGET SDL2::SDL2 AND NOT SDL2_FOUND)
		report_dependency_not_found("SDL2" "SDL2" SDL2::SDL2)
	endif()

	# Set up the detected SDL as the SDL2::SDL2 INTERFACE target if it hasn't already been created. This is done for
	# consistent referencing across the CMake code regardless of the CMake version used.
	if(NOT TARGET SDL2::SDL2)
		add_library(SDL2::SDL2 INTERFACE IMPORTED)
		set_target_properties(SDL2::SDL2 PROPERTIES
			INTERFACE_LINK_LIBRARIES "${SDL2_LIBRARIES}"
			INTERFACE_INCLUDE_DIRECTORIES "${SDL2_INCLUDE_DIRS}"
		)
	endif()
	report_dependency_found("SDL2" SDL2::SDL2)

	add_library(SDL::SDL INTERFACE IMPORTED)
	target_link_libraries(SDL::SDL INTERFACE SDL2::SDL2)
	target_compile_definitions(SDL::SDL INTERFACE RMLUI_SDL_VERSION_MAJOR=2)

	# Check version requirement for the SDL renderer
	if(RMLUI_BACKEND STREQUAL "SDL_SDLrenderer" AND SDL2_VERSION VERSION_LESS "2.0.20")
		message(FATAL_ERROR "SDL native renderer backend (${RMLUI_BACKEND}) requires SDL 2.0.20 (found ${SDL2_VERSION}).")
	endif()

	if(RMLUI_BACKEND STREQUAL "SDL_GPU")
		message(FATAL_ERROR "SDL GPU backend (${RMLUI_BACKEND}) requires SDL3 (found ${SDL2_VERSION}).")
	endif()

	if(RMLUI_SDLIMAGE_REQUIRED)
		find_package("SDL2_image")
		report_dependency_found_or_error("SDL2_image" "SDL2_image" SDL2_image::SDL2_image)
		add_library(SDL_image::SDL_image INTERFACE IMPORTED)
		target_link_libraries(SDL_image::SDL_image INTERFACE SDL2_image::SDL2_image)
	endif()
endif()

if(RMLUI_BACKEND MATCHES "^SDL" AND NOT TARGET SDL::SDL)
	message(FATAL_ERROR "SDL version ${RMLUI_SDL_VERSION_MAJOR} is not supported.")
endif()

# GLFW
if(RMLUI_BACKEND MATCHES "^(BackwardCompatible_)?GLFW")
	find_package("glfw3" "3.3")

	# Instead of relying on the <package_name>_FOUND variable, we check directly for the target
	report_dependency_found_or_error("GLFW" "glfw3" glfw)
endif()

# SFML
if(RMLUI_BACKEND MATCHES "^SFML")
	set(RMLUI_SFML_VERSION_MAJOR "" CACHE STRING "Major version of SFML to search for, or empty for automatic search.")
	mark_as_advanced(RMLUI_SFML_VERSION_MAJOR)

	#[[
		Starting with SFML 3.0, the recommended method to find the library is using
		the official config file which sets up targets for each module of the library.
		The names of the targets follow the namespaced target names convention.

		When one of the required modules isn't present as a SFML::<module> target,
		that means SFML < 3.0 is being used and we need to set up the target(s) by ourselves.

		In SFML 2.5 the first iteration of the SFMLConfig.cmake file was introduced, which
		uses a target-oriented approach to exposing the different modules of SFML but it doesn't
		use the same names as the config file from SFML 3.0.
	]]

	# List of required components in capital case
	set(RMLUI_SFML_REQUIRED_COMPONENTS "Graphics" "Window" "System")

	# Look for SFML 3 first. We always require the window module, so use that to test if the dependency has been found.
	if(NOT TARGET SFML::Window AND (RMLUI_SFML_VERSION_MAJOR EQUAL "3" OR RMLUI_SFML_VERSION_MAJOR STREQUAL ""))
		find_package("SFML" "3" COMPONENTS ${RMLUI_SFML_REQUIRED_COMPONENTS} QUIET)
	endif()

	# Look for SFML 2 next unless another version is found.
	if(NOT TARGET SFML::Window AND (RMLUI_SFML_VERSION_MAJOR EQUAL "2" OR RMLUI_SFML_VERSION_MAJOR STREQUAL ""))
		list(TRANSFORM RMLUI_SFML_REQUIRED_COMPONENTS TOLOWER OUTPUT_VARIABLE RMLUI_SFML_REQUIRED_COMPONENTS_LOWER_CASE)
		find_package("SFML" "2" COMPONENTS ${RMLUI_SFML_REQUIRED_COMPONENTS_LOWER_CASE} QUIET)
	endif()

	if(NOT TARGET SFML::Window)
		#[[
			Since the RmlUi CMake project uses the SFML 3.0 namespaced target names, if the version is lower then wrappers
			need to be set up.

			If e.g. sfml-window exists, then that means the version is either SFML 2.5 or 2.6 which set up
			module-specific CMake targets but with different names using a config file. Therefore, we need to alias the
			target names to match those declared by SFML 3.0 and used by RmlUi.
		]]

		# For each SFML component the project requires
		foreach(rmlui_sfml_component ${RMLUI_SFML_REQUIRED_COMPONENTS})
			# Make the component name lowercase
			string(TOLOWER ${rmlui_sfml_component} rmlui_sfml_component_lower)

			if(TARGET sfml-${rmlui_sfml_component_lower})
				#[[
					RMLUI_CMAKE_MINIMUM_VERSION_RAISE_NOTICE:
					Because the target CMake version is 3.10, we can't alias non-global targets nor global imported targets.

					Promoting an imported target to the global scope without it being necessary can cause undesired behavior,
					specially when the project is consumed as a subdirectory inside another CMake project, therefore is not
					recommended. Instead of that, we pseudo-alias the target creating a second INTERFACE target with alias name.
					More info: https://cmake.org/cmake/help/latest/command/add_library.html#alias-libraries
				]]

				# If the target exists, alias it
				add_library(SFML::${rmlui_sfml_component} INTERFACE IMPORTED)
				target_link_libraries(SFML::${rmlui_sfml_component} INTERFACE sfml-${rmlui_sfml_component_lower})
			endif()
		endforeach()
	endif()

	if(NOT TARGET SFML::Window)
		list(TRANSFORM RMLUI_SFML_REQUIRED_COMPONENTS PREPEND "SFML::" OUTPUT_VARIABLE RMLUI_SFML_REQUIRED_TARGETS)
		report_dependency_not_found("SFML" "SFML" "${RMLUI_SFML_REQUIRED_TARGETS}")
	endif()

	report_dependency_found("SFML" SFML)
endif()

# X11
if(RMLUI_BACKEND MATCHES "^X11")
	find_package("X11")
endif()

# --- Rendering APIs ---

# OpenGL

# Set preferred OpenGL ABI on Linux for target OpenGL::GL
# More info: https://cmake.org/cmake/help/latest/module/FindOpenGL.html#linux-specific
# RMLUI_CMAKE_MINIMUM_VERSION_RAISE_NOTICE:
# Can remove this with CMake 3.11 as this has become the default. See policy CMP0072.
set(OpenGL_GL_PREFERENCE "GLVND")

if(RMLUI_BACKEND MATCHES "GL2$")
	find_package("OpenGL" "2")
	report_dependency_found_or_error("OpenGL" "OpenGL" OpenGL::GL)
endif()

# We use 'glad' as an OpenGL loader for GL3 backends, thus we don't normally need to link to OpenGL::GL. The exception
# is for Emscripten, where we use a custom find module to provide OpenGL support.
if(EMSCRIPTEN AND RMLUI_BACKEND MATCHES "GL3$")
	find_package("OpenGL" "3")
	report_dependency_found_or_error("OpenGL" "OpenGL" OpenGL::GL)
endif()
