#ifndef ROCKETCORELUALUADOCUMENTELEMENTINSTANCER_H
#define ROCKETCORELUALUADOCUMENTELEMENTINSTANCER_H

#include <Rocket/Core/ElementInstancer.h>

namespace Rocket {
namespace Core {
namespace Lua {

class LuaDocumentElementInstancer : public ElementInstancer
{
    /// Instances an element given the tag name and attributes.
	/// @param[in] parent The element the new element is destined to be parented to.
	/// @param[in] tag The tag of the element to instance.
	/// @param[in] attributes Dictionary of attributes.
	virtual Element* InstanceElement(Element* parent, const String& tag, const XMLAttributes& attributes);
	/// Releases an element instanced by this instancer.
	/// @param[in] element The element to release.
	virtual void ReleaseElement(Element* element);
	/// Release the instancer.
	virtual void Release();
};

}
}
}
#endif
