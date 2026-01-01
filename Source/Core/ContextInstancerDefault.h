#pragma once

#include "../../Include/RmlUi/Core/ContextInstancer.h"

namespace Rml {

/**
    Default instancer for instancing contexts.
 */

class ContextInstancerDefault : public ContextInstancer {
public:
	ContextInstancerDefault();
	virtual ~ContextInstancerDefault();

	/// Instances a context.
	ContextPtr InstanceContext(const String& name, RenderManager* render_manager, TextInputHandler* text_input_handler) override;

	/// Releases a context previously created by this context.
	void ReleaseContext(Context* context) override;

	/// Releases this context instancer.
	void Release() override;
};

} // namespace Rml
