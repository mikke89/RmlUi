#[[
    Set up of external dependencies required to build RmlUi itself

    Packages are configured as soft dependencies (i.e. not REQUIRED), so that a consuming project can declare them
    by other means, without an error being emitted here. For the same reason, instead of relying on variables like
    *_NOTFOUND variables, we check directly for the existence of the target.
]]

if(NOT "${RMLUI_IS_CONFIG_FILE}")
    include("${CMAKE_CURRENT_LIST_DIR}/Utils.cmake")
endif()

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
    find_package("rlottie")

    if(NOT TARGET rlottie::rlottie)
        report_not_found_dependency("rlottie" rlottie::rlottie)
    endif()
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
