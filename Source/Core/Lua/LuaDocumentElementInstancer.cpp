#include "precompiled.h"
#include "LuaDocumentElementInstancer.h"
#include "LuaDocument.h"

namespace Rocket {
namespace Core {
namespace Lua {

/// Instances an element given the tag name and attributes.
/// @param[in] parent The element the new element is destined to be parented to.
/// @param[in] tag The tag of the element to instance.
/// @param[in] attributes Dictionary of attributes.
Element* LuaDocumentElementInstancer::InstanceElement(Element* parent, const String& tag, const XMLAttributes& attributes)
{
    return new LuaDocument(tag);
}
/// Releases an element instanced by this instancer.
/// @param[in] element The element to release.
void LuaDocumentElementInstancer::ReleaseElement(Element* element)
{
    delete element;
}
/// Release the instancer.
void LuaDocumentElementInstancer::Release()
{
    delete this;
}

}
}
}