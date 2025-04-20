# Using CMake 3.10 as minimum to support all platforms of interest
# https://github.com/mikke89/RmlUi/issues/198#issuecomment-1246957062
cmake_minimum_required(VERSION 3.10...3.27)

project("RmlUi"
	VERSION "6.1"
	DESCRIPTION "C++ user interface library based on the HTML and CSS standards"
	LANGUAGES "C" "CXX"
)

set(RMLUI_VERSION_RELEASE TRUE)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
	set(RMLUI_IS_ROOT_PROJECT TRUE)
endif()

# Add lists of available options for those that support it
include("CMake/OptionsLists.cmake")

option(BUILD_SHARED_LIBS "CMake standard option. Choose whether to build shared RmlUi libraries." ON)

# Declare project-specific options. Naming conventions:
#   - Use "RMLUI_" prefix to make all options specific to this project easily identifiable, and avoid colliding with any parent project variables.
#   - Do not include negations (such as "not" and "disable"), to avoid situations with double negation.
#   - Do not include a verb prefix (such as "enable" and "build"), as these are often superfluous.
option(RMLUI_SAMPLES "Build samples of the library." OFF)
set(RMLUI_BACKEND "auto" CACHE STRING "Backend to use when building the RmlUi samples. Choose one from ./CMake/OptionsLists.cmake.")
set_property(CACHE RMLUI_BACKEND PROPERTY STRINGS ${RMLUI_BACKEND_OPTIONS})
if(NOT RMLUI_BACKEND IN_LIST RMLUI_BACKEND_OPTIONS)
	message(FATAL_ERROR "The RmlUi backend '${RMLUI_BACKEND}' specified in RMLUI_BACKEND is not valid. Available options: ${RMLUI_BACKEND_OPTIONS}")
endif()

set(RMLUI_FONT_ENGINE "freetype" CACHE STRING "Font engine to build RmlUi with. If set to \"none\", RmlUi won't build with any of the included font engines and some samples might be disabled.")
set_property(CACHE RMLUI_FONT_ENGINE PROPERTY STRINGS ${RMLUI_FONT_ENGINE_OPTIONS})
if(NOT RMLUI_FONT_ENGINE IN_LIST RMLUI_FONT_ENGINE_OPTIONS)
	message(FATAL_ERROR "The RmlUi font engine '${RMLUI_FONT_ENGINE}' specified in RMLUI_FONT_ENGINE is not valid. Available options: ${RMLUI_FONT_ENGINE_OPTIONS}")
endif()
if(RMLUI_FONT_ENGINE STREQUAL "none")
	set(RMLUI_FONT_ENGINE_ENABLED FALSE)
else()
	set(RMLUI_FONT_ENGINE_ENABLED TRUE)
endif()

if(RMLUI_IS_ROOT_PROJECT)
	option(BUILD_TESTING "CMake standard option. Enable RmlUi testing projects." OFF)
	if(BUILD_TESTING)
		enable_testing()
		set(RMLUI_TESTS ON)
		if(WIN32 AND BUILD_SHARED_LIBS AND NOT CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS)
			message(FATAL_ERROR "The RmlUi testing framework cannot be built when using shared libraries with default visibility on Windows. "
				"Please disable either BUILD_SHARED_LIBS or BUILD_TESTING.")
		endif()
	endif()
endif()

option(RMLUI_LUA_BINDINGS "Build Lua bindings." OFF)
if(RMLUI_LUA_BINDINGS)
	set(RMLUI_LUA_BINDINGS_LIBRARY "lua" CACHE STRING "Choose which library to use for lua bindings when enabled.")
	set_property(CACHE RMLUI_LUA_BINDINGS_LIBRARY PROPERTY STRINGS ${RMLUI_LUA_BINDINGS_LIBRARY_OPTIONS})
	if(NOT RMLUI_LUA_BINDINGS_LIBRARY IN_LIST RMLUI_LUA_BINDINGS_LIBRARY_OPTIONS)
		message(FATAL_ERROR "The RmlUi Lua binding library '${RMLUI_LUA_BINDINGS_LIBRARY}' specified in RMLUI_LUA_BINDINGS_LIBRARY is not valid. Available options: ${RMLUI_LUA_BINDINGS_LIBRARY_OPTIONS}")
	endif()
