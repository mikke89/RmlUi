#include "ContextInstancerDefault.h"
#include "../../Include/RmlUi/Core/Context.h"

namespace Rml {

ContextInstancerDefault::ContextInstancerDefault() {}

ContextInstancerDefault::~ContextInstancerDefault() {}

ContextPtr ContextInstancerDefault::InstanceContext(const String& name, RenderManager* render_manager, TextInputHandler* text_input_handler)
{
	return ContextPtr(new Context(name, render_manager, text_input_handler));
}

void ContextInstancerDefault::ReleaseContext(Context* context)
{
	delete context;
}

void ContextInstancerDefault::Release()
{
	delete this;
}

} // namespace Rml
