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

#include "precompiled.h"
#include <RmlUi/Controls/Lua/Controls.h>
#include <RmlUi/Core/Lua/LuaType.h>
#include <RmlUi/Core/Lua/lua.hpp>
#include <RmlUi/Core/Lua/Interpreter.h>
#include <RmlUi/Core/Log.h>
#include "SelectOptionsProxy.h"
#include "DataFormatter.h"
#include "DataSource.h"
#include "ElementForm.h"
#include "ElementFormControl.h"
#include "ElementFormControlSelect.h"
#include "ElementFormControlDataSelect.h"
#include "ElementFormControlInput.h"
#include "ElementFormControlTextArea.h"
#include "ElementDataGrid.h"
#include "ElementDataGridRow.h"
#include "ElementTabSet.h"

using Rml::Core::Lua::LuaType;
namespace Rml {
namespace Controls {
namespace Lua {

//This will define all of the types from RmlControls for Lua. There is not a
//corresponding function for types of RmlCore, because they are defined automatically
//when the Interpreter starts.
void RegisterTypes(lua_State* L)
{
    if(Rml::Core::Lua::Interpreter::GetLuaState() == nullptr)
    {
        Rml::Core::Log::Message(Rml::Core::Log::LT_ERROR,
            "In Rml::Controls::Lua::RegisterTypes: Tried to register the \'Controls\' types for Lua without first initializing the Interpreter.");
        return;
    }
    LuaType<ElementForm>::Register(L);
    LuaType<ElementFormControl>::Register(L);
        //Inherits from ElementFormControl
        LuaType<ElementFormControlSelect>::Register(L);
            LuaType<ElementFormControlDataSelect>::Register(L);
        LuaType<ElementFormControlInput>::Register(L);
        LuaType<ElementFormControlTextArea>::Register(L);
    LuaType<DataFormatter>::Register(L);
    LuaType<DataSource>::Register(L);
    LuaType<ElementDataGrid>::Register(L);
    LuaType<ElementDataGridRow>::Register(L);
    LuaType<ElementTabSet>::Register(L);
    //proxy tables
    LuaType<SelectOptionsProxy>::Register(L);
}

}
}
}
