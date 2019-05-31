/*
 * This source file is part of libRocket, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://www.librocket.com
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
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

#ifndef ROCKETCOREEVENTSPECIFICATION_H
#define ROCKETCOREEVENTSPECIFICATION_H

#include "../../Include/Rocket/Core/Header.h"
#include "../../Include/Rocket/Core/Event.h"
#include "../../Include/Rocket/Core/ID.h"

namespace Rocket {
namespace Core {


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
	// If not found: Inserts a new entry with given values.
	const EventSpecification& GetOrInsert(const String& event_type, bool interruptible, bool bubbles, DefaultActionPhase default_action_phase);

	// Get event specification for the given type.
	// If not found: Inserts a new entry with default values.
	const EventSpecification& GetOrInsert(const String& event_type);

	// Get event id for the given name.
	// If not found: Inserts a new entry with default values.
	EventId GetIdOrInsert(const String& event_type);

}



}
}

#endif
