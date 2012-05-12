/*
 * This source file is part of libRocket, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://www.librocket.com
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
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
 
#ifndef ROCKETCORELUADOCUMENT_H
#define ROCKETCORELUADOCUMENT_H
/*
    This defines the Document type in the Lua global namespace

    It inherits from Element, so check the documentation for Element.h to
    see what other functions you can call from a Document object. Document
    specific things are below

    //methods
    noreturn Document:PullToFront()
    noreturn Document:PushToBack()
    noreturn Document:Show(int flag)
    noreturn Document:Hide()
    noreturn Document:Close()
    Element Document:CreateElement(string tag)
    ElementText Document:CreateTextNode(string text)
    
    //getters
    string Document.title
    Context Document.context

    //setter
    Document.title = string
*/
#include <Rocket/Core/Lua/lua.hpp>
#include <Rocket/Core/Lua/LuaType.h>
#include <Rocket/Core/ElementDocument.h>


namespace Rocket {
namespace Core {
namespace Lua {
typedef ElementDocument Document;

template<> void ExtraInit<Document>(lua_State* L, int metatable_index);

//methods
int DocumentPullToFront(lua_State* L, Document* obj);
int DocumentPushToBack(lua_State* L, Document* obj);
int DocumentShow(lua_State* L, Document* obj);
int DocumentHide(lua_State* L, Document* obj);
int DocumentClose(lua_State* L, Document* obj);
int DocumentCreateElement(lua_State* L, Document* obj);
int DocumentCreateTextNode(lua_State* L, Document* obj);

//getters
int DocumentGetAttrtitle(lua_State* L);
int DocumentGetAttrcontext(lua_State* L);

//setters
int DocumentSetAttrtitle(lua_State* L);

RegType<Document> DocumentMethods[];
luaL_reg DocumentGetters[];
luaL_reg DocumentSetters[];

LUATYPEDECLARE(Document)
}
}
}
#endif
