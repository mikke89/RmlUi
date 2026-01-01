#pragma once

#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementInstancer.h>
#include <RmlUi/Lua/IncludeLua.h>

namespace Rml {
namespace Lua {

class LuaElementInstancer : public ::Rml::ElementInstancer {
public:
	LuaElementInstancer(lua_State* L);
	/// Instances an element given the tag name and attributes.
	/// @param[in] parent The element the new element is destined to be parented to.
	/// @param[in] tag The tag of the element to instance.
	/// @param[in] attributes Dictionary of attributes.
	ElementPtr InstanceElement(Element* parent, const String& tag, const XMLAttributes& attributes) override;
	/// Releases an element instanced by this instancer.
	/// @param[in] element The element to release.
	void ReleaseElement(Element* element) override;

	int ref_InstanceElement;

	// Pushes on to the top of the stack the table named EVENTINSTNACERFUNCTIONS
	void PushFunctionsTable(lua_State* L);
};

} // namespace Lua
} // namespace Rml
