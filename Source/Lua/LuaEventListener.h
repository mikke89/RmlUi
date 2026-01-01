#pragma once

#include <RmlUi/Core/EventListener.h>
#include <RmlUi/Lua/IncludeLua.h>

namespace Rml {
class Element;
class ElementDocument;

namespace Lua {

class LuaEventListener : public ::Rml::EventListener {
public:
	// The plan is to wrap the code in an anonymous function so that we can have named parameters to use,
	// rather than putting them in global variables
	LuaEventListener(const String& code, Element* element);

	// This is called from a Lua Element if in element:AddEventListener it passes a function in as the 2nd
	// parameter rather than a string. We don't wrap the function in an anonymous function, so the user
	// should take care to have the proper order. The order is event,element,document.
	// narg is the position on the stack
	LuaEventListener(lua_State* L, int narg, Element* element);

	virtual ~LuaEventListener();

	// Deletes itself, which also unreferences the Lua function.
	void OnDetach(Element* element) override;

	// Calls the associated Lua function.
	void ProcessEvent(Event& event) override;

private:
	// the lua-side function to call when ProcessEvent is called
	int luaFuncRef = -1;

	Element* attached = nullptr;
	ElementDocument* owner_document = nullptr;
	String strFunc; // for debugging purposes
};

} // namespace Lua
} // namespace Rml
