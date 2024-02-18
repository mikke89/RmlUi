#[[
    Set up of external dependencies required to build RmlUi itself

    Packages are configured as soft dependencies (i.e. not REQUIRED), so that a consuming project can declare them
    by other means, without an error being emitted here. For the same reason, instead of relying on variables like
    *_NOTFOUND variables, we check directly for the existence of the target.
]]

include("${PROJECT_SOURCE_DIR}/CMake/Utils.cmake")

if(RMLUI_FONT_INTERFACE STREQUAL "freetype")
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

if(RMLUI_LUA_BINDINGS)
    # The Lua and LuaJIT modules don't provide targets, so make our own, and mark the libraries as required.
    add_library(rmlui_lua_interface INTERFACE IMPORTED)
    add_library(RmlUi::Private::LuaInterface ALIAS rmlui_lua_interface)
    if(RMLUI_LUA_BINDINGS_LIBRARY STREQUAL "luajit")
        find_package("LuaJIT" REQUIRED)
        target_include_directories(rmlui_lua_interface INTERFACE ${LUAJIT_INCLUDE_DIR})
        target_link_libraries(rmlui_lua_interface INTERFACE ${LUAJIT_LIBRARY})
    elseif(RMLUI_LUA_BINDINGS_LIBRARY STREQUAL "lua")
        find_package("Lua" REQUIRED)
        target_include_directories(rmlui_lua_interface INTERFACE ${LUA_INCLUDE_DIR})
        target_link_libraries(rmlui_lua_interface INTERFACE ${LUA_LIBRARIES})
    else()
        message(FATAL_ERROR "Invalid value for Lua bindings library.")
    endif()
endif()
