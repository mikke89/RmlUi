#[[
	Set up of external dependencies required to build RmlUi itself

	Packages are configured as soft dependencies (i.e. not REQUIRED), so that a consuming project can declare them
	by other means, without an error being emitted here. For the same reason, instead of relying on variables like
	*_NOTFOUND variables, we check directly for the existence of the target.
]]

if(RMLUI_FONT_ENGINE STREQUAL "freetype")
	find_package("Freetype")

	if(NOT TARGET Freetype::Freetype)
		report_not_found_dependency("Freetype" Freetype::Freetype)
	endif()

	if(DEFINED FREETYPE_VERSION_STRING)
		if(FREETYPE_VERSION_STRING VERSION_EQUAL "2.11.0" AND CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
			message(WARNING "Using Freetype 2.11.0 with MSVC can cause issues, please upgrade to Freetype 2.11.1 or newer.")
		endif()
	endif()
endif()

if(RMLUI_LOTTIE_PLUGIN)
	execute_process(COMMAND cmake --help-property-list OUTPUT_VARIABLE CMAKE_PROPERTY_LIST)
	## Convert command output into a CMake list
	STRING(REGEX REPLACE ";" "\\\\;" CMAKE_PROPERTY_LIST "${CMAKE_PROPERTY_LIST}")
	STRING(REGEX REPLACE "\n" ";" CMAKE_PROPERTY_LIST "${CMAKE_PROPERTY_LIST}")

	list(REMOVE_DUPLICATES CMAKE_PROPERTY_LIST)

	function(print_target_properties tgt)
		if(NOT TARGET ${tgt})
			message("There is no target named '${tgt}'")
			return()
		endif()

		foreach(prop ${CMAKE_PROPERTY_LIST})
			string(REPLACE "<CONFIG>" "${CMAKE_BUILD_TYPE}" prop ${prop})
			get_target_property(propval ${tgt} ${prop})
			if(propval)
				message("${tgt} ${prop} = ${propval}")
			endif()
		endforeach(prop)
	endfunction(print_target_properties)

	find_package("rlottie")

	if(NOT TARGET rlottie::rlottie)
		report_not_found_dependency("rlottie" rlottie::rlottie)
	endif()

	message(STATUS "Lottie plugin enabled, rlottie found.")
	print_target_properties(rlottie::rlottie)
endif()

if(RMLUI_SVG_PLUGIN)
	find_package("lunasvg")

	if(NOT TARGET lunasvg::lunasvg)
		report_not_found_dependency("lunasvg" lunasvg::lunasvg)
	endif()
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

	if(NOT TARGET Tracy::TracyClient)
		if(RMLUI_IS_CONFIG_FILE)
			report_not_found_dependency("Tracy" Tracy::TracyClient)
		endif()

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
