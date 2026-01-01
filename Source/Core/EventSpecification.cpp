#include "EventSpecification.h"
#include "../../Include/RmlUi/Core/ID.h"
#include "ControlledLifetimeResource.h"

namespace Rml {

struct EventSpecificationData {
	// An EventId is an index into the specifications vector, must be listed in the same order as the EventId values.
	Vector<EventSpecification> specifications = {
		// clang-format off
		//      id                 type      interruptible  bubbles     default_action
		{EventId::Invalid       , "invalid"       , false , false , DefaultActionPhase::None},
		{EventId::Mousedown     , "mousedown"     , true  , true  , DefaultActionPhase::TargetAndBubble},
		{EventId::Mousescroll   , "mousescroll"   , true  , true  , DefaultActionPhase::None},
		{EventId::Mouseover     , "mouseover"     , true  , true  , DefaultActionPhase::Target},
		{EventId::Mouseout      , "mouseout"      , true  , true  , DefaultActionPhase::Target},
		{EventId::Focus         , "focus"         , false , false , DefaultActionPhase::Target},
		{EventId::Blur          , "blur"          , false , false , DefaultActionPhase::Target},
		{EventId::Keydown       , "keydown"       , true  , true  , DefaultActionPhase::TargetAndBubble},
		{EventId::Keyup         , "keyup"         , true  , true  , DefaultActionPhase::TargetAndBubble},
		{EventId::Textinput     , "textinput"     , true  , true  , DefaultActionPhase::TargetAndBubble},
		{EventId::Mouseup       , "mouseup"       , true  , true  , DefaultActionPhase::TargetAndBubble},
		{EventId::Click         , "click"         , true  , true  , DefaultActionPhase::TargetAndBubble},
		{EventId::Dblclick      , "dblclick"      , true  , true  , DefaultActionPhase::TargetAndBubble},
		{EventId::Load          , "load"          , false , false , DefaultActionPhase::None},
		{EventId::Unload        , "unload"        , false , false , DefaultActionPhase::None},
		{EventId::Show          , "show"          , false , false , DefaultActionPhase::None},
		{EventId::Hide          , "hide"          , false , false , DefaultActionPhase::None},
		{EventId::Mousemove     , "mousemove"     , true  , true  , DefaultActionPhase::None},
		{EventId::Dragmove      , "dragmove"      , true  , true  , DefaultActionPhase::None},
		{EventId::Drag          , "drag"          , false , true  , DefaultActionPhase::Target},
		{EventId::Dragstart     , "dragstart"     , false , true  , DefaultActionPhase::Target},
		{EventId::Dragover      , "dragover"      , true  , true  , DefaultActionPhase::None},
		{EventId::Dragdrop      , "dragdrop"      , true  , true  , DefaultActionPhase::None},
		{EventId::Dragout       , "dragout"       , true  , true  , DefaultActionPhase::None},
		{EventId::Dragend       , "dragend"       , true  , true  , DefaultActionPhase::None},
		{EventId::Handledrag    , "handledrag"    , false , true  , DefaultActionPhase::None},
		{EventId::Resize        , "resize"        , false , false , DefaultActionPhase::None},
		{EventId::Scroll        , "scroll"        , false , true  , DefaultActionPhase::None},
		{EventId::Animationend  , "animationend"  , false , true  , DefaultActionPhase::None},
		{EventId::Transitionend , "transitionend" , false , true  , DefaultActionPhase::None},
		{EventId::Change        , "change"        , false , true  , DefaultActionPhase::None},
		{EventId::Submit        , "submit"        , true  , true  , DefaultActionPhase::None},
		{EventId::Tabchange     , "tabchange"     , false , true  , DefaultActionPhase::None},
		// clang-format on
	};

	// Reverse lookup map from event type to id.
	UnorderedMap<String, EventId> type_lookup;
};

static ControlledLifetimeResource<EventSpecificationData> event_specification_data;

namespace EventSpecificationInterface {

	void Initialize()
	{
		event_specification_data.Initialize();

		auto& specifications = event_specification_data->specifications;
		auto& type_lookup = event_specification_data->type_lookup;

		type_lookup.reserve(specifications.size());
		for (auto& specification : specifications)
			type_lookup.emplace(specification.type, specification.id);

#ifdef RMLUI_DEBUG
		// Verify that all event ids are specified
		RMLUI_ASSERT((int)specifications.size() == (int)EventId::NumDefinedIds);

		for (int i = 0; i < (int)specifications.size(); i++)
		{
			// Verify correct order
			RMLUI_ASSERT(i == (int)specifications[i].id);
		}
#endif
	}

	void Shutdown()
	{
		event_specification_data.Shutdown();
	}

	static EventSpecification& GetMutable(EventId id)
	{
		auto& specifications = event_specification_data->specifications;
		size_t i = static_cast<size_t>(id);
		if (i < specifications.size())
			return specifications[i];
		return specifications[0];
	}

	// Get event specification for the given type.
	// If not found: Inserts a new entry with given values.
	static EventSpecification& GetOrInsert(const String& event_type, bool interruptible, bool bubbles, DefaultActionPhase default_action_phase)
	{
		auto& specifications = event_specification_data->specifications;
		auto& type_lookup = event_specification_data->type_lookup;

		auto it = type_lookup.find(event_type);

		if (it != type_lookup.end())
			return GetMutable(it->second);

		const size_t new_id_num = specifications.size();
		if (new_id_num >= size_t(EventId::MaxNumIds))
		{
			Log::Message(Log::LT_ERROR, "Error while registering event type '%s': Maximum number of allowed events exceeded.", event_type.c_str());
			RMLUI_ERROR;
			return specifications.front();
		}

		// No specification found for this name, insert a new entry with default values
		EventId new_id = static_cast<EventId>(new_id_num);
		specifications.push_back(EventSpecification{new_id, event_type, interruptible, bubbles, default_action_phase});
		type_lookup.emplace(event_type, new_id);
		return specifications.back();
	}

	const EventSpecification& Get(EventId id)
	{
		return GetMutable(id);
	}

	const EventSpecification& GetOrInsert(const String& event_type)
	{
		// Default values for new event types defined as follows:
		constexpr bool interruptible = true;
		constexpr bool bubbles = true;
		constexpr DefaultActionPhase default_action_phase = DefaultActionPhase::None;

		return GetOrInsert(event_type, interruptible, bubbles, default_action_phase);
	}

	EventId GetIdOrInsert(const String& event_type)
	{
		auto& type_lookup = event_specification_data->type_lookup;

		auto it = type_lookup.find(event_type);
		if (it != type_lookup.end())
			return it->second;

		return GetOrInsert(event_type).id;
	}

	EventId InsertOrReplaceCustom(const String& event_type, bool interruptible, bool bubbles, DefaultActionPhase default_action_phase)
	{
		auto& specifications = event_specification_data->specifications;

		const size_t size_before = specifications.size();
		EventSpecification& specification = GetOrInsert(event_type, interruptible, bubbles, default_action_phase);
		bool got_existing_entry = (size_before == specifications.size());

		// If we found an existing entry of same type, replace it, but only if it is a custom event id.
		if (got_existing_entry && (int)specification.id >= (int)EventId::FirstCustomId)
		{
			specification.interruptible = interruptible;
			specification.bubbles = bubbles;
			specification.default_action_phase = default_action_phase;
		}

		return specification.id;
	}

} // namespace EventSpecificationInterface
} // namespace Rml
