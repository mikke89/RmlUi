#[[
    Set up of external dependencies required by the backends provided by the project to build the samples

    All dependencies are searched as soft dependencies so that they won't error out if the library
    is declared by other means

    This file is not meant to be used by consumers of the library, only by the RmlUi CMake project
]]

include("${PROJECT_SOURCE_DIR}/CMake/Utils.cmake")

# --- Window/input APIs ---
# SDL
if(RMLUI_SAMPLES_BACKEND MATCHES "^SDL")
    # Although the official CMake find module is called FindSDL.cmake, the official config module
    # provided by the SDL package for its version 2 is called SDL2Config.cmake
    # Following this trend, the official SDL config files change their name according to their major version number
    find_package("SDL2" REQUIRED)

    #[[
        Current code operates using a hybrid mode by detecting either the variable or the target due to the possibility
        of package managers such as Conan and vcpkg of setting up SDL in their own way but always following the
        target naming conventions of the official SDL config files
    ]]

    if(NOT TARGET SDL2::SDL2 AND NOT SDL2_FOUND)
        report_not_found_dependency("SDL2" SDL2::SDL2)
    endif()

    # Set up the detected SDL as the SDL2::SDL2 INTERFACE target if it hasn't already been created
    # This is done for consistent referencing across the CMake code regardless of the CMake version used
    if(NOT TARGET SDL2::SDL2)
        add_library(SDL2::SDL2 INTERFACE IMPORTED)

        # Any CMake target linking against SDL2::SDL2 will link against the SDL libraries and have
        # their include directories added to their target properties
        target_link_libraries(SDL2::SDL2 INTERFACE ${SDL2_LIBRARIES})
        target_include_directories(SDL2::SDL2 INTERFACE ${SDL2_INCLUDE_DIRS})
    endif()

    # SDL_GL2 backend requires GLEW
    if(RMLUI_SAMPLES_BACKEND STREQUAL "SDL_GL2" AND NOT TARGET GLEW::GLEW)
        find_package(GLEW)
        if(NOT TARGET GLEW::GLEW)
            report_not_found_dependency("GLEW" GLEW::GLEW)
        endif()
    endif()

    # Check version requirement for the SDL renderer
    if(RMLUI_SAMPLES_BACKEND STREQUAL "SDL_SDLrenderer" AND SDL2_VERSION VERSION_LESS "2.0.20")
        message(FATAL_ERROR "SDL native renderer backend (${RMLUI_SAMPLES_BACKEND}) requires SDL 2.0.20 (found ${SDL2_VERSION}).")
    endif()

    # List of SDL backends that require SDL_image to work with samples
    list(APPEND RMLUI_SDL_BACKENDS_WITH_SDLIMAGE "SDL_GL2" "SDL_GL3" "SDL_SDLrenderer")

    # Determine if the selected SDL backend requires SDL_image
    list(FIND RMLUI_SDL_BACKENDS_WITH_SDLIMAGE ${RMLUI_SAMPLES_BACKEND} rmlui_sdl_image_found_item_index)
    if(rmlui_sdl_image_found_item_index EQUAL "-1")
        # If the backend hasn't been found in the list, SDL_image isn't required
        set(RMLUI_SDLIMAGE_REQUIRED FALSE)
    else()
        # If the backend has been found in the list, SDL_image is required
        set(RMLUI_SDLIMAGE_REQUIRED TRUE)
    endif()
    # Clear scope
    unset(rmlui_sdl_image_found_item_index)
    unset(RMLUI_SDL_BACKENDS_WITH_SDLIMAGE)

    # Require SDL_image if needed
    if(RMLUI_SDLIMAGE_REQUIRED)
        find_package("SDL2_image")

        if(NOT TARGET SDL2_image::SDL2_image)
            report_not_found_dependency("SDL2_image" SDL2_image::SDL2_image)
        endif()
    endif()
endif()

# GLFW
if(RMLUI_SAMPLES_BACKEND MATCHES "^GLFW")
    find_package("glfw3" "3.3")

    # Instead of relying on the <package_name>_FOUND variable, we check directly for the target
    if(NOT TARGET glfw)
        report_not_found_dependency("GLFW" glfw)
    endif()
endif()

