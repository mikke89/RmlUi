/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019 The RmlUi Team, and contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */
 
#ifndef RMLUI_LUA_PAIRS_H
#define RMLUI_LUA_PAIRS_H 

#include <RmlUi/Lua/IncludeLua.h>
#include <RmlUi/Lua/Utilities.h>
#include <utility>

namespace Rml {
namespace Lua {
    
template <class T>
int PairsConvertTolua(lua_State* L, const T& v);

template <class F, class S>
int PairsConvertTolua(lua_State* L, const std::pair<F, S>& v) {
    int nresult = 0;
    nresult += PairsConvertTolua(L, v.first);
    nresult += PairsConvertTolua(L, v.second);
    return nresult;
}

template <>
inline int PairsConvertTolua<String>(lua_State* L, const String& s) {
    lua_pushlstring(L, s.c_str(), s.size());
    return 1;
}

template <>
inline int PairsConvertTolua<Variant>(lua_State* L, const Variant& v) {
    PushVariant(L, &v);
    return 1;
}

template <typename T>
struct PairsHelper {
    static int next(lua_State* L) {
        PairsHelper* self = static_cast<PairsHelper*>(lua_touserdata(L, lua_upvalueindex(1)));
        if (self->m_first == self->m_last) {
            return 0;
        }
        int nreslut = PairsConvertTolua(L, *self->m_first);
        ++(self->m_first);
        return nreslut;
    }
    static int destroy(lua_State* L) {
        static_cast<PairsHelper*>(lua_touserdata(L, 1))->~PairsHelper();
        return 0;
    }
    static int constructor(lua_State* L, const T& first, const T& last) {
        void* storage = lua_newuserdata(L, sizeof(PairsHelper<T>));
        if (luaL_newmetatable(L, "RmlUi::Lua::PairsHelper")) {
            static luaL_Reg mt[] = {
                {"__gc", destroy},
                {NULL, NULL},
            };
            luaL_setfuncs(L, mt, 0);
        }
        lua_setmetatable(L, -2);
        lua_pushcclosure(L, next, 1);
        new (storage) PairsHelper<T>(first, last);
        return 1;
    }
    PairsHelper(const T& first, const T& last)
        : m_first(first)
        , m_last(last)
    { }
    T m_first;
    T m_last;
};

template <class Iterator>
int MakePairs(lua_State* L, const Iterator& first, const Iterator& last) {
    return PairsHelper<Iterator>::constructor(L, first, last);
}

template <class Container>
int MakePairs(lua_State* L, const Container& container) {
    return MakePairs(L, std::begin(container), std::end(container));
}

inline int ipairsaux(lua_State* L) {
    lua_Integer i = luaL_checkinteger(L, 2) + 1;
    lua_pushinteger(L, i);
#if LUA_VERSION_NUM >= 503
    return (lua_geti(L, 1, i) == LUA_TNIL) ? 1 : 2;
#else
    lua_pushinteger(L, i);
    lua_gettable(L, 1);
    return (lua_isnil(L, -1)) ? 1 : 2;
#endif
}

inline int MakeIntPairs(lua_State* L) {
    luaL_checkany(L, 1);
    lua_pushcfunction(L, ipairsaux);
    lua_pushvalue(L, 1);
    lua_pushinteger(L, 0);
    return 3;
}

} // namespace Lua
} // namespace Rml

#endif
