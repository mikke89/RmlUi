#[[
	Set up of external dependencies required to build RmlUi itself

	Packages are configured as soft dependencies (i.e. not REQUIRED), so that a consuming project can declare them
	by other means, without an error being emitted here. For the same reason, instead of relying on variables like
	*_NOTFOUND variables, we check directly for the existence of the target.
]]

if(RMLUI_FONT_ENGINE STREQUAL "freetype")
	find_package("Freetype")

	if(FREETYPE_VERSION_STRING)
		if(FREETYPE_VERSION_STRING VERSION_EQUAL "2.11.0" AND CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
			message(WARNING "Using Freetype 2.11.0 with MSVC can cause issues, please upgrade to Freetype 2.11.1 or newer.")
		endif()
	endif()

	report_dependency_found_or_error("Freetype" Freetype::Freetype "Freetype font engine enabled")
endif()

if(RMLUI_LOTTIE_PLUGIN)
	find_package("rlottie")
	report_dependency_found_or_error("rlottie" rlottie::rlottie "Lottie plugin enabled")
endif()

if(RMLUI_SVG_PLUGIN)
	find_package("lunasvg")
	report_dependency_found_or_error("lunasvg" lunasvg::lunasvg "SVG plugin enabled")
endif()

# The Lua and LuaJIT modules don't provide targets, so make our own, or let users define the target already.
if(RMLUI_LUA_BINDINGS AND RMLUI_LUA_BINDINGS_LIBRARY STREQUAL "lua")
	if(NOT TARGET Lua::Lua)
		find_package("Lua" REQUIRED)
		add_library(Lua::Lua INTERFACE IMPORTED)
		set_target_properties(Lua::Lua PROPERTIES
			INTERFACE_LINK_LIBRARIES "${LUA_LIBRARIES}"
			INTERFACE_INCLUDE_DIRECTORIES "${LUA_INCLUDE_DIR}"
		)
	endif()
	add_library(RmlUi::External::Lua ALIAS Lua::Lua)
	report_dependency_found_or_error("Lua" Lua::Lua "Lua bindings enabled")
endif()

if(RMLUI_LUA_BINDINGS AND RMLUI_LUA_BINDINGS_LIBRARY STREQUAL "luajit")
	if(NOT TARGET LuaJIT::LuaJIT)
		find_package("LuaJIT" REQUIRED)
		add_library(LuaJIT::LuaJIT INTERFACE IMPORTED)
		set_target_properties(LuaJIT::LuaJIT PROPERTIES
			INTERFACE_LINK_LIBRARIES "${LUAJIT_LIBRARY}"
			INTERFACE_INCLUDE_DIRECTORIES "${LUAJIT_INCLUDE_DIR}"
		)
	endif()
	add_library(RmlUi::External::Lua ALIAS LuaJIT::LuaJIT)
	report_dependency_found_or_error("Lua" LuaJIT::LuaJIT "Lua bindings enabled with LuaJIT")
endif()

if(NOT RMLUI_IS_CONFIG_FILE)
	if(RMLUI_TRACY_PROFILING AND RMLUI_TRACY_CONFIGURATION)
		enable_configuration_type(Tracy Release ON)
	else()
		enable_configuration_type(Tracy Release OFF)
	endif()
endif()

if(RMLUI_TRACY_PROFILING)
	find_package(Tracy CONFIG QUIET)

	if(RMLUI_IS_CONFIG_FILE)
		report_dependency_found_or_error("Tracy" Tracy::TracyClient)
	endif()

	if(NOT TARGET Tracy::TracyClient)
		message(STATUS "Trying to add Tracy from subdirectory 'Dependencies/tracy'.")
		add_subdirectory("Dependencies/tracy")

		if(NOT TARGET Tracy::TracyClient)
			message(FATAL_ERROR "Tracy client not found. Either "
				"(a) make sure target Tracy::TracyClient is available from parent project, "
				"(b) Tracy can be found as a config package, or "
				"(c) Tracy source files are located in 'Dependencies/Tracy'.")
		endif()

		if(RMLUI_IS_ROOT_PROJECT)
			# Tracy does not export its targets to the build tree. Do that for it here, otherwise CMake will emit an
			# error about target `TracyClient` not being located in any export set.
			export(EXPORT TracyConfig
				NAMESPACE Tracy::
				FILE TracyTargets.cmake
			)
		endif()
	endif()
endif()
