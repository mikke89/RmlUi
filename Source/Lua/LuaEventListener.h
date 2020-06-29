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
 
#ifndef RMLUI_LUA_LUAEVENTLISTENER_H
#define RMLUI_LUA_LUAEVENTLISTENER_H

#include <RmlUi/Core/EventListener.h>
#include <RmlUi/Lua/IncludeLua.h>

namespace Rml {
class Element;
class ElementDocument;

namespace Lua {

class LuaEventListener : public ::Rml::EventListener
{
public:
    //The plan is to wrap the code in an anonymous function so that we can have named parameters to use,
    //rather than putting them in global variables
    LuaEventListener(const String& code, Element* element);

    //This is called from a Lua Element if in element:AddEventListener it passes a function in as the 2nd
    //parameter rather than a string. We don't wrap the function in an anonymous function, so the user
    //should take care to have the proper order. The order is event,element,document.
	//narg is the position on the stack
    LuaEventListener(lua_State* L, int narg, Element* element);

    virtual ~LuaEventListener();

	// Deletes itself, which also unreferences the Lua function.
	void OnDetach(Element* element) override;

	// Calls the associated Lua function.
	void ProcessEvent(Event& event) override;

private:
    //the lua-side function to call when ProcessEvent is called
    int luaFuncRef = -1;

    Element* attached = nullptr;
    ElementDocument* owner_document = nullptr;
    String strFunc; //for debugging purposes
};

} // namespace Lua
} // namespace Rml
#endif
