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
 
#include "RmlUi.h"
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Factory.h>
#include <RmlUi/Core/Input.h>
#include "ElementInstancer.h"
#include "LuaElementInstancer.h"
#include "RmlUiContextsProxy.h"

namespace Rml {
namespace Lua {
#define RMLUILUA_INPUTENUM(keyident,tbl) lua_pushinteger(L,Input::KI_##keyident); lua_setfield(L,(tbl),#keyident);
#define RMLUILUA_INPUTMODIFIERENUM(keymod,tbl) lua_pushinteger(L,Input::KM_##keymod); lua_setfield(L,(tbl),#keymod);

//c++ representation of the global variable in Lua so that the syntax is consistent
LuaRmlUi lua_global_rmlui;

void LuaRmlUiPushrmluiGlobal(lua_State* L)
{
    luaL_getmetatable(L,GetTClassName<LuaRmlUi>());
    LuaRmlUiEnumkey_identifier(L);
    lua_global_rmlui.key_identifier_ref = luaL_ref(L,-2);
    LuaRmlUiEnumkey_modifier(L);
    lua_global_rmlui.key_modifier_ref = luaL_ref(L,-2);
    LuaType<LuaRmlUi>::push(L,&lua_global_rmlui,false);
    lua_setglobal(L,"rmlui");
}

template<> void ExtraInit<LuaRmlUi>(lua_State* /*L*/, int /*metatable_index*/) { return; }

int LuaRmlUiCreateContext(lua_State* L, LuaRmlUi* /*obj*/)
{
    const char* name = luaL_checkstring(L,1);
    Vector2i* dimensions = LuaType<Vector2i>::check(L,2);
    Context* new_context = CreateContext(name, *dimensions);
    if(new_context == nullptr || dimensions == nullptr)
    {
        lua_pushnil(L);
    }
    else
    {
        LuaType<Context>::push(L, new_context);
    }
    return 1;
}

int LuaRmlUiLoadFontFace(lua_State* L, LuaRmlUi* /*obj*/)
{
    const char* file = luaL_checkstring(L,1);
    lua_pushboolean(L,LoadFontFace(file));
    return 1;
}

int LuaRmlUiRegisterTag(lua_State* L, LuaRmlUi* /*obj*/)
{
    const char* tag = luaL_checkstring(L,1);
    LuaElementInstancer* lei = (LuaElementInstancer*)LuaType<ElementInstancer>::check(L,2);
    RMLUI_CHECK_OBJ(lei);
    Factory::RegisterElementInstancer(tag,lei);
    return 0;
}

int LuaRmlUiGetAttrcontexts(lua_State* L)
{
    RmlUiContextsProxy* proxy = new RmlUiContextsProxy();
    LuaType<RmlUiContextsProxy>::push(L,proxy,true);
    return 1;
}

int LuaRmlUiGetAttrkey_identifier(lua_State* L)
{
    luaL_getmetatable(L,GetTClassName<LuaRmlUi>());
    lua_rawgeti(L,-1,lua_global_rmlui.key_identifier_ref);
    return 1;
}

int LuaRmlUiGetAttrkey_modifier(lua_State* L)
{
    luaL_getmetatable(L,GetTClassName<LuaRmlUi>());
    lua_rawgeti(L,-1,lua_global_rmlui.key_modifier_ref);
    return 1;
}

void LuaRmlUiEnumkey_identifier(lua_State* L)
{
    lua_newtable(L);
    int tbl = lua_gettop(L);
	RMLUILUA_INPUTENUM(UNKNOWN,tbl)
	RMLUILUA_INPUTENUM(SPACE,tbl)
	RMLUILUA_INPUTENUM(0,tbl)
	RMLUILUA_INPUTENUM(1,tbl)
	RMLUILUA_INPUTENUM(2,tbl)
	RMLUILUA_INPUTENUM(3,tbl)
	RMLUILUA_INPUTENUM(4,tbl)
	RMLUILUA_INPUTENUM(5,tbl)
	RMLUILUA_INPUTENUM(6,tbl)
	RMLUILUA_INPUTENUM(7,tbl)
	RMLUILUA_INPUTENUM(8,tbl)
	RMLUILUA_INPUTENUM(9,tbl)
	RMLUILUA_INPUTENUM(A,tbl)
	RMLUILUA_INPUTENUM(B,tbl)
	RMLUILUA_INPUTENUM(C,tbl)
	RMLUILUA_INPUTENUM(D,tbl)
	RMLUILUA_INPUTENUM(E,tbl)
	RMLUILUA_INPUTENUM(F,tbl)
	RMLUILUA_INPUTENUM(G,tbl)
	RMLUILUA_INPUTENUM(H,tbl)
	RMLUILUA_INPUTENUM(I,tbl)
	RMLUILUA_INPUTENUM(J,tbl)
	RMLUILUA_INPUTENUM(K,tbl)
	RMLUILUA_INPUTENUM(L,tbl)
	RMLUILUA_INPUTENUM(M,tbl)
	RMLUILUA_INPUTENUM(N,tbl)
	RMLUILUA_INPUTENUM(O,tbl)
	RMLUILUA_INPUTENUM(P,tbl)
	RMLUILUA_INPUTENUM(Q,tbl)
	RMLUILUA_INPUTENUM(R,tbl)
	RMLUILUA_INPUTENUM(S,tbl)
	RMLUILUA_INPUTENUM(T,tbl)
	RMLUILUA_INPUTENUM(U,tbl)
	RMLUILUA_INPUTENUM(V,tbl)
	RMLUILUA_INPUTENUM(W,tbl)
	RMLUILUA_INPUTENUM(X,tbl)
	RMLUILUA_INPUTENUM(Y,tbl)
	RMLUILUA_INPUTENUM(Z,tbl)
	RMLUILUA_INPUTENUM(OEM_1,tbl)
	RMLUILUA_INPUTENUM(OEM_PLUS,tbl)
	RMLUILUA_INPUTENUM(OEM_COMMA,tbl)
	RMLUILUA_INPUTENUM(OEM_MINUS,tbl)
	RMLUILUA_INPUTENUM(OEM_PERIOD,tbl)
	RMLUILUA_INPUTENUM(OEM_2,tbl)
	RMLUILUA_INPUTENUM(OEM_3,tbl)
	RMLUILUA_INPUTENUM(OEM_4,tbl)
	RMLUILUA_INPUTENUM(OEM_5,tbl)
	RMLUILUA_INPUTENUM(OEM_6,tbl)
	RMLUILUA_INPUTENUM(OEM_7,tbl)
	RMLUILUA_INPUTENUM(OEM_8,tbl)
	RMLUILUA_INPUTENUM(OEM_102,tbl)
	RMLUILUA_INPUTENUM(NUMPAD0,tbl)
	RMLUILUA_INPUTENUM(NUMPAD1,tbl)
	RMLUILUA_INPUTENUM(NUMPAD2,tbl)
	RMLUILUA_INPUTENUM(NUMPAD3,tbl)
	RMLUILUA_INPUTENUM(NUMPAD4,tbl)
	RMLUILUA_INPUTENUM(NUMPAD5,tbl)
	RMLUILUA_INPUTENUM(NUMPAD6,tbl)
	RMLUILUA_INPUTENUM(NUMPAD7,tbl)
	RMLUILUA_INPUTENUM(NUMPAD8,tbl)
	RMLUILUA_INPUTENUM(NUMPAD9,tbl)
	RMLUILUA_INPUTENUM(NUMPADENTER,tbl)
	RMLUILUA_INPUTENUM(MULTIPLY,tbl)
	RMLUILUA_INPUTENUM(ADD,tbl)
	RMLUILUA_INPUTENUM(SEPARATOR,tbl)
	RMLUILUA_INPUTENUM(SUBTRACT,tbl)
	RMLUILUA_INPUTENUM(DECIMAL,tbl)
	RMLUILUA_INPUTENUM(DIVIDE,tbl)
	RMLUILUA_INPUTENUM(OEM_NEC_EQUAL,tbl)
	RMLUILUA_INPUTENUM(BACK,tbl)
	RMLUILUA_INPUTENUM(TAB,tbl)
	RMLUILUA_INPUTENUM(CLEAR,tbl)
	RMLUILUA_INPUTENUM(RETURN,tbl)
	RMLUILUA_INPUTENUM(PAUSE,tbl)
	RMLUILUA_INPUTENUM(CAPITAL,tbl)
	RMLUILUA_INPUTENUM(KANA,tbl)
	RMLUILUA_INPUTENUM(HANGUL,tbl)
	RMLUILUA_INPUTENUM(JUNJA,tbl)
	RMLUILUA_INPUTENUM(FINAL,tbl)
	RMLUILUA_INPUTENUM(HANJA,tbl)
	RMLUILUA_INPUTENUM(KANJI,tbl)
	RMLUILUA_INPUTENUM(ESCAPE,tbl)
	RMLUILUA_INPUTENUM(CONVERT,tbl)
	RMLUILUA_INPUTENUM(NONCONVERT,tbl)
	RMLUILUA_INPUTENUM(ACCEPT,tbl)
	RMLUILUA_INPUTENUM(MODECHANGE,tbl)
	RMLUILUA_INPUTENUM(PRIOR,tbl)
	RMLUILUA_INPUTENUM(NEXT,tbl)
	RMLUILUA_INPUTENUM(END,tbl)
	RMLUILUA_INPUTENUM(HOME,tbl)
	RMLUILUA_INPUTENUM(LEFT,tbl)
	RMLUILUA_INPUTENUM(UP,tbl)
	RMLUILUA_INPUTENUM(RIGHT,tbl)
	RMLUILUA_INPUTENUM(DOWN,tbl)
	RMLUILUA_INPUTENUM(SELECT,tbl)
	RMLUILUA_INPUTENUM(PRINT,tbl)
	RMLUILUA_INPUTENUM(EXECUTE,tbl)
	RMLUILUA_INPUTENUM(SNAPSHOT,tbl)
	RMLUILUA_INPUTENUM(INSERT,tbl)
	RMLUILUA_INPUTENUM(DELETE,tbl)
	RMLUILUA_INPUTENUM(HELP,tbl)
	RMLUILUA_INPUTENUM(LWIN,tbl)
	RMLUILUA_INPUTENUM(RWIN,tbl)
	RMLUILUA_INPUTENUM(APPS,tbl)
	RMLUILUA_INPUTENUM(POWER,tbl)
	RMLUILUA_INPUTENUM(SLEEP,tbl)
	RMLUILUA_INPUTENUM(WAKE,tbl)
	RMLUILUA_INPUTENUM(F1,tbl)
	RMLUILUA_INPUTENUM(F2,tbl)
	RMLUILUA_INPUTENUM(F3,tbl)
	RMLUILUA_INPUTENUM(F4,tbl)
	RMLUILUA_INPUTENUM(F5,tbl)
	RMLUILUA_INPUTENUM(F6,tbl)
	RMLUILUA_INPUTENUM(F7,tbl)
	RMLUILUA_INPUTENUM(F8,tbl)
	RMLUILUA_INPUTENUM(F9,tbl)
	RMLUILUA_INPUTENUM(F10,tbl)
	RMLUILUA_INPUTENUM(F11,tbl)
	RMLUILUA_INPUTENUM(F12,tbl)
	RMLUILUA_INPUTENUM(F13,tbl)
	RMLUILUA_INPUTENUM(F14,tbl)
	RMLUILUA_INPUTENUM(F15,tbl)
	RMLUILUA_INPUTENUM(F16,tbl)
	RMLUILUA_INPUTENUM(F17,tbl)
	RMLUILUA_INPUTENUM(F18,tbl)
	RMLUILUA_INPUTENUM(F19,tbl)
	RMLUILUA_INPUTENUM(F20,tbl)
	RMLUILUA_INPUTENUM(F21,tbl)
	RMLUILUA_INPUTENUM(F22,tbl)
	RMLUILUA_INPUTENUM(F23,tbl)
	RMLUILUA_INPUTENUM(F24,tbl)
	RMLUILUA_INPUTENUM(NUMLOCK,tbl)
	RMLUILUA_INPUTENUM(SCROLL,tbl)
	RMLUILUA_INPUTENUM(OEM_FJ_JISHO,tbl)
	RMLUILUA_INPUTENUM(OEM_FJ_MASSHOU,tbl)
	RMLUILUA_INPUTENUM(OEM_FJ_TOUROKU,tbl)
	RMLUILUA_INPUTENUM(OEM_FJ_LOYA,tbl)
	RMLUILUA_INPUTENUM(OEM_FJ_ROYA,tbl)
	RMLUILUA_INPUTENUM(LSHIFT,tbl)
	RMLUILUA_INPUTENUM(RSHIFT,tbl)
	RMLUILUA_INPUTENUM(LCONTROL,tbl)
	RMLUILUA_INPUTENUM(RCONTROL,tbl)
	RMLUILUA_INPUTENUM(LMENU,tbl)
	RMLUILUA_INPUTENUM(RMENU,tbl)
	RMLUILUA_INPUTENUM(BROWSER_BACK,tbl)
	RMLUILUA_INPUTENUM(BROWSER_FORWARD,tbl)
	RMLUILUA_INPUTENUM(BROWSER_REFRESH,tbl)
	RMLUILUA_INPUTENUM(BROWSER_STOP,tbl)
	RMLUILUA_INPUTENUM(BROWSER_SEARCH,tbl)
	RMLUILUA_INPUTENUM(BROWSER_FAVORITES,tbl)
	RMLUILUA_INPUTENUM(BROWSER_HOME,tbl)
	RMLUILUA_INPUTENUM(VOLUME_MUTE,tbl)
	RMLUILUA_INPUTENUM(VOLUME_DOWN,tbl)
	RMLUILUA_INPUTENUM(VOLUME_UP,tbl)
	RMLUILUA_INPUTENUM(MEDIA_NEXT_TRACK,tbl)
	RMLUILUA_INPUTENUM(MEDIA_PREV_TRACK,tbl)
	RMLUILUA_INPUTENUM(MEDIA_STOP,tbl)
	RMLUILUA_INPUTENUM(MEDIA_PLAY_PAUSE,tbl)
	RMLUILUA_INPUTENUM(LAUNCH_MAIL,tbl)
	RMLUILUA_INPUTENUM(LAUNCH_MEDIA_SELECT,tbl)
	RMLUILUA_INPUTENUM(LAUNCH_APP1,tbl)
	RMLUILUA_INPUTENUM(LAUNCH_APP2,tbl)
	RMLUILUA_INPUTENUM(OEM_AX,tbl)
	RMLUILUA_INPUTENUM(ICO_HELP,tbl)
	RMLUILUA_INPUTENUM(ICO_00,tbl)
	RMLUILUA_INPUTENUM(PROCESSKEY,tbl)
	RMLUILUA_INPUTENUM(ICO_CLEAR,tbl)
	RMLUILUA_INPUTENUM(ATTN,tbl)
	RMLUILUA_INPUTENUM(CRSEL,tbl)
	RMLUILUA_INPUTENUM(EXSEL,tbl)
	RMLUILUA_INPUTENUM(EREOF,tbl)
	RMLUILUA_INPUTENUM(PLAY,tbl)
	RMLUILUA_INPUTENUM(ZOOM,tbl)
	RMLUILUA_INPUTENUM(PA1,tbl)
	RMLUILUA_INPUTENUM(OEM_CLEAR,tbl)
}

void LuaRmlUiEnumkey_modifier(lua_State* L)
{
    lua_newtable(L);
    int tbl = lua_gettop(L);
    RMLUILUA_INPUTMODIFIERENUM(CTRL,tbl)
    RMLUILUA_INPUTMODIFIERENUM(SHIFT,tbl)
    RMLUILUA_INPUTMODIFIERENUM(ALT,tbl)
    RMLUILUA_INPUTMODIFIERENUM(META,tbl)
    RMLUILUA_INPUTMODIFIERENUM(CAPSLOCK,tbl)
    RMLUILUA_INPUTMODIFIERENUM(NUMLOCK,tbl)
    RMLUILUA_INPUTMODIFIERENUM(SCROLLLOCK,tbl)
}


RegType<LuaRmlUi> LuaRmlUiMethods[] = 
{
    RMLUI_LUAMETHOD(LuaRmlUi,CreateContext)
    RMLUI_LUAMETHOD(LuaRmlUi,LoadFontFace)
    RMLUI_LUAMETHOD(LuaRmlUi,RegisterTag)
    { nullptr, nullptr },
};

luaL_Reg LuaRmlUiGetters[] = 
{
    RMLUI_LUAGETTER(LuaRmlUi,contexts)
    RMLUI_LUAGETTER(LuaRmlUi,key_identifier)
    RMLUI_LUAGETTER(LuaRmlUi,key_modifier)
    { nullptr, nullptr },
};

luaL_Reg LuaRmlUiSetters[] = 
{
    { nullptr, nullptr },
};

RMLUI_LUATYPE_DEFINE(LuaRmlUi)
} // namespace Lua
} // namespace Rml