endif()

option(RMLUI_LOTTIE_PLUGIN "Enable plugin for Lottie animations. Requires the rlottie library." OFF)
option(RMLUI_SVG_PLUGIN "Enable plugin for SVG images. Requires the lunasvg library." OFF)

option(RMLUI_HARFBUZZ_SAMPLE "Enable harfbuzz text shaping sample. Requires the harfbuzz library." OFF)

option(RMLUI_THIRDPARTY_CONTAINERS "Enable integrated third-party containers for improved performance, rather than their standard library counterparts." ON)

option(RMLUI_MATRIX_ROW_MAJOR "Use row-major matrices. Column-major matrices are used by default." OFF)

option(RMLUI_CUSTOM_RTTI "Build RmlUi with a custom implementation of run-time type information (RTTI). When enabled, all usage of language RTTI features such as dynamic_cast will be disabled." OFF)

option(RMLUI_PRECOMPILED_HEADERS "Enable precompiled headers for RmlUi." ON)

option(RMLUI_COMPILER_OPTIONS "Enable recommended compiler-specific options for the project, such as for supported warning level, standards conformance, and multiprocess builds. Turn off for full control over compiler flags." ON)

option(RMLUI_WARNINGS_AS_ERRORS "Treat compiler warnings as errors." OFF)
mark_as_advanced(RMLUI_WARNINGS_AS_ERRORS)

option(RMLUI_TRACY_PROFILING "Enable profiling with Tracy. Source files can optionally be placed in `Dependencies/tracy`." OFF)
if(RMLUI_TRACY_PROFILING)
	option(RMLUI_TRACY_MEMORY_PROFILING "Overload global operator new/delete to track memory allocations in Tracy." ON)
endif()
if(RMLUI_TRACY_PROFILING AND CMAKE_CONFIGURATION_TYPES)
	option(RMLUI_TRACY_CONFIGURATION "Enable a separate Tracy configuration type for multi-config generators such as Visual Studio, otherwise enable Tracy in all configurations." ON)
endif()

option(RMLUI_CUSTOM_CONFIGURATION "Customize the RmlUi configuration file to override the default configuration and types." OFF)
set(RMLUI_CUSTOM_CONFIGURATION_FILE "" CACHE STRING "Custom configuration file to be included in place of <RmlUi/Config/Config.h>.")
set(RMLUI_CUSTOM_INCLUDE_DIRS "" CACHE STRING "Extra include directories (use with RMLUI_CUSTOM_CONFIGURATION_FILE).")
set(RMLUI_CUSTOM_LINK_LIBRARIES "" CACHE STRING "Extra link libraries (use with RMLUI_CUSTOM_CONFIGURATION_FILE).")
if(RMLUI_CUSTOM_CONFIGURATION)
	mark_as_advanced(CLEAR RMLUI_CUSTOM_CONFIGURATION_FILE RMLUI_CUSTOM_INCLUDE_DIRS RMLUI_CUSTOM_LINK_LIBRARIES)
else()
	mark_as_advanced(FORCE RMLUI_CUSTOM_CONFIGURATION_FILE RMLUI_CUSTOM_INCLUDE_DIRS RMLUI_CUSTOM_LINK_LIBRARIES)
	if(RMLUI_CUSTOM_CONFIGURATION_FILE OR RMLUI_CUSTOM_INCLUDE_DIRS OR RMLUI_CUSTOM_LINK_LIBRARIES)
		message(STATUS "RMLUI_CUSTOM_CONFIGURATION is disabled, but custom configuration variables are set. They will have no effect.")
	endif()
endif()

set(RMLUI_INSTALL_TARGETS_DIR "" CACHE STRING "Override the install directory for the generated CMake targets.")
mark_as_advanced(RMLUI_INSTALL_TARGETS_DIR)
option(RMLUI_INSTALL_LICENSES_AND_BUILD_INFO "Install license files and build info intended for the github actions workflow." OFF)
mark_as_advanced(RMLUI_INSTALL_LICENSES_AND_BUILD_INFO)

