/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019-2023 The RmlUi Team, and contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "EventSpecification.h"
#include "../../Include/RmlUi/Core/ID.h"

namespace Rml {

// An EventId is an index into the specifications vector.
static Vector<EventSpecification> specifications = {{EventId::Invalid, "invalid", false, false, DefaultActionPhase::None}};

// Reverse lookup map from event type to id.
static UnorderedMap<String, EventId> type_lookup;

namespace EventSpecificationInterface {

	void Initialize()
	{
		// Must be specified in the same order as in EventId
		specifications = {
			//      id                 type      interruptible  bubbles     default_action
			// clang-format off
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

		type_lookup.clear();
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

	static EventSpecification& GetMutable(EventId id)
	{
		size_t i = static_cast<size_t>(id);
		if (i < specifications.size())
			return specifications[i];
		return specifications[0];
	}

	// Get event specification for the given type.
	// If not found: Inserts a new entry with given values.
	static EventSpecification& GetOrInsert(const String& event_type, bool interruptible, bool bubbles, DefaultActionPhase default_action_phase)
	{
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
		auto it = type_lookup.find(event_type);
		if (it != type_lookup.end())
			return it->second;

		return GetOrInsert(event_type).id;
	}

	EventId InsertOrReplaceCustom(const String& event_type, bool interruptible, bool bubbles, DefaultActionPhase default_action_phase)
	{
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
