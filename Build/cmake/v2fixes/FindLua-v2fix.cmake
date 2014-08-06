#.rst:
# FindLua
# -------
#
#
#
# Locate Lua library This module defines
#
# ::
#
#   LUA_FOUND          - if false, do not try to link to Lua
#   LUA_LIBRARIES      - both lua and lualib
#   LUA_INCLUDE_DIR    - where to find lua.h
#   LUA_VERSION_STRING - the version of Lua found
#   LUA_VERSION_MAJOR  - the major version of Lua
#   LUA_VERSION_MINOR  - the minor version of Lua
#   LUA_VERSION_PATCH  - the patch version of Lua
#
#
#
# Note that the expected include convention is
#
# ::
#
#   #include "lua.h"
#
# and not
#
# ::
#
#   #include <lua/lua.h>
#
# This is because, the lua location is not standardized and may exist in
# locations other than lua/

#=============================================================================
# Copyright 2007-2009 Kitware, Inc.
# Copyright 2013 Rolf Eike Beer <eike@sf-mail.de>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 
# * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
# 
# * Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution.
# 
# * Neither the names of Kitware, Inc., the Insight Software Consortium,
# nor the names of their contributors may be used to endorse or promote
# products derived from this software without specific prior written
# permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#===========================================================================

unset(_lua_include_subdirs)
unset(_lua_library_names)

# this is a function only to have all the variables inside go away automatically
function(set_lua_version_vars)
    set(LUA_VERSIONS5 5.3 5.2 5.1 5.0)

    if (Lua_FIND_VERSION_EXACT)
        if (Lua_FIND_VERSION_COUNT GREATER 1)
            set(lua_append_versions ${Lua_FIND_VERSION_MAJOR} ${Lua_FIND_VERSION_MINOR})
        endif ()
    elseif (Lua_FIND_VERSION)
        # once there is a different major version supported this should become a loop
        if (NOT Lua_FIND_VERSION_MAJOR GREATER 5)
            if (Lua_FIND_VERSION_COUNT EQUAL 1)
                set(lua_append_versions ${LUA_VERSIONS5})
            else ()
                foreach (subver IN LISTS LUA_VERSIONS5)
                    if (NOT subver VERSION_LESS ${Lua_FIND_VERSION})
                        list(APPEND lua_append_versions ${subver})
                    endif ()
                endforeach ()
            endif ()
        endif ()
    else ()
        # once there is a different major version supported this should become a loop
        set(lua_append_versions ${LUA_VERSIONS5})
    endif ()

    foreach (ver IN LISTS lua_append_versions)
        string(REGEX MATCH "^([0-9]+)\\.([0-9]+)$" _ver "${ver}")
        list(APPEND _lua_include_subdirs
             include/lua${CMAKE_MATCH_1}${CMAKE_MATCH_2}
             include/lua${CMAKE_MATCH_1}.${CMAKE_MATCH_2}
             include/lua-${CMAKE_MATCH_1}.${CMAKE_MATCH_2}
        )
        list(APPEND _lua_library_names
             lua${CMAKE_MATCH_1}${CMAKE_MATCH_2}
             lua${CMAKE_MATCH_1}.${CMAKE_MATCH_2}
             lua-${CMAKE_MATCH_1}.${CMAKE_MATCH_2}
        )
    endforeach ()

    set(_lua_include_subdirs "${_lua_include_subdirs}" PARENT_SCOPE)
    set(_lua_library_names "${_lua_library_names}" PARENT_SCOPE)
endfunction(set_lua_version_vars)

set_lua_version_vars()

find_path(LUA_INCLUDE_DIR lua.h
  HINTS
    ENV LUA_DIR
  PATH_SUFFIXES ${_lua_include_subdirs} include/lua include
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /sw # Fink
  /opt/local # DarwinPorts
  /opt/csw # Blastwave
  /opt
)
unset(_lua_include_subdirs)

find_library(LUA_LIBRARY
  NAMES ${_lua_library_names} lua
  HINTS
    ENV LUA_DIR
  PATH_SUFFIXES lib
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /sw
  /opt/local
  /opt/csw
  /opt
)
unset(_lua_library_names)

if (LUA_LIBRARY)
    # include the math library for Unix
    if (UNIX AND NOT APPLE AND NOT BEOS)
        find_library(LUA_MATH_LIBRARY m)
        set(LUA_LIBRARIES "${LUA_LIBRARY};${LUA_MATH_LIBRARY}")
    # For Windows and Mac, don't need to explicitly include the math library
    else ()
        set(LUA_LIBRARIES "${LUA_LIBRARY}")
    endif ()
endif ()

if (LUA_INCLUDE_DIR AND EXISTS "${LUA_INCLUDE_DIR}/lua.h")
    # At least 5.[012] have different ways to express the version
    # so all of them need to be tested. Lua 5.2 defines LUA_VERSION
    # and LUA_RELEASE as joined by the C preprocessor, so avoid those.
    file(STRINGS "${LUA_INCLUDE_DIR}/lua.h" lua_version_strings
         REGEX "^#define[ \t]+LUA_(RELEASE[ \t]+\"Lua [0-9]|VERSION([ \t]+\"Lua [0-9]|_[MR])).*")

    string(REGEX REPLACE ".*;#define[ \t]+LUA_VERSION_MAJOR[ \t]+\"([0-9])\"[ \t]*;.*" "\\1" LUA_VERSION_MAJOR ";${lua_version_strings};")
    if (LUA_VERSION_MAJOR MATCHES "^[0-9]+$")
        string(REGEX REPLACE ".*;#define[ \t]+LUA_VERSION_MINOR[ \t]+\"([0-9])\"[ \t]*;.*" "\\1" LUA_VERSION_MINOR ";${lua_version_strings};")
        string(REGEX REPLACE ".*;#define[ \t]+LUA_VERSION_RELEASE[ \t]+\"([0-9])\"[ \t]*;.*" "\\1" LUA_VERSION_PATCH ";${lua_version_strings};")
        set(LUA_VERSION_STRING "${LUA_VERSION_MAJOR}.${LUA_VERSION_MINOR}.${LUA_VERSION_PATCH}")
    else ()
        string(REGEX REPLACE ".*;#define[ \t]+LUA_RELEASE[ \t]+\"Lua ([0-9.]+)\"[ \t]*;.*" "\\1" LUA_VERSION_STRING ";${lua_version_strings};")
        if (NOT LUA_VERSION_STRING MATCHES "^[0-9.]+$")
            string(REGEX REPLACE ".*;#define[ \t]+LUA_VERSION[ \t]+\"Lua ([0-9.]+)\"[ \t]*;.*" "\\1" LUA_VERSION_STRING ";${lua_version_strings};")
        endif ()
        string(REGEX REPLACE "^([0-9]+)\\.[0-9.]*$" "\\1" LUA_VERSION_MAJOR "${LUA_VERSION_STRING}")
        string(REGEX REPLACE "^[0-9]+\\.([0-9]+)[0-9.]*$" "\\1" LUA_VERSION_MINOR "${LUA_VERSION_STRING}")
        string(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.([0-9]).*" "\\1" LUA_VERSION_PATCH "${LUA_VERSION_STRING}")
    endif ()

    unset(lua_version_strings)
endif()

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LUA_FOUND to TRUE if
# all listed variables are TRUE
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Lua
                                  REQUIRED_VARS LUA_LIBRARIES LUA_INCLUDE_DIR
                                  VERSION_VAR LUA_VERSION_STRING)

mark_as_advanced(LUA_INCLUDE_DIR LUA_LIBRARY LUA_MATH_LIBRARY)
