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

#ifndef RMLUI_CORE_EVENTSPECIFICATION_H
#define RMLUI_CORE_EVENTSPECIFICATION_H

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
#endif
