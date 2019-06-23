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
 
#ifndef RMLUICONTROLSLUADATAFORMATTER_H
#define RMLUICONTROLSLUADATAFORMATTER_H


#include <RmlUi/Core/Lua/lua.hpp>
#include <RmlUi/Core/Lua/LuaType.h>
#include "LuaDataFormatter.h"

using Rml::Core::Lua::LuaType;
namespace Rml {
namespace Controls {
namespace Lua {
typedef LuaDataFormatter DataFormatter;
//method
int DataFormatternew(lua_State* L);

//setter
int DataFormatterSetAttrFormatData(lua_State* L);

extern Rml::Core::Lua::RegType<DataFormatter> DataFormatterMethods[];
extern luaL_Reg DataFormatterGetters[];
extern luaL_Reg DataFormatterSetters[];
}
}
}
//for DataFormatter.new
namespace Rml { namespace Core { namespace Lua {
template<> void ExtraInit<Rml::Controls::Lua::DataFormatter>(lua_State* L, int metatable_index);
LUACONTROLSTYPEDECLARE(Rml::Controls::Lua::DataFormatter)
}}}
#endif
