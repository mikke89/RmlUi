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
 
//whenever you want to have a type exposed to lua, you have to have it in here as
//LuaType<type>::Register(L);
//It will be #included in the correct place in Interpreter.cpp, as to make this more like a macro file

//THE ORDER DOES MATTER
//You have to register a base class before any others can inherit from it
LuaType<Vector2i>::Register(L);
LuaType<Vector2f>::Register(L);
LuaType<Colourf>::Register(L);
LuaType<Colourb>::Register(L);
LuaType<Log>::Register(L);
LuaType<Element>::Register(L);
    //things that inherit from element
    LuaType<Document>::Register(L);
    LuaType<ElementStyle>::Register(L);
LuaType<Event>::Register(L);
LuaType<Context>::Register(L);
LuaType<rocket>::Register(L);
