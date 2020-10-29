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
 
#include <RmlUi/Lua/Utilities.h>
#include <RmlUi/Core/Variant.h>

namespace Rml {
namespace Lua {

void PushVariant(lua_State* L, const Variant* var)
{
    if(var == nullptr)
    {
        lua_pushnil(L);
        return;
    }
    Variant::Type vartype = var->GetType();
    switch(vartype)
    {
    case Variant::BYTE:
    case Variant::CHAR:
    case Variant::INT:
        lua_pushinteger(L,var->Get<int>());
        break;
    case Variant::FLOAT:
        lua_pushnumber(L,var->Get<float>());
        break;
    case Variant::COLOURB:
        LuaType<Colourb>::push(L,new Colourb(var->Get<Colourb>()),true);
        break;
    case Variant::COLOURF:
        LuaType<Colourf>::push(L,new Colourf(var->Get<Colourf>()),true);
        break;
    case Variant::STRING:
        lua_pushstring(L,var->Get<String>().c_str());
        break;
    case Variant::VECTOR2:
        //according to Variant.inl, it is going to be a Vector2f
        LuaType<Vector2f>::push(L,new Vector2f(var->Get<Vector2f>()),true);
        break;
    case Variant::VOIDPTR:
        lua_pushlightuserdata(L,var->Get<void*>());
        break;
    default:
        lua_pushnil(L);
        break;
    }
}

} // namespace Lua
} // namespace Rml
