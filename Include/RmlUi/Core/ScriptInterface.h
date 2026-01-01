#pragma once

#include "Header.h"
#include "Traits.h"
#include "Types.h"

namespace Rml {

/**
    Base class for all objects that hold a scriptable object.
 */

class RMLUICORE_API ScriptInterface : public Releasable {
public:
	RMLUI_RTTI_Define(ScriptInterface)

	virtual ~ScriptInterface() {}

	virtual ScriptObject GetScriptObject() const { return nullptr; }
};

} // namespace Rml
