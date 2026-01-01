#pragma once

#include "../../Include/RmlUi/Core/ElementDocument.h"
#include "../../Include/RmlUi/Core/EventListener.h"
#include "ElementDebugDocument.h"

namespace Rml {
namespace Debugger {

class ElementDataModels : public ElementDebugDocument, public EventListener {
public:
	RMLUI_RTTI_DefineWithParent(ElementDataModels, ElementDebugDocument)

	ElementDataModels(const String& tag);
	~ElementDataModels();

	bool Initialise(Context* debug_context);
	void Reset();

	void SetDebugContext(Context* debug_context);

protected:
	void ProcessEvent(Event& event) override;
	void OnUpdate() override;

private:
	void UpdateContent();

	Context* debug_context = nullptr;

	double previous_update_time = {};

	SmallOrderedMap<String, String> model_rml_map;
};

} // namespace Debugger
} // namespace Rml