# SFML
if(RMLUI_SAMPLES_BACKEND MATCHES "^SFML")
    #[[
        Starting with SFML 2.7, the recommended method to find the library is using 
        the official config file which sets up targets for each module of the library.
        The names of the targets follow the namespaced target names convention.

        When one of the required modules isn't present as a SFML::<module> target,
        that means SFML < 2.7 is being used and we need to set up the target(s) by ourselves.

        In SFML 2.5 the first iteration of the SFMLConfig.cmake file was introduced, which
        uses a target-oriented approach to exposing the different modules of SFML but it doesn't
        use the same names as the config file from SFML 2.7.

        In SFML <= 2.4 the old official find module is still in use which uses the old variable-based
        approach.
    ]]

    # List of required components in capital case
    list(APPEND RMLUI_SFML_REQUIRED_COMPONENTS "Graphics" "Window" "System")
    if(WIN32)
        list(APPEND RMLUI_SFML_REQUIRED_COMPONENTS "Main")
    endif()

    # Run find package with component names both capitalized and lower-cased
    find_package("SFML" "2" COMPONENTS ${RMLUI_SFML_REQUIRED_COMPONENTS})
    if(NOT SFML_FOUND)
        list(TRANSFORM RMLUI_SFML_REQUIRED_COMPONENTS TOLOWER OUTPUT_VARIABLE RMLUI_SFML_REQUIRED_COMPONENTS_LOWER_CASE)
        find_package("SFML" "2" COMPONENTS ${RMLUI_SFML_REQUIRED_COMPONENTS_LOWER_CASE})
    endif()

    # Since we are using find_package() in basic mode, we can check the _FOUND variable 
    if(NOT SFML_FOUND)
        report_not_found_dependency("SFML" SFML::SFML)
    endif()

    #[[
        Since the RmlUi CMake project uses the SFML 2.7 namespaced target names, if the version is lower wrappers
        need to be set up.

        Because we always require the window module, we can use it to determine which iteration of the config.
    ]]

    # If any of the mandatory SFML 2.7 targets isn't present, asume SFML < 2.7 has been found and set up wrappers
    if(NOT TARGET SFML::Window)
        if(TARGET sfml-window)
            #[[
                If sfml-window exists, then that means the version is either SFML 2.5 or 2.6 which set up
                module-specific CMake targets but with different names using a config file.

                Therefore, we need to alias the target names to match those declared by SFML 2.7 and used by RmlUi.
            ]]

            # For each SFML component the project requires
            foreach(sfml_component ${RMLUI_SFML_REQUIRED_COMPONENTS})
                # Make the component name lowercase
                string(TOLOWER ${sfml_component} sfml_component_lower)

                if(TARGET sfml-${sfml_component_lower})
                    #[[
                        RMLUI_CMAKE_MINIMUM_VERSION_RAISE_NOTICE:
                        Because the target CMake version is 3.10, we can't alias non-global targets nor global imported targets.

                        Promoting an imported target to the global scope without it being necessary can cause undesired behavior,
                        specially when the project is consumed as a subdirectory inside another CMake project, therefore is not
                        recommended. Instead of that, we pseudo-alias the target creating a second INTERFACE target with alias name.
                        More info: https://cmake.org/cmake/help/latest/command/add_library.html#alias-libraries
                    ]]

                    # If the target exists, alias it
                    add_library(SFML::${sfml_component} INTERFACE IMPORTED)
                    target_link_libraries(SFML::${sfml_component} INTERFACE sfml-${sfml_component_lower})
                endif()
            endforeach()

            # Because module-specific targets already exist, there's no need for the wrapper for the older find module
            # Set up wrapper target as dumb target
            add_library(rmlui_SFML_old_wrapper INTERFACE)
        else()
            #[[
                If sfml-window doesn't exist, then SFML version is <= 2.4 and the old-variable approach used in their
                old official find module needs to be wrapped.
            ]]

            # Create our own custom INTERFACE target
            add_library(rmlui_SFML_old_wrapper INTERFACE)

            # Set up the custom target with all the libraries
            target_link_libraries(rmlui_SFML_old_wrapper INTERFACE ${SFML_LIBRARIES})
            target_include_directories(rmlui_SFML_old_wrapper INTERFACE ${SFML_INCLUDE_DIR})

            # Create dumb targets to replace the modern component-specific targets
            foreach(sfml_component ${RMLUI_SFML_REQUIRED_COMPONENTS})
                add_library(SFML::${sfml_component} INTERFACE IMPORTED)
            endforeach()
        endif()
    endif()
endif()

# X11
if(RMLUI_SAMPLES_BACKEND MATCHES "^X11")
    find_package("X11")
endif()

# --- Rendering APIs ---
# OpenGL
# RMLUI_CMAKE_MINIMUM_VERSION_RAISE_NOTICE:
# OpenGL handling changes in CMake 3.11, requiring to set CMake policy CMP0072
# More info: https://cmake.org/cmake/help/latest/policy/CMP0072.html
if(RMLUI_SAMPLES_BACKEND MATCHES "GL2$")
    find_package("OpenGL" "2")
    if(NOT TARGET OpenGL::GL)
        report_not_found_dependency("OpenGL" OpenGL::GL)
    endif()
endif()

if(RMLUI_SAMPLES_BACKEND MATCHES "GL3$")
    find_package("OpenGL" "3")
    if(NOT TARGET OpenGL::GL)
        report_not_found_dependency("OpenGL" OpenGL::GL)
    endif()
endif()

# Vulkan
if(RMLUI_SAMPLES_BACKEND MATCHES "VK$")
    find_package("Vulkan")
    if(NOT TARGET Vulkan::Vulkan)
        report_not_found_dependency("Vulkan" Vulkan::Vulkan)
    endif()
endif()
