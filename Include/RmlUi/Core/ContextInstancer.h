#pragma once

#include "Header.h"
#include "Traits.h"
#include "Types.h"

namespace Rml {

class TextInputHandler;
class RenderManager;
class Context;
class Event;

/**
    Abstract instancer interface for instancing contexts.
 */

class RMLUICORE_API ContextInstancer : public Releasable {
public:
	virtual ~ContextInstancer();

	/// Instances a context.
	/// @param[in] name Name of this context.
	/// @param[in] render_manager The render manager used for this context.
	/// @param[in] text_input_handler The text input handler used for this context.
	/// @return The instanced context.
	virtual ContextPtr InstanceContext(const String& name, RenderManager* render_manager, TextInputHandler* text_input_handler) = 0;

	/// Releases a context previously created by this context.
	/// @param[in] context The context to release.
	virtual void ReleaseContext(Context* context) = 0;
};

} // namespace Rml