# Add custom CMake modules path for external dependencies
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake/Modules")
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Dependencies")

include(GNUInstallDirs)
if(NOT RMLUI_INSTALL_TARGETS_DIR)
	set(RMLUI_INSTALL_TARGETS_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/RmlUi")
endif()

include("CMake/Utilities.cmake")
include("CMake/Dependencies.cmake")

include("CMake/RuntimeUtilities.cmake")
setup_binary_output_directories()
setup_runtime_dependency_set_arg()

add_subdirectory("Source")

if(RMLUI_SAMPLES OR RMLUI_TESTS)
	set(RMLUI_SHELL ON)
endif()

if(RMLUI_SHELL)
	include("CMake/BackendAutoSelection.cmake")
	include("CMake/DependenciesForBackends.cmake")
	add_subdirectory("Backends")
endif()

add_subdirectory("Samples")

if(RMLUI_TESTS)
	add_subdirectory("Tests")
endif()

# Add an interface library to include all sub-libraries.
# RMLUI_CMAKE_MINIMUM_VERSION_RAISE_NOTICE:
# From CMake 3.13 we can place target_link_libraries in the same source as each sub-library is declared, see CMP0079.
add_library(rmlui INTERFACE)
add_library(RmlUi::RmlUi ALIAS rmlui)
target_link_libraries(rmlui INTERFACE rmlui_core rmlui_debugger)
if(RMLUI_LUA_BINDINGS)
	target_link_libraries(rmlui INTERFACE rmlui_lua)
endif()
set_target_properties(rmlui PROPERTIES EXPORT_NAME "RmlUi")
install(TARGETS rmlui EXPORT RmlUiTargets)

if(RMLUI_IS_ROOT_PROJECT)
	# Skip installing any targets when RmlUi is included using `add_subdirectory`. Otherwise, if any of RmlUi's linked
	# dependencies are not exported, then this results in errors about the dependency not being in any export set.
	install(EXPORT RmlUiTargets
		DESTINATION "${RMLUI_INSTALL_TARGETS_DIR}"
		NAMESPACE RmlUi::
		FILE RmlUiTargets.cmake
	)
endif()

install(DIRECTORY
	"${CMAKE_CURRENT_SOURCE_DIR}/Include/RmlUi"
	DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

include(CMakePackageConfigHelpers)

configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/CMake/RmlUiConfig.cmake.in"
	"${CMAKE_CURRENT_BINARY_DIR}/install/RmlUiConfig.cmake"
	@ONLY
)
# RMLUI_CMAKE_MINIMUM_VERSION_RAISE_NOTICE:
# From CMake 3.11 use compatibility mode `SameMinorVersion`.
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/install/RmlUiConfigVersion.cmake"
	COMPATIBILITY ExactVersion
)
install(FILES
	"${CMAKE_CURRENT_BINARY_DIR}/install/RmlUiConfig.cmake"
	"${CMAKE_CURRENT_BINARY_DIR}/install/RmlUiConfigVersion.cmake"
	"${CMAKE_CURRENT_SOURCE_DIR}/CMake/Dependencies.cmake"
	DESTINATION
	"${RMLUI_INSTALL_TARGETS_DIR}"
)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/CMake/Modules"
	DESTINATION "${RMLUI_INSTALL_TARGETS_DIR}"
)

if(RMLUI_IS_ROOT_PROJECT)
	# Export build tree targets if RmlUi is the top-level project.
	export(EXPORT RmlUiTargets
		NAMESPACE RmlUi::
		FILE RmlUiConfig.cmake
	)
endif()

include("CMake/PackageUtilities.cmake")
if(RMLUI_INSTALL_LICENSES_AND_BUILD_INFO)
	install_licenses()
	install_build_info()
endif()
if(RMLUI_INSTALL_DEPENDENCIES_DIR)
	install_vcpkg_dependencies()
endif()

install_runtime_dependencies()
