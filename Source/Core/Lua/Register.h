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
LuaType<rocket>::Register(L);
