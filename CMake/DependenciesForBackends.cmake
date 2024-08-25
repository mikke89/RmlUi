#[[
	Set up external dependencies required by the built-in backends.

	All dependencies are searched as soft dependencies so that they won't error out if the library is declared by other
	means. This file is not meant to be used by consumers of the library, only by the RmlUi CMake project.
]]

# --- Window/input APIs ---
# SDL
if(RMLUI_BACKEND MATCHES "^SDL")
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
		report_dependency_not_found("SDL2" SDL2::SDL2)
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

	# SDL_GL2 backend requires GLEW
	if(RMLUI_BACKEND STREQUAL "SDL_GL2" AND NOT TARGET GLEW::GLEW)
		find_package(GLEW)
		if(NOT TARGET GLEW::GLEW)
			report_dependency_not_found("GLEW" GLEW::GLEW)
		endif()
	endif()

	# Check version requirement for the SDL renderer
	if(RMLUI_BACKEND STREQUAL "SDL_SDLrenderer" AND SDL2_VERSION VERSION_LESS "2.0.20")
		message(FATAL_ERROR "SDL native renderer backend (${RMLUI_BACKEND}) requires SDL 2.0.20 (found ${SDL2_VERSION}).")
	endif()

	# List of SDL backends that require SDL_image to work with samples
	set(RMLUI_SDL_BACKENDS_WITH_SDLIMAGE "SDL_GL2" "SDL_GL3" "SDL_SDLrenderer")

	# Determine if the selected SDL backend requires SDL_image
	if(RMLUI_BACKEND IN_LIST RMLUI_SDL_BACKENDS_WITH_SDLIMAGE)
		set(RMLUI_SDLIMAGE_REQUIRED TRUE)
	else()
		set(RMLUI_SDLIMAGE_REQUIRED FALSE)
	endif()
	unset(RMLUI_SDL_BACKENDS_WITH_SDLIMAGE)

	# Require SDL_image if needed
	if(RMLUI_SDLIMAGE_REQUIRED)
		find_package("SDL2_image")
		report_dependency_found_or_error("SDL2_image" SDL2_image::SDL2_image)
	endif()
endif()

# GLFW
if(RMLUI_BACKEND MATCHES "^(BackwardCompatible_)?GLFW")
	find_package("glfw3" "3.3")

	# Instead of relying on the <package_name>_FOUND variable, we check directly for the target
	if(NOT TARGET glfw)
		report_dependency_found_or_error("GLFW" glfw)
	endif()
endif()

# SFML
if(RMLUI_BACKEND MATCHES "^SFML")
	#[[
		Starting with SFML 2.7, the recommended method to find the library is using
		the official config file which sets up targets for each module of the library.
		The names of the targets follow the namespaced target names convention.

		When one of the required modules isn't present as a SFML::<module> target,
		that means SFML < 2.7 is being used and we need to set up the target(s) by ourselves.

		In SFML 2.5 the first iteration of the SFMLConfig.cmake file was introduced, which
		uses a target-oriented approach to exposing the different modules of SFML but it doesn't
		use the same names as the config file from SFML 2.7.
	]]

	# List of required components in capital case
	set(RMLUI_SFML_REQUIRED_COMPONENTS "Graphics" "Window" "System")

	# Run find package with component names both capitalized and lower-cased
	find_package("SFML" "2" COMPONENTS ${RMLUI_SFML_REQUIRED_COMPONENTS} QUIET)
	if(NOT SFML_FOUND)
		list(TRANSFORM RMLUI_SFML_REQUIRED_COMPONENTS TOLOWER OUTPUT_VARIABLE RMLUI_SFML_REQUIRED_COMPONENTS_LOWER_CASE)
		find_package("SFML" "2" COMPONENTS ${RMLUI_SFML_REQUIRED_COMPONENTS_LOWER_CASE} QUIET)
	endif()

	# Since we are using find_package() in basic mode, we can check the _FOUND variable
	if(NOT SFML_FOUND)
		report_dependency_not_found("SFML" SFML::SFML)
	endif()

	#[[
		Since the RmlUi CMake project uses the SFML 2.7 namespaced target names, if the version is lower wrappers
		need to be set up.

		Because we always require the window module, we can use it to determine which iteration of the config.
	]]

	# If any of the mandatory SFML 2.7 targets isn't present, assume SFML < 2.7 has been found and set up wrappers
	if(NOT TARGET SFML::Window)
		#[[
			If sfml-window exists, then that means the version is either SFML 2.5 or 2.6 which set up
			module-specific CMake targets but with different names using a config file.

			Therefore, we need to alias the target names to match those declared by SFML 2.7 and used by RmlUi.
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
	report_dependency_found_or_error("OpenGL" OpenGL::GL)
endif()

if(RMLUI_BACKEND MATCHES "GL3$")
	find_package("OpenGL" "3")
	report_dependency_found_or_error("OpenGL" OpenGL::GL)
endif()
