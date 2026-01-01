#pragma once

#include "../../Include/RmlUi/Core/Event.h"
#include "../../Include/RmlUi/Core/Header.h"
#include "../../Include/RmlUi/Core/ID.h"

namespace Rml {

struct EventSpecification {
	EventId id;
	String type;
	bool interruptible;
	bool bubbles;
	DefaultActionPhase default_action_phase;
};

namespace EventSpecificationInterface {

	void Initialize();
	void Shutdown();

	// Get event specification for the given id.
	// Returns the 'invalid' event type if no specification exists for id.
	const EventSpecification& Get(EventId id);

	// Get event specification for the given type.
	// If not found: Inserts a new entry with default values.
	const EventSpecification& GetOrInsert(const String& event_type);

	// Get event id for the given name.
	// If not found: Inserts a new entry with default values.
	EventId GetIdOrInsert(const String& event_type);

	// Insert a new specification for the given event_type.
	// If the type already exists, it will be replaced if and only if the event type is not an internal type.
	EventId InsertOrReplaceCustom(const String& event_type, bool interruptible, bool bubbles, DefaultActionPhase default_action_phase);
} // namespace EventSpecificationInterface

} // namespace Rml
