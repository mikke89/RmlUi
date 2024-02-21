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

if(RMLUI_LUA_BINDINGS AND NOT TARGET RmlUi::External::Lua)
    # The Lua and LuaJIT modules don't provide targets, so make our own, or let users define the target already.
    if(RMLUI_LUA_BINDINGS_LIBRARY STREQUAL "lua")
        find_package("Lua" REQUIRED)
        add_library(rmlui_external_lua INTERFACE)
        target_include_directories(rmlui_external_lua INTERFACE ${LUA_INCLUDE_DIR})
        target_link_libraries(rmlui_external_lua INTERFACE ${LUA_LIBRARIES})
        add_library(RmlUi::External::Lua ALIAS rmlui_external_lua)
    elseif(RMLUI_LUA_BINDINGS_LIBRARY STREQUAL "luajit")
        find_package("LuaJIT" REQUIRED)
        add_library(rmlui_external_luajit INTERFACE)
        target_include_directories(rmlui_external_luajit INTERFACE ${LUAJIT_INCLUDE_DIR})
        target_link_libraries(rmlui_external_luajit INTERFACE ${LUAJIT_LIBRARY})
        add_library(RmlUi::External::Lua ALIAS rmlui_external_luajit)
    else()
        message(FATAL_ERROR "Invalid value for Lua bindings library.")
    endif()
endif()
